#ifndef OVERWORLD_H
#define OVERWORLD_H

#include <SDL.h>

void ow_reset(uint8_t map, uint8_t x, uint8_t y);
void ow_update(void);
void ow_draw(SDL_Renderer *r);

#endif
