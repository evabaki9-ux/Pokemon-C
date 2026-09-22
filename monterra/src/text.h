/* Bitmap font rendering (Press Start 2P baked to font.tex, 8x8 monospace). */
#ifndef TEXT_H
#define TEXT_H

#include <SDL.h>

void draw_text(SDL_Renderer *r, int x, int y, const char *s, SDL_Color c, int scale);
void draw_text_shadow(SDL_Renderer *r, int x, int y, const char *s, SDL_Color c, int scale);
int text_width(const char *s, int scale);

#endif
