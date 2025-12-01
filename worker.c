#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>

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
        assert(r == sizeof(int));
        if (r <= 0) break;

        if (n == -1) {
            if (fd_next != -1) {
                int w = write(fd_next, &n, sizeof(int));
                assert(w == sizeof(int));
                waitpid(pid_next, NULL, 0);
            }
            break;
        }

        if (n == P) {
            char mensaje[5];
            mensaje[0] = PRIME;
            *(int*)(mensaje + 1) = n;
            int w = write(fd_master, mensaje, 5);
            assert(w == 5);
        }
        else if (n % P == 0) {
            char mensaje[5];
            mensaje[0] = NOT_PRIME;
            *(int*)(mensaje + 1) = n;
            int w = write(fd_master, mensaje, 5);
            assert(w == 5);
        }
        else {
            if (fd_next != -1) {
                int w = write(fd_next, &n, sizeof(int));
                assert(w == sizeof(int));
            } else {
                int newpipe[2];
                int p = pipe(newpipe);
                assert(p == 0);

                pid_t pid = fork();
                assert(pid != -1);
                
                if (pid == 0) {
                    close(newpipe[1]);
                    
                    char p_str[12], in_str[12], master_str[12];
                    sprintf(p_str, "%d", n);
                    sprintf(in_str, "%d", newpipe[0]);
                    sprintf(master_str, "%d", fd_master);

                    char *args[] = { "worker", p_str, in_str, master_str, NULL };
                    execvp("./worker", args);
                    perror("exec worker");
                    exit(1);
                } else {
                    close(newpipe[0]);
                    fd_next = newpipe[1];
                    pid_next = pid;
                    int w = write(fd_next, &n, sizeof(int));
                    assert(w == sizeof(int));
                }
            }
        }
    }

    if (fd_next != -1) {
        close(fd_next);
        waitpid(pid_next, NULL, 0);
    }

    close(fd_in);
    close(fd_master);
    return 0;
}