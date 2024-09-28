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
#include "daemon.h"
#include "signal.h"

//the code for initializing our daemon process. It will create and be responsible for all shared memory used for IPC.
#define SIZE 4096
#define NAME "TinyFile!"
char *shm_name;
int main(int argc, char* argv[]) {
    int res = init_daemon(1);
    if(res == -1){
        cleanup_daemon();
        return EXIT_FAILURE;
    }
    //our daemon has been created. Time to set up shared memory segment. 
    daemon_log("Daemon started.\n");
    int shm_fd;
    void* shm_ptr;
    shm_name = NAME;
    /* create the shared memory object */
    shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    /* configure the size of the shared memory object */
    ftruncate(shm_fd, SIZE);
    /* memory map the shared memory object */
    shm_ptr = mmap(0, SIZE, PROT_WRITE, MAP_SHARED, shm_fd, 0);
 
    /* write to the shared memory object */
    sprintf(shm_ptr, "%s", "Hello Shared Memory!");
    while (1)
    {
        //TODO: Insert daemon code here.
        sleep (25);
        break;
    }
    cleanup_daemon();

    return EXIT_SUCCESS;
}