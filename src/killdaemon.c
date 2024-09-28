#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

#include "daemon.h"
//When called, this function will issue the SIGTERM signal to the Daemon, causing it to execute its termination and clean-up protocol
int main(int argc, char* argv[]){
    //Open the PID file to find the PID of the most recently initialized Daemon.
    int res = open(PID_FILE,  O_RDWR, 0644);
    if(res == -1){
        perror("Failed to open PID file. Exiting...");
        return -1;
    }
    int proc;
    read(res, &proc, sizeof(int));
    //now, read currently running processes to see if daemon is actually running at the moment.
    FILE *fp;
    char line[MAX_PROCESS_LINE];
    fp = popen("ps -e -o pid", "r");
    if (fp == NULL) {
        perror("Failed get process IDs. Exiting...");
        return -1;
    }
    //iterate over processes in the system, ensuring that the daemon is currently running.
    char running = 0;
    printf("%d, \n", proc);
    while (fgets(line, MAX_PROCESS_LINE, fp) != NULL) {
        if (strstr(line, "PID") == NULL) {
            int pid;
            sscanf(line, "%d", &pid);
            if(pid == proc){
                running = 1;
                break;
            }
        }
    }
    if(!running){
        printf("Daemon is not running!");
        return -1;
    }
    //if we made it to this point, issue the signal to the daemon. It will never block the SIGTERM signal, so we shouldn't require any complex waiting process.
    syscall(__NR_tkill, proc, SIGTERM);
    return 1;
}