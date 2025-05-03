#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <unistd.h>
#include "shm_common.h"

int notif_active = 0;
int stop_notif = 0;
pthread_t notif_thread;

struct SystemData *sys_global;
struct Hunter *hunter_global;

void hunterRegister(struct SystemData *sys) {
    if (sys->num_hunters >= MAX_HUNTERS) {
        printf("The number of hunters is already full!\n");
        return;
    }

    struct Hunter h;
    printf("Username: ");
    scanf("%s", h.username);

    h.level = 1;
    h.exp = 0;
    h.atk = 10;
    h.hp = 100;
    h.def = 5;
    h.banned = 0;
    h.shm_key = rand();  

    int h_shmid = shmget(h.shm_key, sizeof(struct Hunter), IPC_CREAT | 0666);
    if (h_shmid == -1) {
        perror("[ERROR] shmget for hunter failed");
        return;
    }

    struct Hunter *h_ptr = (struct Hunter *)shmat(h_shmid, NULL, 0);
    *h_ptr = h;
    shmdt(h_ptr);

    sys->hunters[sys->num_hunters++] = h;
    printf("Registration succeeded.\n");
}

void showDungeon(struct Hunter *hunter, struct SystemData *sys) {
    printf("\n==========================================================================\n");
    printf("             \033[1;35m Dungeon List for %s (Lv %d) \033[0m\n", hunter->username, hunter->level);
    printf("==========================================================================\n");
    printf("| %-15s | LVL | ATK | HP  | DEF | EXP | KEY     |\n", "Name");
    printf("==========================================================================\n");
    int found = 0;
    for (int i = 0; i < sys->num_dungeons; i++) {
        struct Dungeon d = sys->dungeons[i];
        if (hunter->level >= d.min_level) {
            printf("| %-15s | %3d | %3d | %3d | %3d | %3d | %7d |\n",
                   d.name, d.min_level, d.atk, d.hp, d.def, d.exp, d.shm_key);
            found = 1;
        }
    }
    if (!found) {
        printf("| %-70s |\n", "No dungeons available for your level.");
    }
    printf("==========================================================================\n");
}

void dungeonRaid(struct Hunter *hunter, struct SystemData *sys) {
    if (hunter->banned) {
        printf("You're banned!\n");
        return;
    }

    char dungeonsName[50];
    printf("Enter Dungeon's Name: ");
    scanf("%s", dungeonsName);

    for (int i = 0; i < sys->num_dungeons; i++) {
        struct Dungeon d = sys->dungeons[i];
        if (strcmp(d.name, dungeonsName) == 0) {
            if (hunter->level < d.min_level) {
                printf("Insufficient Level.\n");
                return;
            }

            printf("Dungeon %s has been raided!\n", d.name);
            hunter->atk += d.atk;
            hunter->hp += d.hp;
            hunter->def += d.def;
            hunter->exp += d.exp;

            if (hunter->exp >= 500) {
                hunter->level++;
                hunter->exp = 0;
                printf("Level up to %d!\n", hunter->level);
            }

            for (int j = i; j < sys->num_dungeons - 1; j++) {
                sys->dungeons[j] = sys->dungeons[j + 1];
            }
            sys->num_dungeons--;
            return;
        }
    }

    printf("Dungeon not found.\n");
}

int totalStat(struct Hunter *h) {
    return h->atk + h->hp + h->def;
}

