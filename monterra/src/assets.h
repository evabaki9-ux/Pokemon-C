/* Texture loading for the .tex raw RGBA format + all game assets. */
#ifndef ASSETS_H
#define ASSETS_H

#include <SDL.h>
#include "game.h"

typedef struct {
    SDL_Texture *tex;
    int w, h;
} Tex;

typedef struct {
    Tex tiles;               /* tileset, 16px tiles in one row */
    Tex font;                /* 8x8 glyphs, ASCII 32..126, 16 per row */
    Tex chars;               /* 6 frames x 5 characters, 16px */
    Tex title;               /* 240x160 title art */
    Tex platforms;           /* battle platforms: 2 x (80x20) */
    Tex creature[NUM_SPECIES];
} Assets;

extern Assets A;

int assets_load(SDL_Renderer *r);
void assets_free(void);
/* draw a tile by id at pixel pos (handles animated tiles + column mapping) */
void draw_tile(SDL_Renderer *r, int tile_id, int x, int y, uint32_t tick);
/* draw a character frame: sprite 0..4, dir 0..3 (3=flip), frame 0..1 */
void draw_char(SDL_Renderer *r, int sprite_id, int dir, int frame, int x, int y);
/* draw creature sprite (with tint for evolved placeholders) */
void draw_creature(SDL_Renderer *r, int species, int x, int y, int size, bool flip);

#endif
