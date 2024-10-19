#define _GNU_SOURCE
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <sys/resource.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <fcntl.h>  
#include <mqueue.h>
#include <string.h>
#include <errno.h>
#include <pthread.h> 

#include "daemonutils.h"
#include "snappy.h"
#include "daemon.h"
#include "mq.h"
//the code for initializing our daemon process. It will create and be responsible for all shared memory used for IPC.
//Daemon will also create a receiving message queue. When new clients spin up, they create a response queue that the daemon can use to send information to clients privately.

#define NAME "TinyFile!"
//the size of the child process stack.

//define the daemon's global variables
//first: shared memory variables
char *shm_name[MAX_NUM];
int shm_fd[MAX_NUM];
void* shm_ptr[MAX_NUM];
pthread_t threads[MAX_NUM];
struct clients client_info;
int log_fd;
int readq;
int seg_size;
int num_segs;
int parent;
//our array of queues for clients. Eventually, when both synchronous and asynchronous communication is complete, each client will have a synchronous q and an async q
//the workhorse of our code. Here, each of our threads creates the shared memory segment that they will be responsible for and continuously performs reads
//on the readq. 

void * daemon_loop(void *arg){
    char name[12];
    struct thread_arg *true_arg = (struct thread_arg *) arg;
    int id = true_arg->id;
    sprintf(name, "%s%d", NAME, id);
    /* create the shared memory object */
    shm_fd[id] = shm_open(name, O_CREAT | O_RDWR, 0666);
    /* configure the size of the shared memory object */
    ftruncate(shm_fd[id], seg_size);
    /* memory map the shared memory object */
    shm_ptr[id] = mmap(0, seg_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd[id], 0);
    shm_name[id] = name;
    close(shm_fd[id]);
    /*create the semaphores in our shared memory. For now, we only need 2. I
    Sem 0 (addr 0)- This is the daemon-side semaphore. Clients will wait on this, while Daemon will post.
    Sem 1  (addr 32)- This is the client-side semaphore. Daemon will wait on it while client will post.
    At the first address not taken up by semaphores, we will have an int (4 bytes) giving the size of the message and */
    sem_init(shm_ptr[id], 1, 0);
    sem_init((shm_ptr[id]+sizeof(sem_t)), 1, 0);
    while (1)
    {
        //wait on the receive queue for a message to arrive
        char msg_ptr[mq_size];
        int rec = mq_receive(readq, msg_ptr, mq_size, 0);
        daemon_decode(msg_ptr, log_fd, id);
    }  
    return 0;  
}
int main(int argc, char* argv[]) {
    //process args
    if(argc != 5){
        printf("Usage: TinyClient --n_sms NUM_SEG --sms_size SEG_SIZE\n");
        return -1;
    }
    for(int i = 0; i < argc; i++){
        
        if(strcmp(argv[i], "--n_sms") == 0){
            num_segs = atoi(argv[i+1]);
            if(num_segs < 1 || num_segs > MAX_NUM){
                printf("--n_sms must be an integer >= than 1 and less than %d!\n", MAX_NUM);
                return -1;
            }
        }
        if(strcmp(argv[i], "--sms_size") == 0){
            seg_size = atoi(argv[i+1]) + 64; //we add an additional 32 bytes for metadata. In this case, semaphores.
            if(seg_size < 32 || seg_size > MAX_SIZE){
                printf("--sms_size must be an integer >= 32 and less that %d!\n", MAX_SIZE);
                return -1;
            }
        }
    }
    //first, set up our message queue. This is because we will need to include its name in our PID file in order for clients to interact with us.
    //This will be done in read only mode, as daemon will only ever read from this queue.
    
    client_info.clients = 0;
    readq = create_mq(MQ_NAME, O_RDONLY, 2);
    //daemon setup
    int res = init_daemon(1);
    if(res == -1){
        cleanup_daemon();
        return EXIT_FAILURE;
    }
    //our daemon has been created. Time to set up shared memory segment. 
    daemon_log("Daemon started.\n");
    parent = gettid();
    //create our remaining threads.
    struct thread_arg ids[MAX_NUM];
    for(int i = 0; i < num_segs-1; i++){
        ids[i+1].id = i+1;
        pthread_create(&threads[i], NULL, daemon_loop, &ids[i+1]);
    }
    //main daemon thread calls loop 
    struct thread_arg parent;
    parent.id = 0;
    daemon_loop(&parent);
    return EXIT_SUCCESS;
}