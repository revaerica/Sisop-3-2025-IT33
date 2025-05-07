#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h> 
#include <pthread.h>
#include <time.h>
#include "shop.c"

#define PORT 8080
#define BUFFER_SIZE 8192
#define MAX_PLAYERS 100

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"

typedef struct {
    int sock;
    Player player;
    int active;
} Client;

Client clients[MAX_PLAYERS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void initClients() {
    for (int i = 0; i < MAX_PLAYERS; i++) {
        clients[i].active = 0;
    }
}

int addClient(int sock) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!clients[i].active) {
            clients[i].sock = sock;
            clients[i].player = (Player){"Hero", 1000, {}, 0, {}, 10, 0, 0};
            clients[i].active = 1;
            pthread_mutex_unlock(&clients_mutex);
            return i;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    return -1;
}

void removeClient(int sock) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (clients[i].sock == sock && clients[i].active) {
            clients[i].active = 0;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

Player* getPlayer(int sock) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_PLAYERS; i++) {
        if (clients[i].sock == sock && clients[i].active) {
            pthread_mutex_unlock(&clients_mutex);
            return &clients[i].player;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
    return NULL;
}

void showStats(int client_sock, Player *p) {
    char buffer[BUFFER_SIZE * 2];
    snprintf(buffer, sizeof(buffer),
        "\n%s╔════════════════════════════════════════════╗%s\n"
        "%s║              🎮 Player Stats 🎮            ║%s\n"
        "%s╠════════════════════════════════════════════╣%s\n"
        "%s║ 💰 Gold             : %-21d║%s\n"
        "%s║ 🗡️ Weapon           : %-21s║%s\n"
        "%s║ ⚔️ Base Damage      : %-21d║%s\n"
        "%s║ 🔮 Passive Ability  : %-21s║%s\n"
        "%s║ 👾 Enemies Defeated : %-21d║%s\n"
        "%s╚════════════════════════════════════════════╝%s\n",
        YELLOW, RESET, YELLOW, RESET, YELLOW, RESET,
        GREEN, p->gold, RESET,
        BLUE, p->hasWeapon ? p->currentWeapon.name : "None", RESET,
        CYAN, p->baseDamage, RESET,
        MAGENTA, p->hasWeapon && p->currentWeapon.hasPassive ? p->currentWeapon.passive : "-", RESET,
        RED, p->enemiesDefeated, RESET,
        YELLOW, RESET);
    send(client_sock, buffer, strlen(buffer), 0);
}

void showInvent(int client_sock, Player *p) {
    char buffer[BUFFER_SIZE * 2];
    strcpy(buffer,
        "\n╔════╦════════════════════════════╦════════╦════════════════════════════════════╗\n"
        "║ ID ║ Name                       ║ Damage ║ Passive                            ║\n"
        "╠════╬════════════════════════════╬════════╬════════════════════════════════════╣\n");

    for (int i = 0; i < p->weaponCount; i++) {
        char line[512];
        const char *name = p->inventory[i].name;
        int damage = p->inventory[i].base_damage;
        const char *passive = p->inventory[i].hasPassive ? p->inventory[i].passive : "-";

        char passive1[41] = "", passive2[41] = "";
        strncpy(passive1, passive, 40);
        passive1[40] = '\0';
        if (strlen(passive) > 40) {
            strncpy(passive2, passive + 40, 40);
            passive2[40] = '\0';
        }

        if (strlen(passive2) == 0) {
            snprintf(line, sizeof(line),
                "║ %-2d ║ %-28s ║ %-6d ║ %-36s ║\n",
                i + 1, name, damage, passive1);
        } else {
            snprintf(line, sizeof(line),
                "║ %-2d ║ %-28s ║ %-6d ║ %-36s ║\n"
                "║    ║                              ║        ║ %-36s ║\n",
                i + 1, name, damage, passive1, passive2);
        }

        strcat(buffer, line);
    }

    strcat(buffer, "╚════╩════════════════════════════╩════════╩════════════════════════════════════╝\n");
    send(client_sock, buffer, strlen(buffer), 0);
}

void displayShop(int client_sock) {
    char buffer[BUFFER_SIZE] = "";

    snprintf(buffer, sizeof(buffer),
        "\n%s╔════╦══════════════════════╦═══════╦════════╦════════════════════════════════════╗%s\n"
        "%s║ ID ║ Name                 ║ Price ║ Damage ║ Passive                            ║%s\n"
        "%s╠════╬══════════════════════╬═══════╬════════╬════════════════════════════════════╣%s\n",
        YELLOW, RESET, YELLOW, RESET, YELLOW, RESET);

    for (int i = 0; i < MAX_WEAPONS; i++) {
        const char *passive = weapons[i].hasPassive ? weapons[i].passive : "-";
        char passiveLine1[41] = "", passiveLine2[41] = "";

        strncpy(passiveLine1, passive, 40);
        passiveLine1[40] = '\0';
        if (strlen(passive) > 40) {
            strncpy(passiveLine2, passive + 40, 40);
            passiveLine2[40] = '\0';
        }
        char line[512];

        if (strlen(passiveLine2) == 0) {
            snprintf(line, sizeof(line),
                "%s║ %-2d ║ %-20s ║ %-5d ║ %-6d ║ %-40s ║%s\n",
                YELLOW, i + 1, weapons[i].name, weapons[i].price, weapons[i].base_damage, passiveLine1, RESET);
        } else {
            snprintf(line, sizeof(line),
                "%s║ %-2d ║ %-20s ║ %-5d ║ %-6d ║ %-40s ║%s\n"
                "%s║    ║                      ║       ║        ║ %-40s ║%s\n",
                YELLOW, i + 1, weapons[i].name, weapons[i].price, weapons[i].base_damage, passiveLine1, RESET,
                YELLOW, passiveLine2, RESET);
        }
        strncat(buffer, line, sizeof(buffer) - strlen(buffer) - 1);
    }
    strncat(buffer, YELLOW "╚════╩══════════════════════╩═══════╩════════╩════════════════════════════════════╝\n" RESET, sizeof(buffer) - strlen(buffer) - 1);
    send(client_sock, buffer, strlen(buffer), 0);
}

void handleBattle(int client_sock, Player *p) {
    char buffer[BUFFER_SIZE];
    int enemy_hp = rand() % 151 + 50; 
    int player_hp = 100; 
    while (enemy_hp > 0 && player_hp > 0) {
        int bar_length = 20;
        int enemy_bar = (enemy_hp * bar_length) / 200;
        char health_bar[bar_length + 1];
        memset(health_bar, ' ', bar_length);
        for (int i = 0; i < enemy_bar; i++) {
            health_bar[i] = '#';
        }
        health_bar[bar_length] = '\0';

        snprintf(buffer, sizeof(buffer),
         "\n%s╔══════════════════════════════════════╗%s\n"
         "║ %sEnemy HP: %d%s [%s]%s\n"
         "║ %sYour HP: %d%s\n"
         "║ Options: [attack] [exit]            ║%s\n"
         "%s╚══════════════════════════════════════╝%s\n",
         YELLOW, RESET, RED, enemy_hp, RESET, health_bar, RESET,
         GREEN, player_hp, RESET, YELLOW, YELLOW, RESET);
        send(client_sock, buffer, strlen(buffer), 0);
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_sock, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf("Client disconnected during battle.\n");
            removeClient(client_sock);
            break;
        }
        buffer[strcspn(buffer, "\n")] = 0;
        if (strcmp(buffer, "exit") == 0) {
            enemy_hp = rand() % 151 + 50;
            snprintf(buffer, sizeof(buffer), "%sBattle exited. Enemy HP reset.%s\n", YELLOW, RESET);
            send(client_sock, buffer, strlen(buffer), 0);
            return;
        } else if (strcmp(buffer, "attack") == 0) {
            int base_dmg = p->hasWeapon ? p->baseDamage : 5;
            int damage = base_dmg + (rand() % 10);
            int is_critical = (rand() % 100) < 20;
            if (is_critical) {
                damage *= 2;
                snprintf(buffer, sizeof(buffer), "%sCritical Hit!%s\n", RED, RESET);
                send(client_sock, buffer, strlen(buffer), 0);
            }
            int passive_triggered = 0;
            if (p->hasWeapon && p->currentWeapon.hasPassive) {
                if (strcmp(p->currentWeapon.passive, "Burn: 10% chance to deal 2x damage") == 0 && rand() % 100 < 10) {
                    damage *= 2;
                    passive_triggered = 1;
                    snprintf(buffer, sizeof(buffer), "%sPassive Burn activated!%s\n", MAGENTA, RESET);
                    send(client_sock, buffer, strlen(buffer), 0);
                } else if (strcmp(p->currentWeapon.passive, "Poison: Deals 5 damage per turn") == 0) {
                    enemy_hp -= 5;
                    passive_triggered = 1;
                    snprintf(buffer, sizeof(buffer), "%sPassive Poison activated! Dealt 5 extra damage.%s\n", MAGENTA, RESET);
                    send(client_sock, buffer, strlen(buffer), 0);
                } else if (strcmp(p->currentWeapon.passive, "Shock: 20% chance to chain attack") == 0 && rand() % 100 < 20) {
                    int chain_damage = rand() % 10 + 5;
                    enemy_hp -= chain_damage;
                    snprintf(buffer, sizeof(buffer), "%sPassive Shock activated! Chained attack! Dealt %d extra damage.%s\n", MAGENTA, chain_damage, RESET);
                    send(client_sock, buffer, strlen(buffer), 0);
                } else if (strcmp(p->currentWeapon.passive, "Execute: Auto-kill enemies <20%% HP") == 0 && enemy_hp < 40) {
                    enemy_hp = 0;
                    snprintf(buffer, sizeof(buffer), "%sPassive Execute activated! Enemy auto-killed due to low HP.%s\n", MAGENTA, RESET);
                    send(client_sock, buffer, strlen(buffer), 0);
                } else if (strcmp(p->currentWeapon.passive, "Despair: +25%% damage to enemies <50%% HP") == 0 && enemy_hp < 100) {
                    damage = (int)(damage * 1.25);
                    snprintf(buffer, sizeof(buffer), "%sPassive Despair activated! +25%% damage to enemy with HP below 50%%.%s\n", MAGENTA, RESET);
                    send(client_sock, buffer, strlen(buffer), 0);
                } else if (strcmp(p->currentWeapon.passive, "Wind Chant: Immune to phys damage (2s)") == 0) {
                    snprintf(buffer, sizeof(buffer), "%sPassive Wind Chant activated! Immune to physical damage for 2 seconds.%s\n", MAGENTA, RESET);
                    send(client_sock, buffer, strlen(buffer), 0);
                } else if (strcmp(p->currentWeapon.passive, "Bloodlust: 20%% spell vamp") == 0) {
                    int heal_amount = (int)(damage * 0.20);
                    player_hp += heal_amount;
                    snprintf(buffer, sizeof(buffer), "%sPassive Bloodlust activated! Healed %d HP.%s\n", MAGENTA, heal_amount, RESET);
                    send(client_sock, buffer, strlen(buffer), 0);
                } else if (strcmp(p->currentWeapon.passive, "Life Drain: -50%% HP regen") == 0) {
                    enemy_hp -= 10; 
                    snprintf(buffer, sizeof(buffer), "%sPassive Life Drain activated! Dealt 10 extra damage.%s\n", MAGENTA, RESET);
                    send(client_sock, buffer, strlen(buffer), 0);
                }
            }
            enemy_hp -= damage;
            snprintf(buffer, sizeof(buffer),
                     "%sYou dealt %d damage! Enemy HP: %d%s\n",
                     GREEN, damage, enemy_hp > 0 ? enemy_hp : 0, RESET);
            send(client_sock, buffer, strlen(buffer), 0);
            if (enemy_hp <= 0) {
                int reward = rand() % 51 + 50;
                p->gold += reward;
                p->enemiesDefeated++;
                snprintf(buffer, sizeof(buffer),
                         "%sEnemy defeated! You earned %d gold. Total gold: %d%s\n",
                         GREEN, reward, p->gold, RESET);
                send(client_sock, buffer, strlen(buffer), 0);
                enemy_hp = rand() % 151 + 50;
                continue;
            }
            int enemy_dmg = rand() % 11 + 5;
            player_hp -= enemy_dmg;
            snprintf(buffer, sizeof(buffer),
                     "%sEnemy dealt %d damage! Your HP: %d%s\n",
                     RED, enemy_dmg, player_hp > 0 ? player_hp : 0, RESET);
            send(client_sock, buffer, strlen(buffer), 0);
            if (player_hp <= 0) {
                snprintf(buffer, sizeof(buffer), "%sYou were defeated!%s\n", RED, RESET);
                send(client_sock, buffer, strlen(buffer), 0);
                return;
            }
        } else {
            snprintf(buffer, sizeof(buffer), "%sInvalid option. Use 'attack' or 'exit'.%s\n", RED, RESET);
            send(client_sock, buffer, strlen(buffer), 0);
        }
    }
}

