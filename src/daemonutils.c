#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h> 
#include <unistd.h>
#include <semaphore.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <fcntl.h>
#include <pthread.h> 

#include "daemon.h"
#include "mq.h"
#include "snappy.h"



extern char* shm_name[MAX_NUM];
extern void* shm_ptr[MAX_NUM];
extern int seg_size;
extern int num_segs;
extern struct clients client_info;
void host_write(char *buf, int buf_size, int tid){
    sem_t *daemon_sem = shm_ptr[tid];
    sem_t *client_sem = shm_ptr[tid] + 32;
    int iters = ((int) buf_size/(seg_size-64)) + 1;
    sprintf(shm_ptr[tid]+64,"%d",iters);
    sem_post(daemon_sem);
    sem_wait(client_sem);
    for(int i = 0; i < iters; i++){
        memcpy(shm_ptr[tid]+64, &buf[i*(seg_size-64)], (seg_size-64));
        sem_post(daemon_sem);
        sem_wait(client_sem);
    }
    return;
}
char *compress(char *buf, size_t *compressed_len, int iters){
    struct snappy_env snappy;
    int res = snappy_init_env(&snappy);
    if(res != 0){
        daemon_log("Snappy Initialization Failed!\n", "", 0);
        return NULL;
    }

    char *compressed = (char *) malloc(snappy_max_compressed_length(iters*(seg_size-64)));
    res = snappy_compress(&snappy, buf, iters*(seg_size-64), compressed, compressed_len);
    if(res != 0){
        daemon_log("Snappy Compression Failed!\n", "", 0);
        return NULL;
    }
    char buf2[128];
    snappy_free_env(&snappy);
    sprintf(buf2, "iters: %d \n", iters);
    daemon_log(buf2, "", 0);
    sprintf(buf2, "Compressed length: %ld bytes\n", *compressed_len);
    daemon_log(buf2, "", 0);
    sprintf(buf2, "Uncompressed length: %d bytes\n", iters*(seg_size-64));
    daemon_log(buf2, "", 0);
    return compressed;
}
char *host_read(int *iters, int tid){
    sem_t *daemon_sem = shm_ptr[tid];
    sem_t *client_sem = shm_ptr[tid] + 32;
    char buf2[128];
    //wait until client places data for the first time. First interaction will simply be the number of interations needed to fully communicate the file contents.
    sem_wait(client_sem);
    *iters = atoi(shm_ptr[tid]+64);
    sprintf(buf2, "%d\n", *iters);
    daemon_log(buf2, "", 0);
    //client encountered an error.
    if(*iters == 0){
        return NULL;
    }
    char *buf = (char *) malloc((*iters)*(seg_size-64));
    sem_post(daemon_sem);
    for(int i = 0; i < *iters; i++){
        sem_wait(client_sem);
        memcpy(&buf[i*(seg_size-64)], shm_ptr[tid]+64, (seg_size-64));
        sem_post(daemon_sem);
    }
    return buf;
}
/*
In initialization, the client will send: 0 followed by its name. - Call open_mq when received.
When requesting services, the client will send: 1, followed by compression type (sync/aysnc), followed by its assigned value - initiate compression protocol.
When signing off, the client will send: 2, followed by its name - call close_mq when received.
*/
int daemon_decode(char *msg, int err_fd, int tid){
    char val[2];
    val[0] = msg[0];
    val[1] = '\0';
    int type = atoi(val);
    char buf[128];
    sprintf(buf, "Thread: %d, Message: %s", tid, msg);
    daemon_log(buf, "", 0);
    //this is a client trying to initiate contact with us. We need to register their message queue and send them their numeric identifier
    if(type == 0){
        //format message to open the message queue.
        char *qname = (char *) malloc(17*sizeof(char));
        msg_parse(qname, msg, 2);
        int name_len = strlen(qname);
        qname[name_len] = 's';
        qname[name_len + 1] = '\0';
        int sync = open_mq(qname, O_WRONLY, err_fd);
        qname[name_len] = 'a';
        qname[name_len + 1] = '\0';
        int async = open_mq(qname, O_WRONLY, err_fd);
        //handle client information in a thread-safe manner.
        pthread_mutex_lock(&(client_info.lock));
        client_info.clients++;
        client_info.client_qs[client_info.clients][0] = sync;
        client_info.client_qs[client_info.clients][1] = async;
        pthread_mutex_unlock(&(client_info.lock));
        //prepare the response and send to the client in synchronous mode.
        char buf[mq_size];
        sprintf(buf, "%d|%d|", 0, client_info.clients);
        mq_send(client_info.client_qs[client_info.clients][0], buf, mq_size, 0);
        free(qname);
        return 0;
    } 
    //client is requesting the daemon's services. format is req_type|id|comm_type|.
    // We already know rec_type, id will be sent, comm_type is 0 for sync, 1 for async
    if(type == 1){
        //determine who sent us the message and whether synchronous or asynchronous
        char *num = (char *) malloc(12*sizeof(char));
        msg_parse(num, msg, 2);
        int id = atoi(num);
        free(num);
        int comm_type = atoi(&msg[strlen(msg)-2]);
        
        //send client the shm segment we will use and enter our reading function.
        char buf[mq_size];
        sprintf(buf, "%d|%s|%d|",1, shm_name[tid], seg_size);
        mq_send(client_info.client_qs[id][comm_type], buf, mq_size, 0);
        int iters;
        char *res = host_read(&iters, tid);
        //perform service for the client
        size_t compressed_len;
        char *compressed = compress(res, &compressed_len, iters);
        free(res);
        //Tell client compression is complete (re-send shm segment - not the best solution but allows for sync and async)
        sprintf(buf, "%d|%s|%d|",2, shm_name[tid], seg_size);
        mq_send(client_info.client_qs[id][comm_type], buf, mq_size, 0);
        //daemon writes to shared memory.
        host_write(compressed, compressed_len, tid);
        free(compressed);
        return 1;
    }
    //client is disconnecting. Close the respective message queues.
    if(type == 2){
        char *num = (char *) malloc(mq_size*sizeof(char));
        //get client id and name from message
        msg_parse(num, msg, 2);
        int len = strlen(num);
        int id = atoi(num);
        msg_parse(num, msg, 3 + len);
        //close synchronous queue (0) and asynchronous queue (1)
        len = strlen(num);
        num[len] = 's';
        num[len+1] = '\0';
        mq_close(client_info.client_qs[id][0]);
        mq_unlink(num);
        num[len] = 'a';
        num[len+1] = '\0';
        mq_close(client_info.client_qs[id][1]);
        mq_unlink(num);
        free(num);
        return 0;
    }
    return 0;  
}