#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"
int daemon_running(daemon_t *daemon){
    //Open the PID file to find the PID of the most recently initialized Daemon.
    FILE *file = fopen("../TinyFile.pid",  "r");
    if(file == NULL){
        perror("Failed to open PID file. Exiting...");
        return -1;
    }
    fread(&(daemon->pid), sizeof(int), 1, file);
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
    while (fgets(line, MAX_PROCESS_LINE, fp) != NULL) {
        if (strstr(line, "PID") == NULL) {
            int pid;
            sscanf(line, "%d", &pid);
            if(pid == daemon->pid){
                running = 1;
                break;
            }
        }
    }
    if(!running){
        printf("Daemon is not running!");
        return -1;
    }
    //If we have made it here, the daemon is currently running.
    fread(&(daemon->msg), sizeof(char), 9, file);
    return 1;
}
