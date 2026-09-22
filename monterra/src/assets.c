#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "assets.h"
#include "game.h"

Assets A;

/* column offsets of each tile in the atlas (anim tiles own 2 columns) */
static const uint8_t TILE_COL[TI_COUNT] = {
    0,  /* GRASS */
    1,  /* TALL (2) */
    3,  /* PATH */
    4,  /* FLOWER (2) */
    6,  /* TREE */
    7,  /* WATER (2) */
    9,  /* FENCE */
    10, /* ROCK */
    11, /* SAND */
    12, /* BRIDGE */
    13, /* LEDGE */
    14, /* SIGN */
    15, /* ROOF */
    16, /* ROOF_L */
    17, /* ROOF_R */
    18, /* WALL */
    19, /* WINDOW */
    20, /* DOOR */
    21, /* DOOR_HEAL */
    22, /* FLOOR */
    23, /* WALL_IN */
    24, /* BED */
    25, /* TABLE */
    26, /* SHELF */
    27, /* MAT */
    28, /* PC */
    29, /* COUNTER */
    30, /* PLANT */
    31, /* CARPET */
    32, /* GRASS_TOWN */
};
static const bool TILE_ANIM[TI_COUNT] = {
    [TI_TALL] = true, [TI_FLOWER] = true, [TI_WATER] = true,
};

static Tex load_tex(SDL_Renderer *r, const char *path, bool required)
{
    Tex t = { NULL, 0, 0 };
    FILE *f = fopen(path, "rb");
    if (!f) {
        if (required)
            fprintf(stderr, "missing asset: %s\n", path);
        return t;
    }
    char magic[4];
    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, "TEX1", 4) != 0) {
        fprintf(stderr, "bad asset magic: %s\n", path);
        fclose(f);
        return t;
    }
    uint8_t hdr[4];
    if (fread(hdr, 1, 4, f) != 4) { fclose(f); return t; }
    int w = hdr[0] | (hdr[1] << 8), h = hdr[2] | (hdr[3] << 8);
    if (w <= 0 || h <= 0 || w > 4096 || h > 4096) { fclose(f); return t; }
    size_t n = (size_t)w * h * 4;
    uint8_t *px = malloc(n);
    if (!px || fread(px, 1, n, f) != n) { free(px); fclose(f); return t; }
    fclose(f);
    t.tex = SDL_CreateTexture(r, SDL_PIXELFORMAT_ABGR8888,
                              SDL_TEXTUREACCESS_STATIC, w, h);
    if (!t.tex) { free(px); return t; }
    SDL_UpdateTexture(t.tex, NULL, px, w * 4);
    SDL_SetTextureBlendMode(t.tex, SDL_BLENDMODE_BLEND);
    free(px);
    t.w = w;
    t.h = h;
    return t;
}

int assets_load(SDL_Renderer *r)
{
    memset(&A, 0, sizeof(A));
    A.tiles = load_tex(r, "assets/tiles.tex", true);
    A.font = load_tex(r, "assets/font.tex", true);
    A.chars = load_tex(r, "assets/chars.tex", true);
    A.title = load_tex(r, "assets/title.tex", true);
    A.platforms = load_tex(r, "assets/platforms.tex", false);
    if (!A.tiles.tex || !A.font.tex || !A.chars.tex || !A.title.tex)
        return -1;
    for (int i = 0; i < NUM_SPECIES; i++) {
        char path[128];
        snprintf(path, sizeof(path), "assets/creatures/%s.tex", SPECIES_ART[i]);
        A.creature[i] = load_tex(r, path, false);
    }
    return 0;
}

void assets_free(void)
{
    SDL_DestroyTexture(A.tiles.tex);
    SDL_DestroyTexture(A.font.tex);
    SDL_DestroyTexture(A.chars.tex);
    SDL_DestroyTexture(A.title.tex);
    SDL_DestroyTexture(A.platforms.tex);
    for (int i = 0; i < NUM_SPECIES; i++)
        if (A.creature[i].tex)
            SDL_DestroyTexture(A.creature[i].tex);
    memset(&A, 0, sizeof(A));
}

void draw_tile(SDL_Renderer *r, int tile_id, int x, int y, uint32_t tick)
{
    if (tile_id < 0 || tile_id >= TI_COUNT || !A.tiles.tex)
        return;
    int col = TILE_COL[tile_id];
    if (TILE_ANIM[tile_id])
        col += (tick >> 5) & 1;
    SDL_Rect src = { col * 16, 0, 16, 16 };
    SDL_Rect dst = { x, y, 16, 16 };
    SDL_RenderCopy(r, A.tiles.tex, &src, &dst);
}

void draw_char(SDL_Renderer *r, int sprite_id, int dir, int frame, int x, int y)
{
    if (!A.chars.tex || sprite_id < 0 || sprite_id > 4)
        return;
    /* frames: down A/B = 0/1, up A/B = 2/3, side A/B = 4/5; right = flip side */
    int f;
    bool flip = false;
    switch (dir) {
    case 0: f = frame ? 1 : 0; break;
    case 1: f = frame ? 3 : 2; break;
    case 2: f = frame ? 5 : 4; break;
    default: f = frame ? 5 : 4; flip = true; break;
    }
    SDL_RendererFlip fl = flip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
    SDL_Rect src = { f * 16, sprite_id * 16, 16, 16 };
    SDL_Rect dst = { x, y, 16, 16 };
    SDL_RenderCopyEx(r, A.chars.tex, &src, &dst, 0, NULL, fl);
}

void draw_creature(SDL_Renderer *r, int species, int x, int y, int size, bool flip)
{
    if (species < 0 || species >= NUM_SPECIES || !A.creature[species].tex) {
        /* placeholder box */
        SDL_SetRenderDrawColor(r, 200, 60, 60, 255);
        SDL_Rect box = { x, y, size, size };
        SDL_RenderFillRect(r, &box);
        return;
    }
    extern const uint8_t SPECIES_TINT[NUM_SPECIES][3];
    const uint8_t *tint = SPECIES_TINT[species];
    if (tint[0] != 255 || tint[1] != 255 || tint[2] != 255)
        SDL_SetTextureColorMod(A.creature[species].tex, tint[0], tint[1], tint[2]);
    SDL_Rect src = { 0, 0, A.creature[species].w, A.creature[species].h };
    SDL_Rect dst = { x, y, size, size };
    SDL_RenderCopyEx(r, A.creature[species].tex, &src, &dst, 0, NULL,
                     flip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
    if (tint[0] != 255 || tint[1] != 255 || tint[2] != 255)
        SDL_SetTextureColorMod(A.creature[species].tex, 255, 255, 255);
}
