#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
int status;
int main(int argc, char *argv[]){
    if(argc != 3){
        printf("Usage: ./TinyTest -n NUM_CLIENTS");
        return -1;
    }
    int num = atoi(argv[2]);
    printf("%d\n", num);
    char arg[32];
    for(int i = 30; i < 30 + num; i++){
        sprintf(arg, "/TinyClient%d", i);
        char *args[] = {"../bin/TinyClient",  "-n", arg, "--files", "../input/input_mixed.txt", "--num", "20", NULL};
        int pid = fork();
        if(pid == 0){
            continue;
        }
        else{
            int res = execv(args[0], args);
            if(res == -1){
                perror("");
            }
            return 0;
        }
    }
    int wpid;
    while ((wpid = wait(&status)) > 0);
    return 0;
}