#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

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
        
        int numeros[] = {3, 5, 7, 9, 11, -1};
        
        for (int i = 0; i < 6; i++) {
            int x = numeros[i];
            printf("Enviando: %d\n", x);
            write(tuberia[1], &x, sizeof(int));
            
            if (x != -1) {
                int respuesta;
                read(master_pipe[0], &respuesta, sizeof(int));
                printf("Respuesta: %d -> %s\n", x, respuesta ? "prime" : "not prime");
            }
        }
        
        close(tuberia[1]);
        close(master_pipe[0]);
        wait(NULL);
        printf("Pipeline terminado ordenadamente\n");
    }
    
    return 0;
}