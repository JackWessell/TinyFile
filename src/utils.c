#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <unistd.h>

#include <fcntl.h>

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
const int64_t MICROSECONDS_IN_ONE_SECOND = 1000000;
int64_t timeval_difference_microseconds(struct timeval *start, struct timeval *end)
{
    int64_t difference = (end->tv_sec - start->tv_sec) * MICROSECONDS_IN_ONE_SECOND;
    difference += (end->tv_usec - start->tv_usec);
    return difference;
}
int lock_file(FILE *file) {
    int fd = fileno(file);
    struct flock fl;
    fl.l_type = F_WRLCK;    // Write lock
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;         // Offset from l_whence
    fl.l_len = 0;           // Lock the entire file
    fl.l_pid = getpid();    // Process ID

    return fcntl(fd, F_SETLKW, &fl);  // F_SETLKW for blocking lock
}

int unlock_file(FILE *file) {
    int fd = fileno(file);
    struct flock fl;
    fl.l_type = F_UNLCK;    // Unlock
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;
    fl.l_pid = getpid();

    return fcntl(fd, F_SETLK, &fl);
}