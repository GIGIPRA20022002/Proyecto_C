#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define PRIME 1
#define NOT_PRIME 2

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "usage: worker P fd_in fd_master\n");
        return 1;
    }

    int P = atoi(argv[1]);
    int fd_in = atoi(argv[2]);
    int fd_master = atoi(argv[3]);

    int fd_next = -1;
    int pid_next = -1;

    int n;
    while (1) {
        int r = read(fd_in, &n, sizeof(int));
        if (r <= 0)
            break;

        if (n == -1) {
            if (fd_next != -1) {
                write(fd_next, &n, sizeof(int));
                waitpid(pid_next, NULL, 0);
            }
            break;
        }

        if (n == P) {
            char mensaje[5];
            mensaje[0] = PRIME;
            *(int*)(mensaje + 1) = n;
            write(fd_master, mensaje, 5);
        }
        else if (n % P == 0) {
            char mensaje[5];
            mensaje[0] = NOT_PRIME;
            *(int*)(mensaje + 1) = n;
            write(fd_master, mensaje, 5);
        }
        else {
            if (fd_next != -1) {
                write(fd_next, &n, sizeof(int));
            } else {
                int newpipe[2];
                pipe(newpipe);

                pid_t pid = fork();
                if (pid == 0) {
                    close(newpipe[1]);
                    
                    char p_str[12], in_str[12], master_str[12];
                    sprintf(p_str, "%d", n);
                    sprintf(in_str, "%d", newpipe[0]);
                    sprintf(master_str, "%d", fd_master);

                    char *args[] = { "worker", p_str, in_str, master_str, NULL };
                    execvp("./worker", args);
                    exit(1);
                } else {
                    close(newpipe[0]);
                    fd_next = newpipe[1];
                    pid_next = pid;
                    write(fd_next, &n, sizeof(int));
                }
            }
        }
    }

    if (fd_next != -1) {
        close(fd_next);
    }
    return 0;
}