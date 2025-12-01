#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <assert.h>

#define PRIME 1
#define NOT_PRIME 2

int main() {
    int tuberia[2], master_pipe[2];
    int p1 = pipe(tuberia);
    int p2 = pipe(master_pipe);
    assert(p1 == 0);
    assert(p2 == 0);

    pid_t pid = fork();
    assert(pid != -1);
    
    if (pid == 0) {
        close(tuberia[1]);
        close(master_pipe[0]);
        
        char p_str[12], in_str[12], master_str[12];
        sprintf(p_str, "%d", 2);
        sprintf(in_str, "%d", tuberia[0]);
        sprintf(master_str, "%d", master_pipe[1]);
        
        char *args[] = { "worker", p_str, in_str, master_str, NULL };
        execvp("./worker", args);
        perror("exec worker");
        exit(1);
        
    } else {
        close(tuberia[0]);
        close(master_pipe[1]);
        
        int numeros[] = {2, 4, -1};
        
        for (int i = 0; i < 3; i++) {
            int x = numeros[i];
            printf("Enviando: %d\n", x);
            int w = write(tuberia[1], &x, sizeof(int));
            assert(w == sizeof(int));
            
            if (x != -1) {
                char respuesta[5];
                int r = read(master_pipe[0], respuesta, 5);
                assert(r == 5);
                
                int tipo = respuesta[0];
                int valor = *(int*)(respuesta + 1);
                
                printf("Respuesta: tipo=%d, valor=%d\n", tipo, valor);
            }
        }
        
        close(tuberia[1]);
        close(master_pipe[0]);
        wait(NULL);
        printf("Test errores completado\n");
    }
    
    return 0;
}