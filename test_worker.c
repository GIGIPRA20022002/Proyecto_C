#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>

#define PRIME 1
#define NOT_PRIME 2

int main(int argc, char *argv[]) {
    /* Verificar argumentos: P fd_in fd_master */
    if (argc < 4) {
        fprintf(stderr, "usage: worker P fd_in fd_master\n");
        return 1;
    }

    int P = atoi(argv[1]);       /* Número primo que gestiona este worker */
    int fd_in = atoi(argv[2]);   /* Tubería para recibir números */
    int fd_master = atoi(argv[3]); /* Tubería para enviar resultados al master */

    int fd_next = -1;    /* Tubería hacia el siguiente worker */
    int pid_next = -1;   /* PID del siguiente worker */

    int n;
    while (1) {
        /* Leer número N de la tubería de entrada */
        int r = read(fd_in, &n, sizeof(int));
        assert(r == sizeof(int));
        if (r <= 0) break;

        /* Si es orden de STOP */
        if (n == -1) {
            /* Reenviar STOP al siguiente worker si existe */
            if (fd_next != -1) {
                int w = write(fd_next, &n, sizeof(int));
                assert(w == sizeof(int));
                waitpid(pid_next, NULL, 0);  /* Esperar que termine */
            }
            break;
        }

        /* Si N es igual al primo P que gestiono */
        if (n == P) {
            char mensaje[5];
            mensaje[0] = PRIME;
            *(int*)(mensaje + 1) = n;
            int w = write(fd_master, mensaje, 5);
            assert(w == 5);
        }
        /* Si N es divisible por P */
        else if (n % P == 0) {
            char mensaje[5];
            mensaje[0] = NOT_PRIME;
            *(int*)(mensaje + 1) = n;
            int w = write(fd_master, mensaje, 5);
            assert(w == 5);
        }
        /* Si no es divisible y no es igual */
        else {
            if (fd_next != -1) {
                /* Ya existe siguiente worker: pasarle N */
                int w = write(fd_next, &n, sizeof(int));
                assert(w == sizeof(int));
            } else {
                /* Crear nuevo worker para N */
                int newpipe[2];
                int p = pipe(newpipe);
                assert(p == 0);

                pid_t pid = fork();
                assert(pid != -1);
                
                if (pid == 0) {
                    /* Proceso hijo: nuevo worker */
                    close(newpipe[1]);  /* Cerrar extremo de escritura */
                    
                    char p_str[12], in_str[12], master_str[12];
                    sprintf(p_str, "%d", n);      /* El nuevo primo es N */
                    sprintf(in_str, "%d", newpipe[0]); /* Leerá de aquí */
                    sprintf(master_str, "%d", fd_master); /* Mismo master */

                    char *args[] = { "worker", p_str, in_str, master_str, NULL };
                    execvp("./worker", args);
                    perror("exec worker");
                    exit(1);
                } else {
                    /* Proceso padre: worker actual */
                    close(newpipe[0]);  /* Cerrar extremo de lectura */
                    fd_next = newpipe[1];  /* Guardar para escribir al nuevo */
                    pid_next = pid;        /* Guardar PID del hijo */
                    
                    /* Enviar N al nuevo worker */
                    int w = write(fd_next, &n, sizeof(int));
                    assert(w == sizeof(int));
                }
            }
        }
    }

    /* Limpieza final */
    if (fd_next != -1) {
        close(fd_next);
        waitpid(pid_next, NULL, 0);  /* Asegurar que no quede zombie */
    }

    close(fd_in);
    close(fd_master);
    return 0;
}