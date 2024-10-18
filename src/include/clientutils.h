#ifndef cutils_h
#define cutils_h 
#include <sys/time.h>

#define LINE_SIZE 128
//A struct to hold the information of an asynchronous file request. It maintains the name of the file as well as the time when the request was made.
//This can be used to calculate the time when the request is complete as well as the time of various intermediate steps if necessary.
struct async_file{
    char name[LINE_SIZE];
    struct timeval start;
};
void client_write();
void client_read();
void *client_shm_open(char *name, int size);
void client_shm_close(void *loc, int size);
//client decode includes a message to decode and an optional file to include when doing reads/writes.
int client_decode(char *msg, char *file);
#endif
