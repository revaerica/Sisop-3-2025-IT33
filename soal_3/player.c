#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

const char* RESET = "\x1b[0m";
const char* RED = "\x1b[31m";
const char* GREEN = "\x1b[32m";
const char* YELLOW = "\x1b[33m";
const char* BLUE = "\x1b[34m";
const char* CYAN = "\x1b[36m";

int main() {
    int sock;
    struct sockaddr_in server;
    char buffer[BUFFER_SIZE];
    char input[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Could not create socket");
        return 1;
    }

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connect failed");
        close(sock);
        return 1;
    }

    printf("%sConnected to dungeon server.%s\n", GREEN, RESET);

    while (1) {
        printf("\n%s╔════════════════════════════════╗%s\n", YELLOW, RESET);
        printf("%s║ %s🎮 %s== WELCOME TO THE ADVENTURE! == %s🎮  ║%s\n", YELLOW, GREEN, YELLOW, GREEN, RESET);
        printf("%s╠════════════════════════════════╣%s\n", YELLOW, RESET);
        printf("%s║ %s1. Show Stats       %s💥           ║%s\n", GREEN, YELLOW, CYAN, RESET);
        printf("%s║ %s2. Show Shop        %s🛒           ║%s\n", GREEN, YELLOW, CYAN, RESET);
        printf("%s║ %s3. Buy Weapon       %s⚔️           ║%s\n", GREEN, YELLOW, CYAN, RESET);
        printf("%s║ %s4. Inventory & Equip %s🎒          ║%s\n", GREEN, YELLOW, CYAN, RESET);
        printf("%s║ %s5. Battle           %s🔥           ║%s\n", GREEN, YELLOW, CYAN, RESET);
        printf("%s║ %s6. Exit             %s🚪           ║%s\n", GREEN, YELLOW, CYAN, RESET);
        printf("%s╚════════════════════════════════╝%s\n", YELLOW, RESET);

        printf("%sEnter your choice: %s", GREEN, RESET);
        if (!fgets(input, BUFFER_SIZE, stdin)) {
            printf("%sError reading input.%s\n", RED, RESET);
            continue;
        }
        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "6") == 0) break;

        if (strcmp(input, "1") == 0) {
            if (send(sock, "STATS", strlen("STATS"), 0) < 0) {
                perror("Send failed");
                break;
            }
        } else if (strcmp(input, "2") == 0) {
            if (send(sock, "SHOP", strlen("SHOP"), 0) < 0) {
                perror("Send failed");
                break;
            }
        } else if (strcmp(input, "3") == 0) {
            if (send(sock, "SHOP", strlen("SHOP"), 0) < 0) {
                perror("Send failed");
                break;
            }
            memset(buffer, 0, BUFFER_SIZE);
            if (recv(sock, buffer, BUFFER_SIZE, 0) <= 0) {
                printf("%sDisconnected from server.%s\n", RED, RESET);
                break;
            }
            printf("%s\n", buffer);
            printf("%sEnter Weapon ID to buy: %s", GREEN, RESET);
            if (!fgets(input, BUFFER_SIZE, stdin)) {
                printf("%sError reading input.%s\n", RED, RESET);
                continue;
            }
            input[strcspn(input, "\n")] = 0;
            int choice;
            if (sscanf(input, "%d", &choice) != 1 || choice < 1) {
                printf("%sInvalid input!%s\n", RED, RESET);
                continue;
            }
            snprintf(buffer, sizeof(buffer), "BUY %d", choice);
            if (send(sock, buffer, strlen(buffer), 0) < 0) {
                perror("Send failed");
                break;
            }
        } else if (strcmp(input, "4") == 0) {
            if (send(sock, "INVENTORY", strlen("INVENTORY"), 0) < 0) {
                perror("Send failed");
                break;
            }
            memset(buffer, 0, BUFFER_SIZE);
            if (recv(sock, buffer, BUFFER_SIZE, 0) <= 0) {
                printf("%sDisconnected from server.%s\n", RED, RESET);
                break;
            }
            printf("%s\n", buffer);
            printf("%sEnter Weapon ID to equip (0 to cancel): %s", GREEN, RESET);
            if (!fgets(input, BUFFER_SIZE, stdin)) {
                printf("%sError reading input.%s\n", RED, RESET);
                continue;
            }
            input[strcspn(input, "\n")] = 0;
            int choice;
            if (sscanf(input, "%d", &choice) != 1 || choice < 0) {
                printf("%sInvalid input!%s\n", RED, RESET);
                continue;
            }
            if (choice == 0) {
                printf("%sCanceled.%s\n", YELLOW, RESET);
                continue;
            }
            snprintf(buffer, sizeof(buffer), "EQUIP %d", choice - 1);
            if (send(sock, buffer, strlen(buffer), 0) < 0) {
                perror("Send failed");
                break;
            }
        } else if (strcmp(input, "5") == 0) {
            if (send(sock, "BATTLE", strlen("BATTLE"), 0) < 0) {
                perror("Send failed");
                break;
            }
            while (1) {
                memset(buffer, 0, BUFFER_SIZE);
                ssize_t received = recv(sock, buffer, BUFFER_SIZE, 0);
                if (received <= 0) {
                    printf("%sDisconnected from server.%s\n", RED, RESET);
                    break;
                }
                printf("%s\n", buffer);
                if (strstr(buffer, "defeated") || strstr(buffer, "exited")) break;
                printf("%sEnter action (attack/exit): %s", GREEN, RESET);
                if (!fgets(input, BUFFER_SIZE, stdin)) {
                    printf("%sError reading input.%s\n", RED, RESET);
                    break;
                }
                input[strcspn(input, "\n")] = 0;
                if (send(sock, input, strlen(input), 0) < 0) {
                    perror("Send failed");
                    break;
                }
            }
            continue;
        } else {
            printf("%sInvalid option. Please select 1–6.%s\n", RED, RESET);
            continue;
        }

        memset(buffer, 0, BUFFER_SIZE);
        ssize_t received = recv(sock, buffer, BUFFER_SIZE, 0);
        if (received <= 0) {
            printf("%sDisconnected from server.%s\n", RED, RESET);
            break;
        }
        printf("%s\n", buffer);
    }

    close(sock);
    return 0;
}
