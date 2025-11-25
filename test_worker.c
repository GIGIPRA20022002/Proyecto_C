#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

static void test(int P, int vals[], int n) {
    int t1[2], t2[2];
    pipe(t1);
    pipe(t2);

    pid_t pid = fork();

    if (pid == 0) {
        close(t1[1]);
        close(t2[0]);

        char p_str[12], in_str[12], out_str[12];
        sprintf(p_str, "%d", P);
        sprintf(in_str, "%d", t1[0]);
        sprintf(out_str, "%d", t2[1]);

        char *args[] = { "worker", p_str, in_str, out_str, NULL };
        execvp("./worker", args);
        perror("exec worker");
        exit(1);

    } else {
        close(t1[0]);
        close(t2[1]);

        for (int i = 0; i < n; i++) {
            int x = vals[i];
            write(t1[1], &x, sizeof(int));

            if (x == -1)
                break;

            /* leer respuesta si la hay */
            if (x == P || x % P == 0) {
                int r;
                read(t2[0], &r, sizeof(int));
                printf("P=%d, N=%d -> %s\n", P, x, r ? "prime" : "not prime");
            } else {
                printf("P=%d, N=%d -> pasó al siguiente\n", P, x);
            }
        }

        close(t1[1]);
        close(t2[0]);
        wait(NULL);
    }
}

int main() {
    printf("=== Test 1: Worker P=2 ===\n");
    int a[] = {2, 4, 3, 5, -1};
    test(2, a, 5);

    printf("\n=== Test 2: Worker P=3 ===\n");
    int b[] = {3, 6, 5, 7, -1};
    test(3, b, 5);

    return 0;
}