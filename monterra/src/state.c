/* Global game state, defined separately from main.c so headless tests can
 * link the logic without SDL. */
#include "game.h"

GameState g;
BattleRequest g_battle_req;

/* bag: stack same items into one slot (state logic, testable headless) */
void bag_add(uint8_t item)
{
    /* stack: same items pile up in one slot instead of eating rows */
    for (int i = 0; i < g.bag_n; i++) {
        if (g.bag[i] == item && g.bag_qty[i] < 99) {
            g.bag_qty[i]++;
            return;
        }
    }
    if (g.bag_n < MAX_BAG) {
        g.bag[g.bag_n] = item;
        g.bag_qty[g.bag_n] = 1;
        g.bag_n++;
    }
}

int bag_count(uint8_t item)
{
    for (int i = 0; i < g.bag_n; i++)
        if (g.bag[i] == item)
            return g.bag_qty[i];
    return 0;
}

void bag_consume(uint8_t item)
{
    for (int i = 0; i < g.bag_n; i++) {
        if (g.bag[i] == item) {
            if (g.bag_qty[i] > 1) {
                g.bag_qty[i]--;
                return;
            }
            for (int j = i; j < g.bag_n - 1; j++) {
                g.bag[j] = g.bag[j + 1];
                g.bag_qty[j] = g.bag_qty[j + 1];
            }
            g.bag_n--;
            return;
        }
    }
}
