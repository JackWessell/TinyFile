#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <sys/resource.h>

#include "daemon.h"
int main(int argc, char* argv[]) {
    int res = init_daemon(1);
    if(res == -1){
        cleanup_daemon();
        return EXIT_FAILURE;
    }
    while (1)
    {
        //TODO: Insert daemon code here.
        daemon_log("First daemon started.\n");
        sleep (20);
        break;
    }
    cleanup_daemon();

    return EXIT_SUCCESS;
}