#ifndef __SIGNAL_H 
#define __SIGNAL_H
//basic signal handling functionality.
void install_signal_handler(int signo, void(*handler)());
void block_signal(int signo);
void unblock_signal(int signo);

//our client will get signaled by the OS every .1 seconds. This is when it checks its asynchronous message queues.
#define VTALRM_SEC 0
#define VTALRM_USEC 100000
extern void init_vtalrm_timeslice();

#endif