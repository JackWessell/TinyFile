#ifndef cutils_h
#define cutils_h 

void host_write(char *buf, int buf_size, int tid);
char *compress(char *buf, size_t *compressed_len, int iters);
char *host_read(int *iters, int tid);
int daemon_decode(char *msg, int err_fd, int tid);

#endif