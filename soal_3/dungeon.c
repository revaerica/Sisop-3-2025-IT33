#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include "shop.c"

#define PORT 8080
#define BUFFER_SIZE 1024

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"

void handleBattle(int client_sock, Player *p);

void showStats(int client_sock, Player *p) {
    char buffer[BUFFER_SIZE * 2];

    snprintf(buffer, sizeof(buffer),
        "\n%s╔════════════════════════════════════════════╗%s\n"
        "%s║              🎮 Player Stats 🎮             ║%s\n"
        "%s╠════════════════════════════════════════════╣%s\n"
        "%s║ 💰 Gold             : %-22d ║%s\n"
        "%s║ 🗡️  Weapon           : %-22s ║%s\n"
        "%s║ ⚔️  Base Damage      : %-22d ║%s\n"
        "%s║ 🔮 Passive Ability  : %-22s ║%s\n"
        "%s║ 👾 Enemies Defeated : %-22d ║%s\n"
        "%s╚════════════════════════════════════════════╝%s\n",
        YELLOW, RESET,
        YELLOW, RESET,
        YELLOW, RESET,
        GREEN, p->gold, RESET,
        BLUE, p->hasWeapon ? p->currentWeapon.name : "None", RESET,
        CYAN, p->baseDamage, RESET,
        MAGENTA, p->hasWeapon && p->currentWeapon.hasPassive ? p->currentWeapon.passive : "-", RESET,
        RED, p->weaponCount, RESET,
        YELLOW, RESET
    );

    send(client_sock, buffer, strlen(buffer), 0);
}

void showInvent(int client_sock, Player *p) {
    char buffer[BUFFER_SIZE * 2];
    strcpy(buffer, "\n📦 Inventory:\n");
    for (int i = 0; i < p->weaponCount; i++) {
        char line[256];
        snprintf(line, sizeof(line), 
            "%s- %s (Damage: %d, Passive: %s)%s\n",
            CYAN, 
            p->inventory[i].name,
            p->inventory[i].base_damage,
            p->inventory[i].hasPassive ? p->inventory[i].passive : "-", 
            RESET
        );
        strcat(buffer, line);
    }
    send(client_sock, buffer, strlen(buffer), 0);
}

void handlePlayer(int client_sock) {
    Player player = {"Hero", 300, {}, 0, {}, 10, 0};
    char buffer[BUFFER_SIZE];
    srand(time(NULL));

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_sock, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf("Player disconnected.\n");
            break;
        }

        if (strncmp(buffer, "STATS", 5) == 0) {
            showStats(client_sock, &player);
        } else if (strncmp(buffer, "INVENTORY", 9) == 0) {
            showInvent(client_sock, &player);
        } else if (strncmp(buffer, "SHOP", 4) == 0) {
            char shopBuf[BUFFER_SIZE * 2] = "";
            FILE *fp = tmpfile();
            if (fp) {
                int old = dup(fileno(stdout));
                dup2(fileno(fp), fileno(stdout));
                displayShop();
                fflush(stdout);
                dup2(old, fileno(stdout));
                fseek(fp, 0, SEEK_SET);
                fread(shopBuf, 1, sizeof(shopBuf), fp);
                fclose(fp);
                send(client_sock, shopBuf, strlen(shopBuf), 0);
            }
        } else if (strncmp(buffer, "BUY", 3) == 0) {
            int id = atoi(buffer + 4);
            Weapon *w = buyWeapon(id, &player);
            if (w) {
                char msg[128];
                snprintf(msg, sizeof(msg),
                         "%s✅ You bought %s!%s\n", GREEN, w->name, RESET);
                send(client_sock, msg, strlen(msg), 0);
            } else {
                char msg[] = "❌ Purchase failed.\n";
                send(client_sock, msg, strlen(msg), 0);
            }
        } else if (strncmp(buffer, "BATTLE", 6) == 0) {
            handleBattle(client_sock, &player);
        } else {
            char msg[] = "❓ Unknown command.\n";
            send(client_sock, msg, strlen(msg), 0);
        }
    }
    close(client_sock);
}

void handleBattle(int client_sock, Player *p) {
    char result[256];
    int damage = p->hasWeapon ? p->baseDamage : 5;
    int monster_hp = rand() % 50 + 50; 
    int player_hit = rand() % damage + (damage / 2);

    monster_hp -= player_hit; 

    snprintf(result, sizeof(result),
             "You fought a dungeon beast with %d HP.\nYou dealt %d damage!\nMonster HP left: %d\n",
             monster_hp + player_hit, player_hit, monster_hp > 0 ? monster_hp : 0);

    if (monster_hp <= 0) {
        snprintf(result + strlen(result), sizeof(result) - strlen(result),
                 "You defeated the monster! 🎉\n");
    } else {
        int monster_attack = rand() % 10 + 5; 
        int player_hp = 100;

        player_hp -= monster_attack;

        snprintf(result + strlen(result), sizeof(result) - strlen(result),
                 "The monster attacks! You lost %d HP.\nYour remaining HP: %d\n", 
                 monster_attack, player_hp > 0 ? player_hp : 0);

        if (player_hp <= 0) {
            snprintf(result + strlen(result), sizeof(result) - strlen(result),
                     "You were defeated by the monster. 😞\n");
        }
    }

    send(client_sock, result, strlen(result), 0);
}

void *handleClient(void *client_sock_ptr) {
    int client_sock = *((int*)client_sock_ptr);
    free(client_sock_ptr); 
    handlePlayer(client_sock);
    close(client_sock); 
    return NULL;
}

int main() {
    int server_fd, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_fd, 5);
    printf("%s[Server Started] Listening on port %d...%s\n", CYAN, PORT, RESET);
    printf("        __..,,__　　　,.｡='`1\n");
    printf("　　　　 .,,..;~`''''　　　　`''''＜``彡　}\n");
    printf("　 _...:=,`'　　 　︵　 т　︵　　X彡-J\n");
    printf("＜`　彡 /　　ミ　　,_人_.　＊彡　`~\n");
    printf("　 `~=::　　　 　　　　　　 　　　Y\n");
    printf("　　 　i.　　　　　　　　　　　　 .:\n");
    printf("　　　.\\　　　　　　　,｡---.,,　　./\n");
    printf("　　　　ヽ　／ﾞ''```\\;.{　　　 ＼／\n");
    printf("　　　　　Y　　　`J..r_.彳　 　|\n");
    printf("　　　　　{　　　``　　`　　　i\n");
    printf("　　　　　\\　　　　　　　　　＼　　　..︵︵.\n");
    printf("　　　　　`＼　　　　　　　　　``ゞ.,/` oQ o`)\n");
    printf("　　　　　　`i,　　　　　　　　　　Y　 ω　/\n");
    printf("　　　　 　　`i,　　　 　　.　　　　\"　　　/\n");
    printf("　　　　　　`iミ　　　　　　　　　　　,,ノ\n");
    printf("　　　　 　 ︵Y..︵.,,　　　　　,,+..__ノ``\n");
    printf("　　　　　(,`, З о　　　　,.ノ川彡ゞ彡　　＊\n");
    printf("　　　　　 ゞ_,,,....彡彡~　　　`+Х彡彡彡彡*\n");
    
    while ((client_sock = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len))) {
        pthread_t tid;
        int *pclient = malloc(sizeof(int)); 
        *pclient = client_sock;
        pthread_create(&tid, NULL, handleClient, pclient); 
        pthread_detach(tid); 
    }

    close(server_fd);
    return 0;
}
