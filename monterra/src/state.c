/* Global game state, defined separately from main.c so headless tests can
 * link the logic without SDL. */
#include "game.h"

GameState g;
BattleRequest g_battle_req;
