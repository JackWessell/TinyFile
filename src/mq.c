#include <fcntl.h>           /* For O_* constants */
#include <sys/stat.h>        /* For mode constants */
#include <mqueue.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h> 

#include "mq.h"

int create_mq(char *name, int mode, int err_fd){
    struct mq_attr new_attr;
    new_attr.mq_flags = 0;
    new_attr.mq_maxmsg = mq_num;
    new_attr.mq_msgsize = mq_size;
    int fd = mq_open(name,  O_CREAT  | mode, 0644, &new_attr);
    //throw error to appropriate location
    if(fd == -1){
        char buf[128];
        sprintf(buf, "%s", strerror(errno));
        write(err_fd, buf, 128);
        //not the best way to handle the error? TinyFile will need to clean up so can't just exit - Hopefully will not cause too many issues...
        return -1;
    }
    return fd;
}
int open_mq(char *name, int mode, int err_fd){
    int fd = mq_open(name, mode);
    if(fd == -1){
        char buf[128];
        sprintf(buf, "%s\n", name);
        write(err_fd, buf, 128);
        return -1;
    }
    return fd;
}
void send(int fd, char *buf, int err_fd){
    int send = mq_send(fd, buf, mq_size, 0);
    if(send == -1){
        char err[128];
        sprintf(err, "Failed to send message");
        write(err_fd, err, 128);
        return;
    }
    return;
}
int recv(int fd, char *buf, int err_fd){
    int rec = mq_receive(fd, buf, mq_size,0);
    if(errno == EAGAIN){
        errno = 0;
        return 0;
    }
    if(rec == -1){
        char err[128];
        sprintf(err, "Failed to receive message: %s", strerror(errno));
        write(err_fd, err, 128);
        return -1;
    }
    buf[rec] = '\0';
    return 1;
}
//Fill buf with info in msg.
void msg_parse(char *buf, char *msg, int start){
    int i = start;
    while(msg[i] != '|'){
        buf[i - start] = msg[i];
        i++;
    }
    buf[i-start] = '\0';
    return;
}
