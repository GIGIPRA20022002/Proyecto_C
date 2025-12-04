#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>

typedef struct {
    int diviseur;
    int N;
    int *tableau;
    pthread_mutex_t *mutex;
} ThreadData;

// Fonction pour calculer la raccine carré
static int int_sqrt(int n) {
    if (n <= 1) return n;
    
    int x = n;
    int y = 1;
    
    while (x > y) {
        x = (x + y) / 2;
        y = n / x;
    }
    
    return x;
}

static void* marquer_multiples(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    int i = data->diviseur;
    int N = data->N;
    int *tableau = data->tableau;
    pthread_mutex_t *mutex = data->mutex;
    
    // Signaler multiples du diviseur i
    for (int multiple = 2 * i; multiple <= N; multiple += i) {
        if (pthread_mutex_lock(mutex) != 0) {
            perror("pthread_mutex_lock");
            free(data);
            return NULL;
        }
        
        tableau[multiple] = 0;  // 0 = non prime
        
        if (pthread_mutex_unlock(mutex) != 0) {
            perror("pthread_mutex_unlock");
            free(data);
            return NULL;
        }
    }
    
    free(data);
    return NULL;
}

static int parse_int(const char *str, int *result) {
    char *endptr;
    long val = strtol(str, &endptr, 10);
    
    if (endptr == str || *endptr != '\0') {
        return 0;
    }
    
    if (val < 2 || val > 1000000) {  // Límite razonable
        return 0;
    }
    
    *result = (int)val;
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <N>\n", argv[0]);
        fprintf(stderr, "N must be >= 2\n");
        return EXIT_FAILURE;
    }
    
    int N = 0;
    if (!parse_int(argv[1], &N)) {
        fprintf(stderr, "Invalid N: must be an integer >= 2\n");
        return EXIT_FAILURE;
    }
    
    if (N < 2) {
        fprintf(stderr, "N must be >= 2\n");
        return EXIT_FAILURE;
    }
    
    // Creer et initialiser le tableau (0 = non prime, 1 = prime)
    int *tableau = malloc((N + 1) * sizeof(int));
    if (tableau == NULL) {
        perror("malloc tableau");
        return EXIT_FAILURE;
    }
    
    // initialiser: tous prime initialement (dehors 0 y 1)
    for (int i = 0; i <= N; i++) {
        tableau[i] = 1;
    }
    tableau[0] = 0;
    tableau[1] = 0;
    
    // Creer mutex
    pthread_mutex_t mutex;
    int ret = pthread_mutex_init(&mutex, NULL);
    if (ret != 0) {
        perror("pthread_mutex_init");
        free(tableau);
        return EXIT_FAILURE;
    }
    
    // Calculer sqrt(N)
    int sqrt_N = int_sqrt(N);
    
    // Compter nombre de threads dont on a besoin (nombres primes jusqu'a sqrt_N)
    int nb_threads = 0;
    for (int i = 2; i <= sqrt_N; i++) {
        if (tableau[i] == 1) {
            nb_threads++;
        }
    }
    
    if (nb_threads == 0) {
        nb_threads = 1;  // Au moins un thread
    }
    
    pthread_t *threads = malloc(nb_threads * sizeof(pthread_t));
    if (threads == NULL) {
        perror("malloc threads");
        pthread_mutex_destroy(&mutex);
        free(tableau);
        return EXIT_FAILURE;
    }
    
    // Lancer threads
    int thread_idx = 0;
    for (int i = 2; i <= sqrt_N; i++) {
        if (tableau[i] == 1) {  // Seulement s'il est prime
            ThreadData *data = malloc(sizeof(ThreadData));
            if (data == NULL) {
                perror("malloc ThreadData");
                // Free les threads deja creé
                for (int j = 0; j < thread_idx; j++) {
                    pthread_join(threads[j], NULL);
                }
                free(threads);
                pthread_mutex_destroy(&mutex);
                free(tableau);
                return EXIT_FAILURE;
            }
            
            data->diviseur = i;
            data->N = N;
            data->tableau = tableau;
            data->mutex = &mutex;
            
            ret = pthread_create(&threads[thread_idx], NULL, 
                                marquer_multiples, data);
            if (ret != 0) {
                perror("pthread_create");
                free(data);
                // Free les threads deja creé
                for (int j = 0; j < thread_idx; j++) {
                    pthread_join(threads[j], NULL);
                }
                free(threads);
                pthread_mutex_destroy(&mutex);
                free(tableau);
                return EXIT_FAILURE;
            }
            
            thread_idx++;
        }
    }
    
    //Attendre threads
    for (int i = 0; i < thread_idx; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join");
        }
    }
    
    //Montrer les resultat
    printf("Prime numbers up to %d:\n", N);
    int count = 0;
    int line_count = 0;
    
    for (int i = 2; i <= N; i++) {
        if (tableau[i] == 1) {
            printf("%d ", i);
            count++;
            line_count++;
            
            if (line_count % 10 == 0) {
                printf("\n");
                line_count = 0;
            }
        }
    }
    
    if (line_count != 0) {
        printf("\n");
    }
    
    printf("\nTotal: %d primes\n", count);
    
    // Liberer memoire
    free(threads);
    
    if (pthread_mutex_destroy(&mutex) != 0) {
        perror("pthread_mutex_destroy");
    }
    
    free(tableau);
    
    return EXIT_SUCCESS;
}