#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <time.h>

#define MAX_ORDERS 100
#define SHM_KEY 1234

typedef struct {
    char name[50];
    char address[100];
    char type[20];   // "Express" or "Reguler"
    char status[20]; // "Pending" or "Delivered"
    char agent[20];
} Order;

typedef struct {
    Order orders[MAX_ORDERS];
    int count;
} OrderList;

void loadOrdersFromCSV(OrderList *orderList) {
    FILE *file = fopen("delivery_order.csv", "r");
    if (!file) {
        perror("Failed to open delivery_order.csv");
        exit(1);
    }

    char line[256];
    orderList->count = 0;

    // Skip header
    fgets(line, sizeof(line), file);

    while (fgets(line, sizeof(line), file) && orderList->count < MAX_ORDERS) {
        char *token = strtok(line, ",");
        if (!token) continue;
        strcpy(orderList->orders[orderList->count].name, token);

        token = strtok(NULL, ",");
        if (!token) continue;
        strcpy(orderList->orders[orderList->count].address, token);

        token = strtok(NULL, ",\r\n");
        if (!token) continue;
        strcpy(orderList->orders[orderList->count].type, token);

        strcpy(orderList->orders[orderList->count].status, "Pending");
        strcpy(orderList->orders[orderList->count].agent, "");

        orderList->count++;
    }

    fclose(file);
}

void logDelivery(const char *agent, const char *name, const char *address, const char *type) {
    time_t now;
    time(&now);
    struct tm *local = localtime(&now);

    FILE *logFile = fopen("delivery.log", "a");
    if (!logFile) {
        perror("Failed to open delivery.log");
        return;
    }

    fprintf(logFile, "[%02d/%02d/%04d %02d:%02d:%02d] [AGENT %s] %s package delivered to %s in %s\n",
            local->tm_mday, local->tm_mon + 1, local->tm_year + 1900,
            local->tm_hour, local->tm_min, local->tm_sec,
            agent, type, name, address);

    fclose(logFile);
}

void printHelp() {
    printf("Usage:\n");
    printf("  ./dispatcher -list\n");
    printf("  ./dispatcher -status [Name]\n");
    printf("  ./dispatcher -deliver [Name]\n");
}

int main(int argc, char *argv[]) {
    // Create or access shared memory
    int shmid = shmget(SHM_KEY, sizeof(OrderList), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(1);
    }

    OrderList *orderList = (OrderList *)shmat(shmid, NULL, 0);
    if (orderList == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }

    // Initialize if empty
    if (orderList->count <= 0 || orderList->count > MAX_ORDERS) {
        loadOrdersFromCSV(orderList);
    }

    if (argc < 2) {
        printHelp();
        shmdt(orderList);
        return 0;
    }

    if (strcmp(argv[1], "-list") == 0) {
        printf("All Orders:\n");
        for (int i = 0; i < orderList->count; i++) {
            printf("%d. %s - %s - %s - %s\n", i + 1,
                   orderList->orders[i].name,
                   orderList->orders[i].type,
                   orderList->orders[i].status,
                   orderList->orders[i].agent[0] ? orderList->orders[i].agent : "Not assigned");
        }
    } else if (strcmp(argv[1], "-status") == 0 && argc == 3) {
        int found = 0;
        for (int i = 0; i < orderList->count; i++) {
            if (strcmp(orderList->orders[i].name, argv[2]) == 0) {
                printf("Status for %s: %s", argv[2], orderList->orders[i].status);
                if (orderList->orders[i].agent[0]) {
                    printf(" by Agent %s", orderList->orders[i].agent);
                }
                printf("\n");
                found = 1;
                break;
            }
        }
        if (!found) {
            printf("Order for %s not found\n", argv[2]);
        }
    } else if (strcmp(argv[1], "-deliver") == 0 && argc == 3) {
        int found = 0;
        for (int i = 0; i < orderList->count; i++) {
            if (strcmp(orderList->orders[i].name, argv[2]) == 0 &&
                strcmp(orderList->orders[i].status, "Pending") == 0 &&
                strcmp(orderList->orders[i].type, "Reguler") == 0) {

                strcpy(orderList->orders[i].status, "Delivered");

                char *username = getenv("USER");
                if (!username) username = "Unknown";
                strcpy(orderList->orders[i].agent, username);

                logDelivery(username,
                            orderList->orders[i].name,
                            orderList->orders[i].address,
                            orderList->orders[i].type);

                printf("Reguler package delivered to %s by Agent %s\n",
                       orderList->orders[i].name, username);
                found = 1;
                break;
            }
        }
        if (!found) {
            printf("Reguler order for %s not found or already delivered\n", argv[2]);
        }
    } else {
        printHelp();
    }

    shmdt(orderList);
    return 0;
}
