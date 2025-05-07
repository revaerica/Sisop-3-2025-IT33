#ifndef SHOP_C
#define SHOP_C

#include <stdio.h>
#include <string.h>

#define MAX_WEAPONS 10

typedef struct {
    char name[50];
    int price;
    int base_damage;
    char passive[100];
    int hasPassive;
} Weapon;

typedef struct {
    char name[50];
    int gold;
    Weapon inventory[MAX_WEAPONS];
    int weaponCount;
    Weapon currentWeapon;
    int baseDamage;
    int hasWeapon;
    int enemiesDefeated;
} Player;

Weapon weapons[MAX_WEAPONS] = {
    {"Iron Sword", 50, 10, "-", 0},
    {"Flame Blade", 200, 25, "Burn: 10% chance to deal 2x damage", 1},
    {"Poison Dagger", 90, 12, "Poison: Deals 5 damage per turn", 1},
    {"Thunder Staff", 250, 30, "Shock: 20% chance to chain attack", 1},
    {"Sky Piercer", 275, 20, "Execute: Auto-kill enemies <20%% HP", 1},
    {"Blade of Despair", 600, 45, "Despair: +25% damage to enemies <50%% HP", 1},
    {"Wind of Nature", 480, 0, "Wind Chant: Immune to phys damage (2s)", 1},
    {"Bloodlust Axe", 520, 28, "Bloodlust: 20%% spell vamp", 1},
    {"Sea Halberd", 500, 30, "Life Drain: -50%% HP regen", 1}
};

Weapon* buyWeapon(int choice, Player *p) {
    if (choice < 1 || choice > MAX_WEAPONS) {
        return NULL;
    }

    Weapon *selected = &weapons[choice - 1];
    if (p->gold < selected->price) {
        return NULL;
    }

    if (p->weaponCount >= MAX_WEAPONS) {
        return NULL;
    }

    p->gold -= selected->price;
    p->inventory[p->weaponCount] = *selected;
    p->weaponCount++;
    return selected;
}

#endif
