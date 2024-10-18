
#define MAX_LOG_LINE 1024
#define MAX_PROCESS_LINE 2048

#define LOG_FILE "../TinyFile.log"
#define PID_FILE "../TinyFile.pid"
#define MQ_NAME  "/TinyFile"

#define MAX_CLIENTS 1024
#define STACK_SIZE 4096
#define MAX_SIZE 10000
#define MAX_NUM 20

struct thread_arg{
    int id;
};
struct clients{
    int client_qs[MAX_CLIENTS][2];
    int clients;
    pthread_mutex_t lock; 
};
int log_fd;
int readq;
//void reset_all_signal_handlers();
//void reset_signal_mask();
//void redirect_output();
void daemon_log();
int init_daemon(int enable_logging);
void cleanup_daemon();