#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <string.h>
#include <errno.h>

// Nombres des tubes nommés
#define TUBE_CLIENT_TO_MASTER "tube_client_to_master"
#define TUBE_MASTER_TO_CLIENT "tube_master_to_client"

// Clés pour les semaphores
#define FTOK_FILE "/tmp/projet_cc2"
#define FTOK_ID_MUTEX 1
#define FTOK_ID_SYNC 2

// Etats du pipeline
#define ORDER_STOP 1
#define ORDER_TEST_PRIME 2
#define ORDER_COUNT_PRIMES 3
#define ORDER_MAX_PRIME 4

#define PRIME 1
#define NOT_PRIME 0

// Structure des messages
typedef struct {
    char type;
    int value;
} Message;

// Variables globales du master
static int dernier_envoye = 2;
static int nombre_premiers_trouves = 1;  // 2 est deja premier
static int plus_grand_premier = 2;

static int creer_semaphore(key_t cle, int valeur_initiale) {
    int semid = semget(cle, 1, IPC_CREAT | IPC_EXCL | 0641);
    if (semid == -1 && errno == EEXIST) {
        semid = semget(cle, 1, 0);
    }
    
    if (semid == -1) {
        perror("semget");
        return -1;
    }
    
    // Si on le cree nous meme, on l'initialise
    if (errno != EEXIST) {
        if (semctl(semid, 0, SETVAL, valeur_initiale) == -1) {
            perror("semctl SETVAL");
            return -1;
        }
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

static void creer_tube_nomme(const char *nom) {
    if (mkfifo(nom, 0644) == -1 && errno != EEXIST) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }
}

static int envoyer_au_worker(int fd, int valeur) {
    ssize_t w = write(fd, &valeur, sizeof(int));
    if (w != sizeof(int)) {
        perror("write to worker");
        return 0;
    }
    return 1;
}

static int lire_du_worker(int fd, Message *msg) {
    ssize_t r = read(fd, msg, sizeof(Message));
    if (r != sizeof(Message)) {
        perror("read from worker");
        return 0;
    }
    return 1;
}

static int est_premier_connu(int n) {
    if (n < 2) return 0;
    
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) return 0;
    }
    
    return 1;
}

