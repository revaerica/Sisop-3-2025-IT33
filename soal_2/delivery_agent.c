#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <semaphore.h>

#define MAX_ORDERS 100
#define SHM_KEY 1234
#define SEM_KEY 5678

typedef struct {
    char name[100];
    char address[200];
    char type[20];
    int delivered;
    time_t order_time;
} Order;

typedef struct {
    Order orders[MAX_ORDERS];
    int count;
    sem_t semaphore;
} SharedMemory;

SharedMemory *shared_mem;
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void download_csv() {
    
    system("cp delivery_order.csv delivery_order_downloaded.csv");
    printf("File downloaded as delivery_order_downloaded.csv\n");
}

void read_csv_to_shared_memory() {
    FILE *file = fopen("delivery_order_downloaded.csv", "r");
    if (!file) {
        perror("Failed to open CSV file");
        exit(1);
    }

    char line[500];
    int first_line = 1;
    int index = 0;

    while (fgets(line, sizeof(line), file) && index < MAX_ORDERS) {
        if (first_line) {
            first_line = 0;
            continue; 
        }

        char *token;
        token = strtok(line, ",");
        if (!token) continue;

        
        strncpy(shared_mem->orders[index].name, token, sizeof(shared_mem->orders[index].name) - 1);
        
        
        token = strtok(NULL, ",");
        if (!token) continue;
        strncpy(shared_mem->orders[index].address, token, sizeof(shared_mem->orders[index].address) - 1);
        
        
        token = strtok(NULL, ",");
        if (!token) continue;
        strncpy(shared_mem->orders[index].type, token, sizeof(shared_mem->orders[index].type) - 1);
        
        
        shared_mem->orders[index].type[strcspn(shared_mem->orders[index].type, "\n")] = 0;
        
        shared_mem->orders[index].delivered = 0;
        shared_mem->orders[index].order_time = time(NULL);
        
        index++;
    }

    shared_mem->count = index;
    fclose(file);
    printf("Loaded %d orders into shared memory\n", index);
}

void log_delivery(const char *agent, const char *name, const char *address) {
    pthread_mutex_lock(&log_mutex);
    
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    
    FILE *log_file = fopen("delivery.log", "a");
    if (!log_file) {
        perror("Failed to open log file");
        pthread_mutex_unlock(&log_mutex);
        return;
    }
    
    char timestamp[20];
    strftime(timestamp, sizeof(timestamp), "%d/%m/%Y %H:%M:%S", tm_info);
    
    fprintf(log_file, "[%s] [%s] Express package delivered to %s in %s\n", 
            timestamp, agent, name, address);
    
    fclose(log_file);
    pthread_mutex_unlock(&log_mutex);
}

void *agent_delivery(void *arg) {
    char *agent_name = (char *)arg;
    
    while (1) {
        sem_wait(&shared_mem->semaphore);
        
        int found = 0;
        for (int i = 0; i < shared_mem->count; i++) {
            if (strcmp(shared_mem->orders[i].type, "Express") == 0 && 
                shared_mem->orders[i].delivered == 0) {
                
                
                shared_mem->orders[i].delivered = 1;
                found = 1;
                
                
                sleep(1);
                
                
                log_delivery(agent_name, shared_mem->orders[i].name, shared_mem->orders[i].address);
                
                printf("%s delivered to %s at %s\n", 
                       agent_name, shared_mem->orders[i].name, shared_mem->orders[i].address);
                break;
            }
        }
        
        sem_post(&shared_mem->semaphore);
        
        if (!found) {
            sleep(1); 
        }
    }
    
    return NULL;
}

int main() {
    
    int shmid = shmget(SHM_KEY, sizeof(SharedMemory), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(1);
    }
    
    shared_mem = (SharedMemory *)shmat(shmid, NULL, 0);
    if (shared_mem == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }
    
    
    if (sem_init(&shared_mem->semaphore, 1, 1) == -1) {
        perror("sem_init failed");
        exit(1);
    }
    
    
    download_csv();
    
    
    read_csv_to_shared_memory();
    
    
    pthread_t agent_a, agent_b, agent_c;
    
    pthread_create(&agent_a, NULL, agent_delivery, "AGENT A");
    pthread_create(&agent_b, NULL, agent_delivery, "AGENT B");
    pthread_create(&agent_c, NULL, agent_delivery, "AGENT C");
    
    
    sleep(30);
    
    
    pthread_cancel(agent_a);
    pthread_cancel(agent_b);
    pthread_cancel(agent_c);
    
    shmdt(shared_mem);
    shmctl(shmid, IPC_RMID, NULL);
    
    return 0;
}
