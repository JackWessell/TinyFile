#define _GNU_SOURCE
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <fcntl.h>  
#include <mqueue.h>
#include <pthread.h>

#include "daemon.h"
#include "signals.h"

extern int readq;
extern int log_fd;
void daemon_log(const char *format, char *error, int errnum) {
    if (log_fd == -1) return;  // Logging not initialized

    time_t now;
    time(&now);
    char timestamp[26];
    ctime_r(&now, timestamp);
    timestamp[24] = '\0';  // Remove newline

    char message[MAX_LOG_LINE - 26];
    //va_list args;
    //va_start(args, format);
    //vsnprintf(message, sizeof(message), format, args);
    if(errnum == 1){
        snprintf(message,  MAX_LOG_LINE, "%s. Error: %s",  format, error);
    }
    else{
        snprintf(message,  MAX_LOG_LINE, "%s",  format);
    }
    
    char log_line[MAX_LOG_LINE];
    snprintf(log_line, MAX_LOG_LINE, "Thread: %d, [%s] %s\n", gettid(), timestamp, message);

    write(log_fd, log_line, strlen(log_line));
    return;
}
int daemon_pid(){
    //we will create a file called TinyFile.pid. If this file exists, our Daemon initialization fails.
    //This ensures that the daemon cannot be started more than once and that we must shut down the daemon after initialization.
    //Additionally, users of the daemon will know which process to send their signals to.
    pid_t curr = getpid();
    FILE* pid_file;
    pid_file = fopen(PID_FILE, "r+");
    if(pid_file == NULL){
        pid_file = fopen(PID_FILE, "w");
        //daemon_log("Failed to create PID file. Exiting...", strerror(errno), 1);
        //return -1;
    }
    int proc;
    fread(&proc, sizeof(int), 1, pid_file);
    //now, read currently running processes
    FILE *fp;
    char line[MAX_PROCESS_LINE];
    fp = popen("ps -e -o pid", "r");
    if (fp == NULL) {
        daemon_log("Failed to get process IDs. Exiting...", strerror(errno), 1);
        return -1;
    }
    //iterate over processes in the system, ensuring that the daemon isn't currently running.
    while (fgets(line, MAX_PROCESS_LINE, fp) != NULL) {
        if (strstr(line, "PID") == NULL) {
            int pid;
            sscanf(line, "%d", &pid);
            if(pid == proc){
                daemon_log("Daemon currently running. Exiting...", "", 0);
                return -1;
            }
        }
    }
    //if we've made it this far, we are the existing daemon and can overwrite the file with our pid. Safely exit after that.
    fseek(pid_file, 0, SEEK_SET);
    fwrite(&curr, sizeof(int), 1, pid_file);
    fwrite(&MQ_NAME, sizeof(char), strlen(MQ_NAME), pid_file);
    fflush(pid_file);
    return 1; 
}
void reset_all_signal_handlers() {
    struct sigaction sa;
    // Initialize the sigaction struct
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    // Iterate through all possible signal numbers
    for (int i = 1; i < NSIG; i++) {
        // Attempt to reset the signal handler
        // Ignore any errors (some signals may not be changeable)
        sigaction(i, &sa, NULL);
    }
    return;
}
void reset_signal_mask() {
    sigset_t mask;
    
    // Initialize the signal set to empty
    sigemptyset(&mask);

    // Set the process's signal mask to the empty set
    if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1) {
        // Handle error (e.g., errno, perror, etc.)
        daemon_log("Failed to reset signal mask: %s", strerror(errno), 1);
    }
    return;
}
void redirect_standard_streams() {
    int fd = open("/dev/null", O_RDWR);
    if (fd == -1) {
        daemon_log("Failed to open /dev/null: %s", strerror(errno),1);
        return;
    }
    if (dup2(fd, STDIN_FILENO) == -1) {
        daemon_log("Failed to redirect stdin: %s", strerror(errno),1);
    }
    if (dup2(fd, STDOUT_FILENO) == -1) {
        daemon_log("Failed to redirect stdout: %s", strerror(errno),1);
    }
    if (dup2(fd, STDERR_FILENO) == -1) {
        daemon_log("Failed to redirect stderr: %s", strerror(errno),1);
    }

    if (close(fd) == -1) {
        daemon_log("Failed to close file descriptor: %s", strerror(errno),1);
    }
    return;
}
extern char* shm_name[MAX_NUM];
extern int num_segs;
extern pthread_t threads[MAX_NUM];
extern int parent;
void cleanup_daemon() {
    //daemon performs a clean-up before it exits.
    //First, close down all children.
    if(gettid() != parent){
        int retval;
        pthread_exit(&retval);
    }
    if (log_fd != -1) {
        daemon_log("Daemon shutting down", "",0);
        close(log_fd);
        log_fd = -1;
    }
    for(int i = 0; i < num_segs; i++){
        daemon_log(shm_name[i], strerror(errno), 1);
        if(shm_unlink(shm_name[i]) == -1){
            daemon_log("Failed to unlink shared memory", strerror(errno), 1);
        }
    }

    if(mq_unlink(MQ_NAME) == -1){
        daemon_log("Failed to unlink message queue", strerror(errno), 1);
    }
    exit(EXIT_SUCCESS);
}
int init_daemon(int enable_logging){
    //step 0: create a logfile for our daemon process. This is necessary as we will close all file descriptors during the initialization process
    if (enable_logging) {
        log_fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (log_fd == -1) {
            // If we can't open the log file, we'll try to write an error to stderr
            fprintf(stderr, "Failed to open log file: %s\n", strerror(errno));
        }
    }
    
    //Step 1: Close all file descriptors except for stdin, stdout, and stderr, and our log file.
    int fdlimit = (int)sysconf(_SC_OPEN_MAX);
    for (int i = STDERR_FILENO + 1; i < fdlimit; i++){
        if(i == log_fd || i == readq) continue;
        close(i);
    }
    

    //step 2: Restore all default file handlers
    reset_all_signal_handlers();

    //step 3: reset the signal mask:
    reset_signal_mask();

    //step 4: fork from parent process.
    pid_t pid;
    pid = fork();
    //fork failed: exit w/ failure
    if (pid < 0)
        exit(EXIT_FAILURE);
    //fork succeeded and parent can exit
    if (pid > 0)
        exit(EXIT_SUCCESS);

    //step 5: create new session and exit on an error:
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    //step 6: fork again??? I'm not sure why this is necessary but oh well.
    /* Fork off for the second time*/
    pid = fork();
    /* An error occurred */
    if (pid < 0)
        exit(EXIT_FAILURE);
    /* Success: Let the parent terminate */
    if (pid > 0)
        exit(EXIT_SUCCESS);

    //step 7: redirect stdin, stdout, and stderr to /dev/null. We will need to attach logging capabilities to debug my daemon.
    redirect_standard_streams();

     //step 8: set new file permissions
    umask(0);

    //step 9: put our pid in a file. This is ensure we are the only daemon and that other processes can find us.
    //Usually we would do this as step 10, but I want this file to be easily accessible for debugging and not in the root dir.
    int res = daemon_pid();
    if(res == -1){
        return -1;
    }
    //step 10: set working directory to root.
    chdir("/");
    
    //step 11: Install signal handlers. Currently, this only includes SIGTERM, which is handled by the cleanup daemon function
    install_signal_handler(SIGTERM , cleanup_daemon);
    return 1;
}