int main(void) {
    // Initialisation
    printf("Master starting...\n");
    
    // Creer tubes nommées
    creer_tube_nomme(TUBE_CLIENT_TO_MASTER);
    creer_tube_nomme(TUBE_MASTER_TO_CLIENT);
    
    // Creer semaphores
    key_t cle_mutex = ftok(FTOK_FILE, FTOK_ID_MUTEX);
    key_t cle_sync = ftok(FTOK_FILE, FTOK_ID_SYNC);
    
    if (cle_mutex == -1 || cle_sync == -1) {
        perror("ftok");
        exit(EXIT_FAILURE);
    }
    
    int sem_mutex = creer_semaphore(cle_mutex, 1);  // Mutex inicializado a 1
    int sem_sync = creer_semaphore(cle_sync, 0);    // Sincronización inicializado a 0
    
    if (sem_mutex == -1 || sem_sync == -1) {
        fprintf(stderr, "Failed to create semaphores\n");
        exit(EXIT_FAILURE);
    }
    
    //Cree pipes pour le primer worker
    int pipe_to_worker[2];   // master -> worker
    int pipe_from_worker[2]; // worker -> master
    
    if (pipe(pipe_to_worker) == -1) {
        perror("pipe to_worker");
        exit(EXIT_FAILURE);
    }
    
    if (pipe(pipe_from_worker) == -1) {
        perror("pipe from_worker");
        close(pipe_to_worker[0]);
        close(pipe_to_worker[1]);
        exit(EXIT_FAILURE);
    }
    
    // Lancer le premier worker
    pid_t pid_worker = fork();
    if (pid_worker == -1) {
        perror("fork");
        close(pipe_to_worker[0]);
        close(pipe_to_worker[1]);
        close(pipe_from_worker[0]);
        close(pipe_from_worker[1]);
        exit(EXIT_FAILURE);
    }
    
    if (pid_worker == 0) {
        // Fils: worker
        close(pipe_to_worker[1]);  // Fermer l'ecriture
        close(pipe_from_worker[0]); // Fermer la lecture
        
        // Convertir file descriptors a strings
        char p_str[12], in_str[12], master_str[12];
        snprintf(p_str, sizeof(p_str), "%d", 2);
        snprintf(in_str, sizeof(in_str), "%d", pipe_to_worker[0]);
        snprintf(master_str, sizeof(master_str), "%d", pipe_from_worker[1]);
        
        char *args[] = { "./worker", p_str, in_str, master_str, NULL };
        execvp("./worker", args);
        perror("exec worker");
        exit(EXIT_FAILURE);
    }
    
    // Pere: master
    close(pipe_to_worker[0]);  // Fermer l'ecriture
    close(pipe_from_worker[1]); // Fermer la lecture
    
    int fd_write_worker = pipe_to_worker[1];
    int fd_read_master = pipe_from_worker[0];
    
    // Envoyer le 2 au pipeline pour l'initialisation
    if (!envoyer_au_worker(fd_write_worker, 2)) {
        fprintf(stderr, "Failed to send initial value to worker\n");
        close(fd_write_worker);
        close(fd_read_master);
        waitpid(pid_worker, NULL, 0);
        exit(EXIT_FAILURE);
    }
    
    // Boucle principale
    while (1) {
        printf("\nMaster waiting for client...\n");
        
        // Synchronisation: attendre que le client finise (P(sync))
        operation_semaphore(sem_sync, -1);
        
        // Ouvrir communication avec le client
        int fd_from_client = open(TUBE_CLIENT_TO_MASTER, O_RDONLY);
        if (fd_from_client == -1) {
            perror("open TUBE_CLIENT_TO_MASTER");
            operation_semaphore(sem_mutex, 1);
            continue;
        }
        
        int fd_to_client = open(TUBE_MASTER_TO_CLIENT, O_WRONLY);
        if (fd_to_client == -1) {
            perror("open TUBE_MASTER_TO_CLIENT");
            close(fd_from_client);
            operation_semaphore(sem_mutex, 1);
            continue;
        }
        
        // Lire ordre du client
        int ordre[2];
        ssize_t r = read(fd_from_client, ordre, 2 * sizeof(int));
        if (r != 2 * sizeof(int)) {
            perror("read from client");
            close(fd_from_client);
            close(fd_to_client);
            operation_semaphore(sem_mutex, 1);
            continue;
        }
        
        int commande = ordre[0];
        int nombre = ordre[1];
        
        printf("Master received order %d (number=%d)\n", commande, nombre);
        
        if (commande == ORDER_STOP) {
            // Ordre d'arrete
            if (!envoyer_au_worker(fd_write_worker, -1)) {
                fprintf(stderr, "Failed to send stop signal\n");
            }
            
            waitpid(pid_worker, NULL, 0);
            
            // Envoyer un accuse de reception
            int ack = 1;
            ssize_t w = write(fd_to_client, &ack, sizeof(int));
            if (w != sizeof(int)) {
                perror("write ack");
            }
            
            close(fd_from_client);
            close(fd_to_client);
            break;
            
        } else if (commande == ORDER_TEST_PRIME) {
            // Remplir pipeline avec nombres intermediaires
            for (int i = dernier_envoye + 1; i < nombre; i++) {
                if (!envoyer_au_worker(fd_write_worker, i)) {
                    fprintf(stderr, "Failed to send number %d to worker\n", i);
                    break;
                }
                
                // Lire la reponse du pipeline
                Message msg;
                if (!lire_du_worker(fd_read_master, &msg)) {
                    fprintf(stderr, "Failed to read response for %d\n", i);
                    break;
                }
                
                if (msg.type == PRIME) {
                    nombre_premiers_trouves++;
                    if (msg.value > plus_grand_premier) {
                        plus_grand_premier = msg.value;
                    }
                }
            }
            
            //Envoyer le nombre demandé
            if (!envoyer_au_worker(fd_write_worker, nombre)) {
                fprintf(stderr, "Failed to send requested number\n");
                close(fd_from_client);
                close(fd_to_client);
                operation_semaphore(sem_mutex, 1);
                continue;
            }
            
            // Lire reponse
            Message msg;
            if (!lire_du_worker(fd_read_master, &msg)) {
                fprintf(stderr, "Failed to read final response\n");
                close(fd_from_client);
                close(fd_to_client);
                operation_semaphore(sem_mutex, 1);
                continue;
            }
            
            int resultat = (msg.type == PRIME) ? 1 : 0;
            
            if (resultat == 1) {
                nombre_premiers_trouves++;
                if (msg.value > plus_grand_premier) {
                    plus_grand_premier = msg.value;
                }
            }
            
            // Envoyer la reponse au client
            ssize_t w = write(fd_to_client, &resultat, sizeof(int));
            if (w != sizeof(int)) {
                perror("write result to client");
            }
            
            dernier_envoye = nombre;
            
        } else if (commande == ORDER_COUNT_PRIMES) {
            //Envoyer le compteur directement
            ssize_t w = write(fd_to_client, &nombre_premiers_trouves, sizeof(int));
            if (w != sizeof(int)) {
                perror("write count to client");
            }
            
        } else if (commande == ORDER_MAX_PRIME) {
            //Envoyer prime max directement
            ssize_t w = write(fd_to_client, &plus_grand_premier, sizeof(int));
            if (w != sizeof(int)) {
                perror("write max prime to client");
            }
        } else {
            fprintf(stderr, "Unknown command: %d\n", commande);
            int error = -1;
            write(fd_to_client, &error, sizeof(int));
        }
        
        // Fermer comunication avec ce client
        if (close(fd_from_client) == -1) {
            perror("close fd_from_client");
        }
        if (close(fd_to_client) == -1) {
            perror("close fd_to_client");
        }
    }
    
    // Nettoyage
    printf("Master cleaning up...\n");
    
    // Fermer les pipes
    if (close(fd_write_worker) == -1) {
        perror("close fd_write_worker");
    }
    if (close(fd_read_master) == -1) {
        perror("close fd_read_master");
    }
    
    // Detruire semaphores
    if (semctl(sem_mutex, 0, IPC_RMID) == -1) {
        perror("semctl RMID mutex");
    }
    if (semctl(sem_sync, 0, IPC_RMID) == -1) {
        perror("semctl RMID sync");
    }
    
    // Eliminer les tubes nommés
    if (unlink(TUBE_CLIENT_TO_MASTER) == -1) {
        perror("unlink TUBE_CLIENT_TO_MASTER");
    }
    if (unlink(TUBE_MASTER_TO_CLIENT) == -1) {
        perror("unlink TUBE_MASTER_TO_CLIENT");
    }
    
    printf("Master exiting.\n");
    return EXIT_SUCCESS;
}