void* handlePlayer(void* arg) {
    int client_sock = *((int*)arg);
    free(arg);

    int client_id = addClient(client_sock);
    if (client_id == -1) {
        char msg[] = "Server full!\n";
        send(client_sock, msg, strlen(msg), 0);
        close(client_sock);
        return NULL;
    }

    Player *player = getPlayer(client_sock);
    char buffer[BUFFER_SIZE];
    srand(time(NULL));

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_sock, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf("Player disconnected.\n");
            removeClient(client_sock);
            break;
        }

        if (strncmp(buffer, "STATS", 5) == 0) {
            showStats(client_sock, player);
        } else if (strncmp(buffer, "INVENTORY", 9) == 0) {
            showInvent(client_sock, player);
        } else if (strncmp(buffer, "SHOP", 4) == 0) {
            displayShop(client_sock);
        }  else if (strncmp(buffer, "BUY", 3) == 0) {
            char temp_buf[BUFFER_SIZE];
            strncpy(temp_buf, buffer, BUFFER_SIZE);
            temp_buf[BUFFER_SIZE - 1] = '\0';
        
            char *cmd = strtok(temp_buf, " ");
            char *id_str = strtok(NULL, " ");
        
            int id = (id_str != NULL) ? atoi(id_str) : -1;
            Weapon *w = buyWeapon(id, player);
        
            if (w) {
                snprintf(buffer, sizeof(buffer), "%s✅ You bought %s!%s\n", GREEN, w->name, RESET);
            } else {
                if (id < 1 || id > MAX_WEAPONS) {
                    snprintf(buffer, sizeof(buffer), "%s❌ Invalid weapon ID.%s\n", RED, RESET);
                } else if (player->gold < weapons[id - 1].price) {
                    snprintf(buffer, sizeof(buffer), "%s❌ Insufficient gold.%s\n", RED, RESET);
                } else if (player->weaponCount >= MAX_WEAPONS) {
                    snprintf(buffer, sizeof(buffer), "%s❌ Inventory full.%s\n", RED, RESET);
                } else {
                    snprintf(buffer, sizeof(buffer), "%s❌ Purchase failed.%s\n", RED, RESET);
                }
            }
            send(client_sock, buffer, strlen(buffer), 0);
        }else if (strncmp(buffer, "EQUIP", 5) == 0) {
            char temp_buf[BUFFER_SIZE];
            strncpy(temp_buf, buffer, BUFFER_SIZE);
            temp_buf[BUFFER_SIZE - 1] = '\0';
        
            char *cmd = strtok(temp_buf, " ");
            char *id_str = strtok(NULL, " ");
        
            int id = (id_str != NULL) ? atoi(id_str) : -1;
        
            if (id >= 1 && id <= player->weaponCount) {
                player->currentWeapon = player->inventory[id - 1];
                player->baseDamage = player->currentWeapon.base_damage;
                player->hasWeapon = 1;
                snprintf(buffer, sizeof(buffer),
                         "%sEquipped %s!%s\n", GREEN, player->currentWeapon.name, RESET);
            } else {
                snprintf(buffer, sizeof(buffer), "%sInvalid weapon ID!%s\n", RED, RESET);
            }
            send(client_sock, buffer, strlen(buffer), 0);
        } else if (strncmp(buffer, "BATTLE", 6) == 0) {
            handleBattle(client_sock, player);
        } else {
            snprintf(buffer, sizeof(buffer), "%s❓ Unknown command.%s\n", RED, RESET);
            send(client_sock, buffer, strlen(buffer), 0);
        }
    }
    close(client_sock);
    return NULL;
}

int main() {
    initClients();
    int server_fd, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        return 1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        close(server_fd);
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) < 0) {
        perror("Listen failed");
        close(server_fd);
        return 1;
    }

    printf("%s[Server Started] Listening on port %d...%s\n", CYAN, PORT, RESET);
    printf("      __..,,__　　　,.｡='`1\n");
    printf("　　　 .,,..;~`''''　　　　`''''＜``彡　}\n");
    printf("_...:=,`'　　 　︵　 т　︵　　X彡-J\n");
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
        if (!pclient) {
            perror("Memory allocation failed");
            close(client_sock);
            continue;
        }
        *pclient = client_sock;
        if (pthread_create(&tid, NULL, handlePlayer, pclient) != 0) {
            perror("Thread creation failed");
            free(pclient);
            close(client_sock);
            continue;
        }
        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}
