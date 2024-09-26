#include <signal.h>
#include <string.h>

#define MAX_LOG_LINE 1024
#define MAX_PROCESS_LINE 2048

#define LOG_FILE "../TinyFile.log"
#define PID_FILE "../TinyFile.pid"
int log_fd;
//void reset_all_signal_handlers();
//void reset_signal_mask();
//void redirect_output();
void daemon_log();
int init_daemon(int enable_logging);
void cleanup_daemon();