#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>
#include <time.h>

#define PRIME 1
#define NOT_PRIME 2

int main() {
    int tuberia[2], master_pipe[2];
    pipe(tuberia);
    pipe(master_pipe);

    pid_t pid = fork();
    
    if (pid == 0) {
        close(tuberia[1]);
        close(master_pipe[0]);
        
        char p_str[12], in_str[12], master_str[12];
        sprintf(p_str, "%d", 2);
        sprintf(in_str, "%d", tuberia[0]);
        sprintf(master_str, "%d", master_pipe[1]);
        
        char *args[] = { "worker", p_str, in_str, master_str, NULL };
        execvp("./worker", args);
        exit(1);
        
    } else {
        close(tuberia[0]);
        close(master_pipe[1]);
        
        int limite = 500;
        int primos = 0;
        
        clock_t inicio = clock();
        
        for (int x = 2; x <= limite; x++) {
            write(tuberia[1], &x, sizeof(int));
            
            char respuesta[5];
            read(master_pipe[0], respuesta, 5);
            
            if (respuesta[0] == PRIME) {
                primos++;
            }
        }
        
        int stop = -1;
        write(tuberia[1], &stop, sizeof(int));
        
        close(tuberia[1]);
        close(master_pipe[0]);
        wait(NULL);
        
        clock_t fin = clock();
        double tiempo = (double)(fin - inicio) / CLOCKS_PER_SEC;
        
        printf("Primos hasta %d: %d\n", limite, primos);
        printf("Tiempo: %.3f segundos\n", tiempo);
    }
    
    return 0;
}
