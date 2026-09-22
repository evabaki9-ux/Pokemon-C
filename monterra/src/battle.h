#ifndef BATTLE_H
#define BATTLE_H

#include <SDL.h>
#include "game.h"

void battle_start_wild(uint8_t species, uint8_t level);
void battle_start_trainer(const TrainerDef *t);
void battle_update(void);
void battle_draw(SDL_Renderer *r);
bool battle_over(void);
uint8_t battle_result(void); /* 0 win/escape, 1 whiteout, 2 caught, 3 trainer win */

#endif
