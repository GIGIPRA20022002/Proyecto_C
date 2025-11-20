#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void test(int P, int valores[], int n) {
    int p1[2], p2[2];
    pipe(p1);
    pipe(p2);

    pid_t pid = fork();

    if (pid == 0) {
        close(p1[1]);
        close(p2[0]);

        char p_str[10], in_str[10], out_str[10];
        sprintf(p_str, "%d", P);
        sprintf(in_str, "%d", p1[0]);
        sprintf(out_str, "%d", p2[1]);

        char *args[] = {"worker", p_str, in_str, out_str, NULL};
        execvp("./worker", args);
        perror("exec worker");
        exit(1);

    } else {
        close(p1[0]);
        close(p2[1]);

        for (int i = 0; i < n; i++) {
            int x = valores[i];
            write(p1[1], &x, sizeof(int));

            if (x == -1) break;

            if (x == P || x % P == 0) {
                int r;
                read(p2[0], &r, sizeof(int));
                printf("P=%d, N=%d => %s\n", P, x, r ? "PRIMO" : "NO PRIMO");
            }
        }

        close(p1[1]);
        close(p2[0]);
        wait(NULL);
    }
}

int main() {
    int a[] = {2, 4, 3, -1};
    test(2, a, 4);

    int b[] = {3, 5, 9, -1};
    test(3, b, 4);

    return 0;
}
