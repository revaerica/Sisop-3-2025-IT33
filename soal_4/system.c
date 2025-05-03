#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <time.h>
#include "shm_common.h"

int shmid = -1;
struct SystemData *sys = NULL;

void cleanup(int sig) {
    printf("\n[!] System shutting down, shared memory cleaned up.\n");
    if (sys != NULL) shmdt(sys);
    if (shmid != -1) shmctl(shmid, IPC_RMID, NULL);
    exit(0);
}

void showHunter() {
        printf("=================================================================\n");
        printf("                \033[1;34m Hunter List \033[0m\n");
        printf("=================================================================\n");
        printf("| %-15s | LVL | EXP  | ATK | HP  | DEF | STATUS   |\n", "Name");
        printf("=================================================================\n");
        for (int i = 0; i < sys->num_hunters; i++) {
            struct Hunter h = sys->hunters[i];
            printf("| %-15s | %3d | %4d | %3d | %3d | %3d | %-8s |\n",
                   h.username, h.level, h.exp, h.atk, h.hp, h.def,
                   h.banned ? "BANNED" : "ACTIVE");
        }
        printf("=================================================================\n");
}

void createDungeon() {
    if (sys->num_dungeons >= MAX_DUNGEONS) return;

    struct Dungeon *d = &sys->dungeons[sys->num_dungeons++];
    sprintf(d->name, "Dungeon-%d", rand() % 100);
    d->min_level = rand() % 5 + 1;
    d->atk = rand() % 51 + 100;
    d->hp = rand() % 51 + 50;
    d->def = rand() % 26 + 25;
    d->exp = rand() % 151 + 150;
    d->shm_key = rand(); 
    
    key_t dungeon_key = ftok(".", rand() % 100 + 1);
    d->shm_key = dungeon_key;

    int dungeon_shmid = shmget(dungeon_key, 128, IPC_CREAT | 0666); 
    if (dungeon_shmid == -1) {
        perror("Failed to create dungeon shared memory");
        exit(1);
    }

    printf("Dungeon %s created with shm_key %d.\n", d->name, dungeon_key);
}

void showDungeon() {
        printf("\n==========================================================================\n");
        printf("                         \033[1;35m Dungeon List \033[0m\n");
        printf("==========================================================================\n");
        printf("| %-15s | LVL | ATK | HP  | DEF | EXP | KEY     |\n", "Name");
        printf("==========================================================================\n");
        for (int i = 0; i < sys->num_dungeons; i++) {
            struct Dungeon d = sys->dungeons[i];
            printf("| %-15s | %3d | %3d | %3d | %3d | %3d | %7d |\n",
                   d.name, d.min_level, d.atk, d.hp, d.def, d.exp, d.shm_key);
        }
        printf("==========================================================================\n");   
}

void banHunter() {
    char username[50];
    printf("Enter the hunter's username to ban: ");
    scanf("%s", username);
    for (int i = 0; i < sys->num_hunters; i++) {
        if (strcmp(sys->hunters[i].username, username) == 0) {
            sys->hunters[i].banned = 1;
            printf("%s has been banned.\n", username);
            return;
        }
    }
    printf("Not found.\n");
}

void unbanHunter() {
    char username[50];
    printf("Enter the hunter's username to unban: ");
    scanf("%s", username);
    for (int i = 0; i < sys->num_hunters; i++) {
        if (strcmp(sys->hunters[i].username, username) == 0) {
            sys->hunters[i].banned = 0;
            printf("%s has been unbanned.\n", username);
            return;
        }
    }
    printf("Not found.\n");
}

void resetHunter() {
    char username[50];
    printf("Enter the hunter's username to reset: ");
    scanf("%s", username);
    for (int i = 0; i < sys->num_hunters; i++) {
        if (strcmp(sys->hunters[i].username, username) == 0) {
            sys->hunters[i].level = 1;
            sys->hunters[i].exp = 0;
            sys->hunters[i].atk = 10;
            sys->hunters[i].hp = 100;
            sys->hunters[i].def = 5;
            printf("Stats for %s have been reset.\n", username);
            return;
        }
    }
    printf("Not found.\n");
}

int main() {
    signal(SIGINT, cleanup);
    srand(time(NULL));

    key_t key = get_system_key();  // dari shm_common.h
    shmid = shmget(key, sizeof(struct SystemData), IPC_CREAT | 0666);
    
    if (shmid == -1) {
        perror("Shared memory creation failed.");
        exit(1);
    }
    
    sys = (struct SystemData *)shmat(shmid, NULL, 0);

    if (sys == (void *) -1) {
        perror("Failed to attach shared memory.");
        exit(1);
    }

    int choices;
    while (1) {
        printf("\n=== \033[1;34m MENU SYSTEM \033[0m===\n");
        printf("1. Show Hunter\n"
               "2. Create Dungeon\n"
               "3. Show Dungeon\n"
               "4. Ban Hunter\n"
               "5. Unban Hunter\n"
               "6. Reset Hunter\n"
               "7. Exit\n");
        printf("Choose: ");
        scanf("%d", &choices);

        if (choices == 1) showHunter();
        else if (choices == 2) createDungeon();
        else if (choices == 3) showDungeon();
        else if (choices == 4) banHunter();
        else if (choices == 5) unbanHunter();
        else if (choices == 6) resetHunter();
        else break;
    }

    cleanup(0);
    return 0;
}