void battle(struct SystemData *sys, struct Hunter *self) {
    if (self->banned) {
        printf("You're banned!\n");
        return;
    }

    printf("\n=== \033[1;34m Other Hunter List\033[0m ===\n");
    for (int i = 0; i < sys->num_hunters; i++) {
        struct Hunter *opponent = &sys->hunters[i];
        if (opponent != self) {
            printf("[%d] %s (Lv %d, ATK %d, HP %d, DEF %d)\n", i,
                   opponent->username, opponent->level, opponent->atk, opponent->hp, opponent->def);
        }
    }

    int idx;
    printf("Choose Opponent Index: ");
    scanf("%d", &idx);

    if (idx < 0 || idx >= sys->num_hunters || &sys->hunters[idx] == self) {
        printf("Invalid choice.\n");
        return;
    }

    struct Hunter *opponent = &sys->hunters[idx];
    if (opponent->banned) {
        printf("The opponent is being banned\n");
        return;
    }

    int stat_self = totalStat(self);
    int stat_enemy = totalStat(opponent);

    if (stat_self >= stat_enemy) {
        printf("\033[38;5;208m██╗░░░██╗░█████╗░██╗░░░██╗  ░██╗░░░░░░░██╗██╗███╗░░██╗\n");
        printf("\033[38;5;214m╚██╗░██╔╝██╔══██╗██║░░░██║  ░██║░░██╗░░██║██║████╗░██║\n");
        printf("\033[38;5;220m░╚████╔╝░██║░░██║██║░░░██║  ░╚██╗████╗██╔╝██║██╔██╗██║\n");
        printf("\033[38;5;226m░░╚██╔╝░░██║░░██║██║░░░██║  ░░████╔═████║░██║██║╚████║\n");
        printf("\033[38;5;228m░░░██║░░░╚█████╔╝╚██████╔╝  ░░╚██╔╝░╚██╔╝░██║██║░╚███║\n");
        printf("\033[38;5;229m░░░╚═╝░░░░╚════╝░░╚═════╝░  ░░░╚═╝░░░╚═╝░░╚═╝╚═╝░░╚══╝\n");
        printf("\033[0m");

        self->atk += opponent->atk;
        self->hp += opponent->hp;
        self->def += opponent->def;
        self->exp += opponent->exp;
        self->level += opponent->level;

        for (int j = idx; j < sys->num_hunters - 1; j++) {
            sys->hunters[j] = sys->hunters[j + 1];
        }
        sys->num_hunters--;

    } else {
        printf("\033[38;5;52m░██████╗░░█████╗░███╗░░░███╗███████╗  ░█████╗░██╗░░░██╗███████╗██████╗░\n");
        printf("\033[38;5;88m██╔════╝░██╔══██╗████╗░████║██╔════╝  ██╔══██╗██║░░░██║██╔════╝██╔══██╗\n");
        printf("\033[38;5;124m██║░░██╗░███████║██╔████╔██║█████╗░░  ██║░░██║╚██╗░██╔╝█████╗░░██████╔╝\n");
        printf("\033[38;5;160m██║░░╚██╗██╔══██║██║╚██╔╝██║██╔══╝░░  ██║░░██║░╚████╔╝░██╔══╝░░██╔══██╗\n");
        printf("\033[38;5;196m╚██████╔╝██║░░██║██║░╚═╝░██║███████╗  ╚█████╔╝░░╚██╔╝░░███████╗██║░░██║\n");
        printf("\033[38;5;210m░╚═════╝░╚═╝░░╚═╝╚═╝░░░░░╚═╝╚══════╝  ░╚════╝░░░░╚═╝░░░╚══════╝╚═╝░░╚═╝\n");
        printf("\033[0m");

        opponent->atk += self->atk;
        opponent->hp += self->hp;
        opponent->def += self->def;
        opponent->exp += self->exp;
        opponent->level += self->level;

        for (int i = 0; i < sys->num_hunters; i++) {
            if (&sys->hunters[i] == self) {
                for (int j = i; j < sys->num_hunters - 1; j++) {
                    sys->hunters[j] = sys->hunters[j + 1];
                }
                sys->num_hunters--;
                break;
            }
        }

        printf("You are removed from the system.\n");
        shmdt(sys);
        exit(0);
    }
}

void *notifDungeon(void *arg) {
    while (!stop_notif) {
        printf("\n📢 Dungeon Notification:\n");
        showDungeon(hunter_global, sys_global);
        sleep(3);
    }
    return NULL;
}

void login(struct SystemData *sys) {
    char username[50];
    printf("Hunter Name: ");
    scanf("%s", username);

    for (int i = 0; i < sys->num_hunters; i++) {
        if (strcmp(sys->hunters[i].username, username) == 0) {
            struct Hunter *current = &sys->hunters[i];
            printf("Login as %s\n", current->username);

            sys_global = sys;
            hunter_global = current;

            int choose;
            while (1) {
                printf("\n=== \033[1;34m%s's MENU\033[0m ===\n", current->username);
                printf("1. Show Dungeon\n"
                       "2. Dungeon Raid\n"
                       "3. Battle\n"
                       "4. Toggle Notification [%s]\n"
                       "5. Logout\n"
                       "Choose: ", notif_active ? "ON" : "OFF");
                scanf("%d", &choose);

                if (choose == 1) showDungeon(current, sys);
                else if (choose == 2) dungeonRaid(current, sys);
                else if (choose == 3) battle(sys, current);
                else if (choose == 4) {
                    if (!notif_active) {
                        stop_notif = 0;
                        pthread_create(&notif_thread, NULL, notifDungeon, NULL);
                        notif_active = 1;
                    } else {
                        stop_notif = 1;
                        pthread_join(notif_thread, NULL);
                        notif_active = 0;
                    }
                } else {
                    if (notif_active) {
                        stop_notif = 1;
                        pthread_join(notif_thread, NULL);
                    }
                    break;
                }
            }

            return;
        }
    }

    printf("Hunter not found.\n");
}

int main() {
    key_t key = get_system_key();

    int shmid = shmget(key, sizeof(struct SystemData), 0666);

    if (shmid == -1) {
        perror("[ERROR] Failed to access shared memory");
        printf("Make sure the system has been run first.\n");
        exit(1);
    }

    struct SystemData *sys = (struct SystemData *)shmat(shmid, NULL, 0);

    int choose;
    while (1) {
        printf("\n=== \033[1;34mMENU HUNTER\033[0m ===\n"
               "1. Register\n"
               "2. Login\n"
               "3. Exit\n"
               "Choose: ");
        scanf("%d", &choose);

        if (choose == 1) {
            hunterRegister(sys); 
        } else if (choose == 2) {
            login(sys); 
        } else {
            break; 
        }
    }

    shmdt(sys);
    return 0;
}
