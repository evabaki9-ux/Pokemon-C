#include <string.h>
#include "text.h"
#include "assets.h"

#define FONT_FIRST 32
#define FONT_COLS 16

static void draw_glyph(SDL_Renderer *r, char c, int x, int y, SDL_Color col, int scale)
{
    unsigned char u = (unsigned char)c;
    if (u < FONT_FIRST || u > 126)
        u = '?';
    int idx = u - FONT_FIRST;
    SDL_Rect src = { (idx % FONT_COLS) * 8, (idx / FONT_COLS) * 8, 8, 8 };
    SDL_Rect dst = { x, y, 8 * scale, 8 * scale };
    SDL_SetTextureColorMod(A.font.tex, col.r, col.g, col.b);
    SDL_RenderCopy(r, A.font.tex, &src, &dst);
    SDL_SetTextureColorMod(A.font.tex, 255, 255, 255);
}

void draw_text(SDL_Renderer *r, int x, int y, const char *s, SDL_Color c, int scale)
{
    if (!A.font.tex)
        return;
    int cx = x;
    for (const char *p = s; *p; p++) {
        if (*p == '\n') {
            cx = x;
            y += 10 * scale;
            continue;
        }
        draw_glyph(r, *p, cx, y, c, scale);
        cx += 8 * scale;
    }
}

void draw_text_shadow(SDL_Renderer *r, int x, int y, const char *s, SDL_Color c, int scale)
{
    SDL_Color black = { 24, 24, 24, 255 };
    draw_text(r, x + scale, y + scale, s, black, scale);
    draw_text(r, x, y, s, c, scale);
}

int text_width(const char *s, int scale)
{
    int best = 0, cur = 0;
    for (const char *p = s; *p; p++) {
        if (*p == '\n') {
            if (cur > best) best = cur;
            cur = 0;
            continue;
        }
        cur += 8 * scale;
    }
    return cur > best ? cur : best;
}
