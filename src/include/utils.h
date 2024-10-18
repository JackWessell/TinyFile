#ifndef __UTILS_H
#define __UTILS_H

#define PID_FILE "../TinyFile.pid"
#define MAX_PROCESS_LINE 2048
//number of chars in the name of the daemon's message queue
#define SIZE 10
//define the aspects of the daemon we allow clients to know: its PID and the name of its input message queue/
//Knowing the pid may be unsafe, but we are assuming well-behaved clients
typedef struct daemon_info{
    int pid;
    char msg[SIZE];
} daemon_t;
//take a reference to a daemon and check if there is a daemon running. If there is, populate the daemon with the necessary info and return 1.
//Else, return 0
int64_t timeval_difference_microseconds(struct timeval *start, struct timeval *end);
int daemon_running(daemon_t *daemon);
int lock_file(FILE *file);
int unlock_file(FILE *file);
//int init_msgq(char *name, int flag);
#endif