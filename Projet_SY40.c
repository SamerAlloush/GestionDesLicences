#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <time.h>

#define TEMPS_UTILISATION 5 // Time in seconds a client holds a license

int nb_licences, nb_clients;

typedef struct {
  pid_t pid;
  int num_client;
  int jeton;
  int in_use;
  int desire;
} ClientInfo;

ClientInfo *clients;
int *available_jetons;
pthread_mutex_t mutex; // Lock for accessing available_jetons

void init_clients() {
    clients = (ClientInfo *)malloc(nb_clients * sizeof(ClientInfo));
    available_jetons = (int *)malloc(nb_licences * sizeof(int));

    for (int i = 0; i < nb_clients; i++) {
        clients[i].pid = getpid();
        clients[i].num_client = i + 1;
        clients[i].jeton = 0;
        clients[i].in_use = 0;
        clients[i].desire = 0;
        available_jetons[i] = 0;
        if (i < nb_licences) {
            available_jetons[i] = 1;
        }
    }
    pthread_mutex_init(&mutex, NULL); // Initialize mutex
}

int allocate_jeton() {
    pthread_mutex_lock(&mutex); // Acquire lock

    for (int i = 0; i < nb_licences; i++) {
        if (available_jetons[i]) {
            available_jetons[i] = 0;
            pthread_mutex_unlock(&mutex); // Release lock
            return i + 1; // Return the index of the available jeton
        }
    }

    pthread_mutex_unlock(&mutex); // Release lock
    return 0; // No available jetons
}

void free_jeton(int jeton) {
    if (jeton > 0 && jeton <= nb_licences) {
        pthread_mutex_lock(&mutex); // Acquire lock
        available_jetons[jeton - 1] = 1; // Mark the jeton as available
        pthread_mutex_unlock(&mutex); // Release lock
    }
}

void gestionnaireSIGUSR1(int signum) {
    printf("Serveur (PID %d): Signal SIGUSR1 reçu.\n", getpid());
    /*pthread_mutex_lock(&mutex); // Acquire lock
    // Sort clients by request timestamps
    struct timespec timestamps[nb_clients];
    for (int i = 0; i < nb_clients; i++) {
        if (clients[i].desire > 0) {
            clock_gettime(CLOCK_REALTIME, &timestamps[i]);
        } else {
            timestamps[i].tv_sec = 0;
            timestamps[i].tv_nsec = 0;
        }
    }

    // Sort clients based on request timestamps
    
    for (int i = nb_clients - 1; i >= 0; i--) {
        for (int j = i - 1; j >= 0; j--) {
            if (timestamps[i].tv_sec > timestamps[j].tv_sec) {
                struct timespec temp_timestamp = timestamps[i];
                int temp_client_id = clients[i].num_client;
                timestamps[i] = timestamps[j];
                clients[i].num_client = clients[j].num_client;
                timestamps[j] = temp_timestamp;
                clients[j].num_client = temp_client_id;
            } else if (timestamps[i].tv_sec == timestamps[j].tv_sec && timestamps[i].tv_nsec > timestamps[j].tv_nsec) {
                struct timespec temp_timestamp = timestamps[i];
                int temp_client_id = clients[i].num_client;
                timestamps[i] = timestamps[j];
                clients[i].num_client = clients[j].num_client;
                timestamps[j] = temp_timestamp;
                clients[j].num_client = temp_client_id;
            }
        }
        signal(SIGUSR1, gestionnaireSIGUSR1);
    }
    pthread_mutex_unlock(&mutex); // Release lock*/
    
    // Allocate licenses to clients based on LIFO
    for (int i = nb_clients - 1; i >= 0; i--) {
        if (!clients[i].jeton && !clients[i].in_use && clients[i].desire) {
            int jeton = allocate_jeton();
            if (jeton) {
                clients[i].in_use = 1;
                clients[i].jeton = jeton;
                printf("Client %d (PID %d): Licence accordée (Jeton %d). Utilisation en cours.\n", clients[i].num_client, clients[i].pid, clients[i].jeton);
                sleep(TEMPS_UTILISATION);
                printf("Client %d (PID %d): Fin de l'utilisation. Jeton %d disponible.\n", clients[i].num_client, clients[i].pid, clients[i].jeton);
                clients[i].in_use = 0;
                clients[i].jeton = 0;
                clients[i].desire = 0;
                free_jeton(clients[i + 1].jeton);
                break;
            }
        }
    }
    signal(SIGUSR1, gestionnaireSIGUSR1);
    //pthread_mutex_unlock(&mutex); // Release lock
}

