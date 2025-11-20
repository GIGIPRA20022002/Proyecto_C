#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "uso: worker P fd_in fd_master\n");
        return 1;
    }

    int P = atoi(argv[1]);
    int fd_in = atoi(argv[2]);
    int fd_master = atoi(argv[3]);

    int n;
    while (1) {
        int r = read(fd_in, &n, sizeof(int));
        if (r <= 0) break;

        if (n == -1) {
            break;
        }

        if (n == P) {
            int x = 1;
            write(fd_master, &x, sizeof(int));
        } else if (n % P == 0) {
            int x = 0;
            write(fd_master, &x, sizeof(int));
        } else {
            /* todavía no hay worker siguiente en esta versión */
        }
    }

    return 0;
}
