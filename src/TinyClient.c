#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>  
#include <sys/stat.h>
#include <mqueue.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h> 
#include <string.h>
#include <stdlib.h> 
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include <pthread.h> 
#include <sys/time.h>
#include <stdint.h>

#include "clientutils.h"
#include "utils.h"
#include "mq.h"
#include "signals.h"

//some global information for the client.
int id;
int syncq;
int asyncq;
//information necessary to keep track of our pending asynchronous transfers.
struct async_file *backlog;
int curr_async = -1;
//Name of file we are currently compressing will be stored in the global variable. Not the cleanest implementation I know...
int compressed;
int num;
int synct;
//the signal handler for our asynchronous transfers. If we receive a messge beginning with 1, pull a file off of the backlog and set curr_file.
//If it begins with 2, we are just receiving data and we can simply call client_decode.
void async_check(int *compressed_async){
    char *buf = (char *) malloc(mq_size * sizeof(char));
    int res = recv(asyncq, buf, 2);
    //no messages yet.
    if(res == 0 || curr_async == -1){
        free(buf);
        return;
    }    
    //if 2, we are receiving compressed data. Nothing to do as client_decode handles everythin.
    if(buf[0] == '2'){
        client_decode(buf, "");
        struct timeval tv2;
        gettimeofday(&tv2, NULL);
        int time = timeval_difference_microseconds(&(backlog[*compressed_async].start), &tv2);
        printf("Request %d from process %d took %d microseconds\n", compressed, synct, time);
        free(buf);
        (*compressed_async)++;
        return;
    }
    //otherwise, we are sending the daemon a file.
    char curr_file[LINE_SIZE];
    memcpy(curr_file, backlog[curr_async].name, strlen(backlog[curr_async].name));
    curr_file[strlen(backlog[curr_async].name)] = '\0';
    client_decode(buf, curr_file);
    curr_async++;
    free(buf);
    return;
}
//we have an asynchronous thread that will continually query our async queue. Main thread adds files to the backlog, it pulls them off.
void *async_thread(void *arg){
    //as long as all files haven't been compressed, continuously check the asynchronous message queue.
    int compressed_async = 0;
    while(compressed != num){
        async_check(&compressed_async);
    }
    return NULL;
}
int main(int argc, char* argv[]){
    if(argc != 7){
        printf("Usage: ./TinyClient -n /CLIENT_NAME --files FILES.txt --num NUMBER_OF_FILES");
        return -1;
    }
    char name[16];
    FILE *jobs;

    for(int i = 0; i < argc; i++){
        if(strcmp(argv[i], "-n") == 0){
            if(strlen(argv[i]) > 16){
                printf("TinyClient name must be <= 16 characters!!!");
                return -1;
            }
            memcpy(name, argv[i+1], strlen(argv[i+1]));   
            if(argv[i+1][0] != '/'){
                printf("TinyClient name must be begin with a forward slash!!!");
                return -1;
            }       
        }
        if(strcmp(argv[i], "--files") == 0){
            jobs = fopen(argv[i+1], "r");
            if(jobs == NULL){
                printf("Could not open file of jobs!!!");
                return -1;
            }
        }
        if(strcmp(argv[i], "--num") == 0){
            num = atoi(argv[i+1]);
            if(num < 1){
                printf("Must include at least one file to compress!");
                return -1;
            }
            backlog = malloc(num * sizeof(struct async_file));
        }
    }

    struct daemon_info daemon;
    daemon.msg[9] = '\0';
    if(daemon_running(&daemon) == -1){
        return -1;
    }
    compressed = 0;
    //open our queues. We need a writeq and two readq's. One readq is opened in non-blocking mode to allow for asynchronous compression.
    int writeq = open_mq(daemon.msg, O_WRONLY, 1);
    char sync[17];
    char async[17];
    memcpy(sync, name, strlen(name));
    memcpy(async, name, strlen(name));
    sync[strlen(name)] = 's';
    sync[strlen(name) + 1] = '\0';
    async[strlen(name)] = 'a';
    async[strlen(name) +1] = '\0';
    syncq = create_mq(sync, O_RDONLY, 1);
    asyncq = create_mq(async, O_RDONLY | O_NONBLOCK, 1);
    //Setup: initialize our async thread
    pthread_t athread;
    synct = gettid();
    pthread_create(&athread, NULL, async_thread, NULL);
    //########################## Step 1: send handshake to Daemon, receive id ###################################
    char *buf = (char *) malloc(mq_size * sizeof(char));
    sprintf(buf, "%d|%s|", 0, name);
    send(writeq, buf, 2);
    //this part will always be blocking.
    recv(syncq, buf, 2);
    client_decode(buf, "");
    //########################### Step 2a: request daemon services. Receive shared memory name and pass data to daemon ###################
    char mode[LINE_SIZE];
    char file[LINE_SIZE];
    int tot_async = 0;
    //maintain an additional list of asynchronous file names. This will allow us to keep track of the files we have requested services for asynchronously
    while(fgets(file, LINE_SIZE, jobs) != NULL){
        if(fgets(mode, LINE_SIZE, jobs) == NULL || (atoi(mode) != 0 && atoi(mode) != 1)){
            printf("Each file must have an associated mode of either 0 (synchronous) or 1 (asynchronous)!!!");
            mq_unlink(sync);
            mq_unlink(async);
            return -1;
        }
        //last character of each line is the newline character. This will throw of fopen.
        int type = atoi(mode);
        sprintf(buf, "%d|%d|%d|", 1, id, type);
        struct timeval tv;
        gettimeofday(&tv, NULL);
        //daemon's write q is synchronous. This part blocks even in mode 1.
        send(writeq, buf, 2);
        //if synchronous we do our work.
        if(type == 0){
            file[strlen(file)-1] = '\0';
            //must block signals while receiving.
            recv(syncq, buf, 2);
            client_decode(buf, file);    
            //wait for daemon to complete its job
            recv(syncq, buf, 2);
            client_decode(buf, file);
            struct timeval tv2;
            gettimeofday(&tv2, NULL);
            int time = timeval_difference_microseconds(&tv,&tv2);
            printf("Request %d from process %d took %d microseconds\n", compressed, synct, time);
        }
        //transfer mode is asynchronous. We will add the file we need to transfer to our backlog and continue;
        else{
            backlog[tot_async].start = tv;
            file[strlen(file)-1] = '\0';
            memcpy(backlog[tot_async].name, file, strlen(file));
            tot_async++;
            if(curr_async == -1){
                curr_async = 0; 
            }
        }
    }
    //client may still be waiting on asynchronous requests from other thread.
    while(compressed != num){
        continue;
    }
    pthread_join(athread, NULL);
    //Step 3: Send disconnection message to daemon and close message queue fds
    //it will unlink the message queues - this is necessary because mq_unlink may not work if another process has an open file descriptor.
    mq_close(syncq);
    mq_close(asyncq);
    sprintf(buf, "%d|%d|%s|", 2, id, name);
    send(writeq, buf, 2);
    free(backlog);
    free(buf);
    return 0;
}