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

#include "mq.h"
/* 
When acknowledging contact, the server will send 0 followed by the client's unique id.
When acknowledging compression, the server will send 1, followed by a shmem name - open shmem and initiate compression protocol.
When completing compression, the server will send 2 - read from shmem.
*/
#define LINE_SIZE 128
//This is the core of the algorithm. Here, the client passes information to the host over and over until it has been exhausted.
//for now, file is hard-coded. Will eventually be passed as arg once sample application calling TinyClient is created.
int shm_size;
void *shmsegs[2];
extern int id;
extern int compressed;
extern int synct;
void client_write(char *file){
    //Our two semaphores. We will post client sem, signaling to daemon that we have written to shared memory.
    //Then, we will wait on daemon sem, blocking until the daemon has done what it needs to do with the data we have written.
    void *shmseg = shmsegs[synct == gettid()];
    sem_t *daemon_sem = shmseg;
    sem_t *client_sem = (shmseg+32);
    FILE *fp = fopen(file, "r");
    if(fp == NULL){
        printf("%s", file);
        perror("File Not Found!");
        //Host should not hang if the file isn't found.
        sprintf(shmseg+64,"%d",0);
        sem_post(client_sem);
        return;
    }
    //calculate how many bytes in the file and let daemon know how many iterations we will need to send over the entire file.
    fseek(fp, 0, SEEK_END);
    long int file_size = ftell(fp);
    int iters = ((int) (file_size/(shm_size-64)) +1);
    fseek(fp, 0, SEEK_SET);
    char *buf = (char *) malloc(file_size+10);
    if(buf == NULL){
        perror("File buffer allocation failed!");
        sprintf(shmseg+64,"%d",0);
        sem_post(client_sem);
        return;
    }
    //printf("File: %s\n", file);
    sprintf(shmseg+64,"%d",iters);
    sem_post(client_sem);
    sem_wait(daemon_sem);
    //read data into our buffer
    int amt = (shm_size-64);
    fread(buf, file_size, 1, fp);
    for(int i = 0; i < iters; i++){
        //on last iteration, may access more bytes than in rest of array.
        if(i == iters -1){
            amt = file_size - i*(shm_size-64);
        }
        memcpy(shmseg+64, &buf[i*(shm_size-64)], amt);
        sem_post(client_sem);
        sem_wait(daemon_sem);
    }
    return;
}
void client_read(char *file){
    void *shmseg = shmsegs[(synct == gettid())];
    sem_t *daemon_sem = shmseg;
    sem_t *client_sem = (shmseg+32);
    int iters;
    //wait for daemon to write number of iterations we will be using.
    sem_wait(daemon_sem);
    iters = atoi(shmseg+64);
    //daemon encountered an issue
    if(iters == 0){
        return;
    }
    char *buf = (char *) malloc(iters*(shm_size-64));
    sem_post(client_sem);
    for(int i = 0; i<iters;i++){
        sem_wait(daemon_sem);
        memcpy(&buf[i*(shm_size-64)], shmseg+64, (shm_size-64));
        sem_post(client_sem);
    }
    //save file to configurable output space (eventually)
    printf("Compression Complete!\n");
    compressed += 1;
}
//open a shared memory segment and map it into our address space. If we have already opened it before, we will just return the file descriptor.
void *client_shm_open(char *name, int size){
    int shm_fd = shm_open(name, O_RDWR, 0666);
    if(shm_fd == -1){
        perror("Opening shared memory failed.");
        return NULL;
    }
    void* shm_ptr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(shm_ptr == MAP_FAILED){
        perror("Memory mapping failed");
        return NULL;
    }
    close(shm_fd);
    return shm_ptr;
}
void client_shm_close(void *loc, int size){
    int res = munmap(loc, size);
    if(res == -1){
        perror("Unlinking shared memory failed");
        return;
    }
    return;
}
int client_decode(char *msg, char *file){
    char val[2];
    val[0] = msg[0];
    val[1] = '\0';
    int type = atoi(val);
    //the server acknowledges our participation and returns to us our unique id.
    if(type == 0){
        char *num = (char *) malloc(strlen(msg) * sizeof(char)); 
        msg_parse(num, msg, 2);
        id = atoi(num);
        free(num);
        return 0;
    }
    //server is prepared to compress our data. Need to open the shm we have agreed to use. And begin the protocol
    if(type == 1){
        char *name = (char *) malloc(strlen(msg) * sizeof(char)); 
        msg_parse(name, msg, 2);
        char *size = (char *) malloc(strlen(msg) * sizeof(char));
        msg_parse(size, msg, 3 + strlen(name));
        shm_size = atoi(size);
        free(size);
        shmsegs[gettid() == synct] = client_shm_open(name, shm_size);
        client_write(file);
        client_shm_close(shmsegs[gettid() == synct], shm_size);
        free(name);
    }
    //server has completed our compression and is waiting to return our data. There is nothing that needs to be done as we already have our segment mapped.s
    //After, we close our shared memory as we may not use the same segment during the next request
    if(type == 2){
        //re-open  s
        char *name = (char *) malloc(strlen(msg) * sizeof(char)); 
        msg_parse(name, msg, 2);
        char *size = (char *) malloc(strlen(msg) * sizeof(char));
        msg_parse(size, msg, 3 + strlen(name));
        shm_size = atoi(size);
        free(size);
        shmsegs[gettid() == synct] = client_shm_open(name, shm_size);
        client_read(file);
        client_shm_close(shmsegs[gettid() == synct], shm_size);
        free(name);
    }
    return 1;
}