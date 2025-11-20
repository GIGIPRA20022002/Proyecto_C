#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>  // <-- AÑADIR ESTA LÍNEA PARA wait()

int main() {
    int tuberia[2];
    
    if (pipe(tuberia) == -1) {
        printf("Error creando tubería\n");
        exit(1);
    }
    
    printf("Tubería creada: lectura=%d, escritura=%d\n", tuberia[0], tuberia[1]);
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // Proceso HIJO (worker)
        close(tuberia[1]);  // Cerrar extremo de escritura
        
        char p_str[10], fd_in_str[10], fd_master_str[10];
        sprintf(p_str, "%d", 2);
        sprintf(fd_in_str, "%d", tuberia[0]);
        sprintf(fd_master_str, "%d", 1);
        
        execl("./worker", "worker", p_str, fd_in_str, fd_master_str, NULL);
        printf("Error ejecutando worker\n");
        exit(1);
        
    } else {
        // Proceso PADRE
        close(tuberia[0]);  // Cerrar extremo de lectura
        
        int numeros[] = {5, 10, 15, -1};
        for (int i = 0; i < 4; i++) {
            printf("Enviando número: %d\n", numeros[i]);
            write(tuberia[1], &numeros[i], sizeof(numeros[i]));
            sleep(1);
        }
        
        close(tuberia[1]);  // Cerrar tubería para que el worker detecte fin de datos
        
        // ESPERAR A QUE EL WORKER TERMINE
        wait(NULL);  // <-- ESTA LÍNEA FALTABA
        printf("Test completado - Worker terminó correctamente\n");
    }
    
    return 0;
}