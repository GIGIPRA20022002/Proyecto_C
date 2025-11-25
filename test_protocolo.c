#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

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
        
        int numeros[] = {2, 4, 3, -1};
        
        for (int i = 0; i < 4; i++) {
            int x = numeros[i];
            printf("Enviando: %d\n", x);
            write(tuberia[1], &x, sizeof(int));
            
            if (x != -1) {
                char respuesta[5];
                read(master_pipe[0], respuesta, 5);
                
                int tipo = respuesta[0];
                int valor = *(int*)(respuesta + 1);
                
                printf("Respuesta: tipo=%d, valor=%d -> ", tipo, valor);
                if (tipo == PRIME) printf("PRIME\n");
                else if (tipo == NOT_PRIME) printf("NOT_PRIME\n");
            }
        }
        
        close(tuberia[1]);
        close(master_pipe[0]);
        wait(NULL);
        printf("Test protocolo completado\n");
    }
    
    return 0;
}