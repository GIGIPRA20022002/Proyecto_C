#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <assert.h>
#include <string.h>

// Nombres des tubes nommés (crées par le master)
#define TUBE_CLIENT_TO_MASTER "tube_client_to_master"
#define TUBE_MASTER_TO_CLIENT "tube_master_to_client"

//Semaphores
#define FTOK_FILE "."
#define FTOK_ID_MUTEX 1
#define FTOK_ID_SYNC 2

// Ordres posibles
#define ORDER_STOP 1
#define ORDER_TEST_PRIME 2
#define ORDER_COUNT_PRIMES 3
#define ORDER_MAX_PRIME 4

static int obtenir_semaphore(key_t cle) {
    int semid = semget(cle, 1, 0);
    if (semid == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }
    return semid;
}

static void operation_semaphore(int semid, int op) {
    struct sembuf operation = {0, op, 0};
    if (semop(semid, &operation, 1) == -1) {
        perror("semop");
        exit(EXIT_FAILURE);
    }
}

static int parse_int(const char *str, int *result) {
    char *endptr;
    long val = strtol(str, &endptr, 10);
    
    if (endptr == str || *endptr != '\0') {
        return 0; // Nombre pas valide
    }
    
    // Verifier debordement
    if (val < -2147483648 || val > 2147483647) {
        return 0;
    }
    
    *result = (int)val;
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <order> [number]\n", argv[0]);
        fprintf(stderr, "Orders: 1=stop, 2=test_prime, 3=count_primes, 4=max_prime\n");
        return EXIT_FAILURE;
    }
    
    int order = 0;
    int number = 0;
    
    // Analyser la commande en toute sécurité
    if (!parse_int(argv[1], &order)) {
        fprintf(stderr, "Invalid order: must be an integer\n");
        return EXIT_FAILURE;
    }
    
    // Valider l'ordre
    if (order < ORDER_STOP || order > ORDER_MAX_PRIME) {
        fprintf(stderr, "Invalid order: must be between 1 and 4\n");
        return EXIT_FAILURE;
    }
    
    // Gerer les different ordres
    if (order == ORDER_TEST_PRIME) {
        if (argc < 3) {
            fprintf(stderr, "Need a number to test for order 2\n");
            return EXIT_FAILURE;
        }
        if (!parse_int(argv[2], &number)) {
            fprintf(stderr, "Invalid number: must be an integer\n");
            return EXIT_FAILURE;
        }
        if (number < 2) {
            fprintf(stderr, "Number must be >= 2\n");
            return EXIT_FAILURE;
        }
    } else if (argc > 2) {
        fprintf(stderr, "Too many arguments for this order\n");
        return EXIT_FAILURE;
    }
    
    // Obtenir semaphores
    key_t cle_mutex = ftok(FTOK_FILE, FTOK_ID_MUTEX);
    key_t cle_sync = ftok(FTOK_FILE, FTOK_ID_SYNC);
    
    if (cle_mutex == -1 || cle_sync == -1) {
        perror("ftok");
        return EXIT_FAILURE;
    }
    
    int sem_mutex = obtenir_semaphore(cle_mutex);
    int sem_sync = obtenir_semaphore(cle_sync);
    
    // Selection critique
    operation_semaphore(sem_mutex, -1);
    
    // Ouvrir tubes nommés
    int fd_to_master = open(TUBE_CLIENT_TO_MASTER, O_WRONLY);
    if (fd_to_master == -1) {
        perror("open TUBE_CLIENT_TO_MASTER");
        operation_semaphore(sem_mutex, 1);
        return EXIT_FAILURE;
    }
    
    int fd_from_master = open(TUBE_MASTER_TO_CLIENT, O_RDONLY);
    if (fd_from_master == -1) {
        perror("open TUBE_MASTER_TO_CLIENT");
        close(fd_to_master);
        operation_semaphore(sem_mutex, 1);
        return EXIT_FAILURE;
    }
    
    // Envoyer une ordre
    int buf_envoi[2] = {order, number};
    ssize_t w = write(fd_to_master, buf_envoi, 2 * sizeof(int));
    if (w != 2 * sizeof(int)) {
        perror("write");
        close(fd_to_master);
        close(fd_from_master);
        operation_semaphore(sem_mutex, 1);
        return EXIT_FAILURE;
    }
    
    // Recevoir une reponse
    int reponse;
    ssize_t r = read(fd_from_master, &reponse, sizeof(int));
    if (r != sizeof(int)) {
        perror("read");
        close(fd_to_master);
        close(fd_from_master);
        operation_semaphore(sem_mutex, 1);
        return EXIT_FAILURE;
    }
    
    // Montrer le resultat
    switch (order) {
        case ORDER_TEST_PRIME:
            printf("Number %d is %s\n", number, 
                   (reponse == 1) ? "PRIME" : "NOT PRIME");
            break;
        case ORDER_COUNT_PRIMES:
            printf("Number of primes found: %d\n", reponse);
            break;
        case ORDER_MAX_PRIME:
            printf("Maximum prime found: %d\n", reponse);
            break;
        case ORDER_STOP:
            printf("Master stop acknowledged\n");
            break;
        default:
            fprintf(stderr, "Unknown order\n");
            break;
    }
    
    // Fermer les tubes
    if (close(fd_to_master) == -1) {
        perror("close fd_to_master");
    }
    if (close(fd_from_master) == -1) {
        perror("close fd_from_master");
    }
    
    // Fin section critique
    operation_semaphore(sem_mutex, 1);
    
    // Debloquer le master
    operation_semaphore(sem_sync, 1);
    
    return EXIT_SUCCESS;
}