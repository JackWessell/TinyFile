//basic signal handling functionality.
void install_signal_handler(int signo, void(*handler)());
void block_signal(int signo);
void unblock_signal(int signo);