void client(int num_client) {
    //while (1) {
        printf("Client %d (PID %d): Demande de licence.\n", num_client, getpid());
        clients[num_client - 1].num_client = num_client;
        clients[num_client - 1].desire = 1;
        signal(SIGUSR1, gestionnaireSIGUSR1);
        kill(getppid(), SIGUSR1);
        sleep(TEMPS_UTILISATION);
        sleep(1);   // Simulate client activity
        //printf("Client %d (PID %d): Fin de l'utilisation. Jeton %d disponible.\n", num_client, clients[num_client - 1].pid, clients[num_client - 1].jeton);
        //free_jeton(clients[num_client - 1].jeton); // Release the old license
    //}
}
/*void client(int num_client) {
    printf("Client %d (PID %d): Demande de licence.\n", num_client, getpid());
    sleep(0.35);
    clients[num_client - 1].num_client = num_client;
    clients[num_client - 1].desir = 1;
    kill(getppid(), SIGUSR1);
    sleep(1); // Simulate client activity
}*/
int main() {
    printf("Serveur: Lancement du serveur...\n");

    // Get number of licenses and clients from the user
    printf("Saisir le nombre de licences (supérieur à 0) : ");
    scanf("%d", &nb_licences);

    printf("Saisir le nombre de clients (supérieur à 0) : ");
    scanf("%d", &nb_clients);

    if (nb_licences <= 0 || nb_clients <= 0 || nb_licences >= nb_clients) {
        printf("Erreur : nombre de licences doit être supérieur à 0 et inférieur au nombre de clients.\n");
        return 1;
    }

    // Initialize the clients and mutex
    init_clients();
    signal(SIGUSR1, gestionnaireSIGUSR1);

    // Create signal handler for SIGUSR1

    /*struct sigaction action;
    action.sa_handler = gestionnaireSIGUSR1;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGUSR1, &action, NULL);*/

    // Create client processes
    while(1) {
        for (int i = 1; i <= nb_clients; ++i) {
            pid_t pid = fork();
            clients[i].desire = 1;
            if (pid < 0) {
                perror("Erreur lors de la création du processus client");
                exit(1);
            } else if (pid == 0) {
                signal(SIGUSR1, gestionnaireSIGUSR1);
                client(i);
                exit(0);
            } 
        }

    // Wait for all clients to finish
        for (int i = 0; i < nb_clients; ++i) {
            wait(NULL);
        }
    }
    

    // Clean up resources
    pthread_mutex_destroy(&mutex);
    free(clients);
    free(available_jetons);
    printf("Serveur : Fin de la simulation.\n");
    return 0;
}
/*int main() {
    printf("Saisir le nombre de licences (supérieur à 0) : ");
    scanf("%d", &nb_licences);

    printf("Saisir le nombre de clients (supérieur à 0) : ");
    scanf("%d", &nb_clients);

    if (nb_licences <= 0 || nb_clients <= 0 || nb_licences >= nb_clients) {
        printf("Erreur : nombre de licences doit être supérieur à 0 et inférieur au nombre de clients.\n");
        return 1;
    }

    init_clients();
    signal(SIGUSR1, gestionnaireSIGUSR1);

    for (int i = 1; i <= nb_clients; ++i) {
        pid_t pid = fork();
        clients[i].desir = 1;
        if (pid < 0) {
            perror("Erreur lors de la création du processus client");
            exit(1);
        } else if (pid == 0) {
            client(i);
            signal(SIGUSR1, gestionnaireSIGUSR1);
            exit(0);
        }
    }

    for (int i = 0; i < nb_clients; ++i) {
        wait(NULL);
    }

    printf("Serveur : Fin de la simulation.\n");

    free(clients);
    free(available_jetons);
    return 0;
}*/
