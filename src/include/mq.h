//Handles all message queue functionality - from creating, opening, closing, formatting decoding messages, etc.
#ifndef __MQ_H
#define __MQ_H

#define mq_num 10
#define mq_size 128

//note: do not pass O_CREAT to create_mq - this is understood.
//each of these functions requires an output file descriptor - this is because TinyClients can print to stdout/stderr, but 
//TinyFile daemon can only print to its output log. This is to make error handling as descriptive and clean as possible.
int create_mq(char *name, int mode, int err_fd);
int open_mq(char *name, int mode, int err_fd);
void send(int fd, char *buf, int err_fd);
int recv(int fd, char *buf, int err_fd);
void msg_parse(char *buf,char *msg, int start);
//A client will send the following messages to the server: init contact, request compression, break contact.

//The server will send the following messages to the client: contact acknowledged, compression acknowledged, compression complete.

#endif