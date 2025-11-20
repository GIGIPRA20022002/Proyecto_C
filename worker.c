#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Error: Uso: ./worker P fd_in fd_master\n");
        exit(1);
    }
    
    int P = atoi(argv[1]);
    int fd_in = atoi(argv[2]);
    int fd_master = atoi(argv[3]);
    
    printf("Worker P=%d iniciado (fd_in=%d, fd_master=%d)\n", P, fd_in, fd_master);
    
    while (1) {
        int numero;
        int bytes_leidos = read(fd_in, &numero, sizeof(numero));
        
        // Verificar si la tubería se cerró o error
        if (bytes_leidos == 0) {
            printf("Worker P=%d: Tubería cerrada - terminando\n", P);
            break;
        }
        
        if (bytes_leidos != sizeof(numero)) {
            printf("Worker P=%d: Error leyendo tubería\n", P);
            break;
        }
        
        printf("Worker P=%d recibió N=%d\n", P, numero);
        
        if (numero == -1) {
            printf("Worker P=%d recibió orden STOP\n", P);
            break;
        }
        
        printf("  - Procesando número %d...\n", numero);
    }
    
    printf("Worker P=%d terminado\n", P);
    return 0;
}