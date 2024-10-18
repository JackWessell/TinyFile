#ifndef cutils_h
#define cutils_h 

void client_write();
void client_read();
void *client_shm_open(char *name, int size);
void client_shm_close(void *loc, int size);
//client decode includes a message to decode and an optional file to include when doing reads/writes.
int client_decode(char *msg, char *file);
#endif
