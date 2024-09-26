#include <stdio.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sched.h>
#include <signal.h>
#include <setjmp.h>
#include <errno.h>
#include <assert.h>

#include "signals.h"
void install_signal_handler(int signo, void (*handler)()){
	sigset_t set;
	struct sigaction act;

	/* Setup the handler */
	act.sa_handler = handler;
	act.sa_flags = SA_RESTART;
	sigaction(signo, &act,0);

	/* Unblock the signal */
	sigemptyset(&set);
	sigaddset(&set, signo);
	sigprocmask(SIG_UNBLOCK, &set, NULL);

	return;
}

void  block_signal(int signo){
	sigset_t set;

	/* Block the signal */
	sigemptyset(&set);
	sigaddset(&set, signo);
	sigprocmask(SIG_BLOCK, &set, NULL);

	return;
}

void unblock_signal(int signo){
	sigset_t set;

	/* Unblock the signal */
	sigemptyset(&set);
	sigaddset(&set, signo);
	sigprocmask(SIG_UNBLOCK, &set, NULL);

	return;
}