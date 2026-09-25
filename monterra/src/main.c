#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <SDL.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include "game.h"
#include "audio.h"
#include "data/music.h"
#include "assets.h"
#include "text.h"
#include "ui.h"
#include "overworld.h"
#include "battle.h"
#include "save.h"

Input g_in;

static SDL_Renderer *rend;
static bool running = true;

/* ---- global helpers ---- */
void bag_add(uint8_t item)
{
    if (g.bag_n < MAX_BAG)
        g.bag[g.bag_n++] = item;
}

int bag_count(uint8_t item)
{
    int n = 0;
    for (int i = 0; i < g.bag_n; i++)
        if (g.bag[i] == item)
            n++;
    return n;
}

void bag_consume(uint8_t item)
{
    for (int i = 0; i < g.bag_n; i++) {
        if (g.bag[i] == item) {
            for (int j = i; j < g.bag_n - 1; j++)
                g.bag[j] = g.bag[j + 1];
            g.bag_n--;
            return;
        }
    }
}

void heal_party(void)
{
    for (int i = 0; i < g.party_n; i++) {
        Creature *c = &g.party[i];
        c->hp = c->stats[ST_HP];
        c->ailment = AIL_NONE;
        c->sleep_turns = 0;
        c->conf_turns = 0;
        for (int m = 0; m < 4; m++)
            if (c->moves[m] != 0xFF)
                c->pp[m] = MOVES[c->moves[m]].pp;
    }
}

void dex_see(uint8_t species)
{
    if (species < NUM_SPECIES_TOTAL)
        g.dex_seen[species / 8] |= (uint8_t)(1u << (species % 8));
}

void dex_own(uint8_t species)
{
    if (species < NUM_SPECIES_TOTAL) {
        g.dex_seen[species / 8] |= (uint8_t)(1u << (species % 8));
        g.dex_caught[species / 8] |= (uint8_t)(1u << (species % 8));
    }
}

void start_fade(void (*cb)(void))
{
    if (g.fade_dir != 0)
        return;
    g.fade_dir = 1;
    g.fade_cb = cb;
}

/* ---- game setup ---- */
static void game_reset(void)
{
    memset(&g, 0, sizeof(g));
    g.mode = MODE_OVERWORLD;
    g.map = MAP_HOUSE;
    g.px = 5 * 16;
    g.py = 6 * 16;
    g.dir = 0;
    g.money = 1500;
    g.party_n = 0;
    g.active_slot = 0;
    g.heal_map = MAP_HOUSE;
    g.heal_x = 5;
    g.heal_y = 5;
    g_battle_req.active = 0;
    ow_reset(MAP_HOUSE, 5, 6);
}

/* ---- battle orchestration ---- */
static BattleRequest pending_req;

static void cb_start_battle(void)
{
    if (pending_req.trainer)
        battle_start_trainer(pending_req.trainer);
    else
        battle_start_wild(pending_req.species, pending_req.level);
    g.mode = MODE_BATTLE;
}

static void cb_end_battle(void)
{
    uint8_t result = battle_result();
    if (result == 3)
        audio_play_jingle(MUS_VICTORY);
    g.mode = MODE_OVERWORLD;
    if (result == 1) { /* whiteout */
        heal_party();
        ow_reset(g.heal_map, g.heal_x, g.heal_y);
    } else if (result == 3) {
        g.flags |= battle_trainer_flag();
    }
}

/* ---- input ---- */
static const struct { SDL_Keycode key; int btn; } KEYMAP[] = {
    { SDLK_UP, BTN_UP }, { SDLK_KP_8, BTN_UP }, { SDLK_w, BTN_UP },
    { SDLK_DOWN, BTN_DOWN }, { SDLK_KP_2, BTN_DOWN }, { SDLK_s, BTN_DOWN },
    { SDLK_LEFT, BTN_LEFT }, { SDLK_KP_4, BTN_LEFT }, { SDLK_a, BTN_LEFT },
    { SDLK_RIGHT, BTN_RIGHT }, { SDLK_KP_6, BTN_RIGHT }, { SDLK_d, BTN_RIGHT },
    { SDLK_z, BTN_A }, { SDLK_RETURN, BTN_A }, { SDLK_KP_ENTER, BTN_A },
    { SDLK_SPACE, BTN_A },
    { SDLK_x, BTN_B }, { SDLK_ESCAPE, BTN_B }, { SDLK_BACKSPACE, BTN_B },
    { SDLK_m, BTN_START }, { SDLK_TAB, BTN_START },
};
#define KEYMAP_N (int)(sizeof(KEYMAP) / sizeof(KEYMAP[0]))

static void input_update(void)
{
    memset(g_in.pressed, 0, sizeof(g_in.pressed));
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
        case SDL_QUIT:
            running = false;
            break;
        case SDL_KEYDOWN: {
            for (int i = 0; i < KEYMAP_N; i++) {
                if (ev.key.keysym.sym == KEYMAP[i].key) {
                    if (!g_in.held[KEYMAP[i].btn])
                        g_in.pressed[KEYMAP[i].btn] = true;
                    g_in.held[KEYMAP[i].btn] = true;
                }
            }
            break;
        }
        case SDL_KEYUP:
            for (int i = 0; i < KEYMAP_N; i++)
                if (ev.key.keysym.sym == KEYMAP[i].key)
                    g_in.held[KEYMAP[i].btn] = false;
            break;
        default:
            break;
        }
    }
}

/* ---- fade ---- */
static void fade_update(void)
{
    if (g.fade_dir == 0)
        return;
    g.fade = (uint8_t)(g.fade + (uint8_t)(g.fade_dir > 0 ? 1 : -1) * 2);
    if (g.fade_dir > 0 && g.fade >= 16) {
        g.fade = 16;
        if (g.fade_cb) {
            void (*cb)(void) = g.fade_cb;
            g.fade_cb = NULL;
            g.fade_dir = -1;
            cb();
            return;
        }
        g.fade_dir = -1;
    } else if (g.fade_dir < 0 && g.fade == 0) {
        g.fade_dir = 0;
    }
}

static void fade_draw(SDL_Renderer *r)
{
    if (g.fade == 0)
        return;
    int a = g.fade * 16; /* 0..256 */
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, 8, 8, 12, a > 255 ? 255 : a);
    SDL_Rect full = { 0, 0, SCREEN_W, SCREEN_H };
    SDL_RenderFillRect(r, &full);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

/* ---- title ---- */
static void cb_title_start(void)
{
    game_reset();
}

static void title_update(void)
{
    if (g_in.pressed[BTN_A] && g.fade_dir == 0) {
        start_fade(cb_title_start);
    }
}

static void title_draw(SDL_Renderer *r)
{
    if (A.title.tex) {
        SDL_Rect dst = { 0, 0, SCREEN_W, SCREEN_H };
        SDL_RenderCopy(r, A.title.tex, NULL, &dst);
    } else {
        SDL_SetRenderDrawColor(r, 24, 20, 72, 255);
        SDL_RenderClear(r);
    }
    SDL_Color white = { 250, 248, 240, 255 };
    SDL_Color gold = { 248, 208, 96, 255 };
    draw_text_shadow(r, 28, 34, "MONTERRA", gold, 3);
    draw_text_shadow(r, 44, 66, "LEGENDS OF THE VELDT", white, 1);
    if ((g.tick >> 4) & 1)
        draw_text_shadow(r, 72, 104, "PRESS Z", white, 1);
    draw_text_shadow(r, 20, 146, "AN ORIGINAL MONSTER RPG", white, 1);
}

/* ---- frame ---- */
static void frame(void)
{
    input_update();
    if (!running)
        return;
    g.tick++;

    if (g.fade_dir != 0) {
        fade_update();
    } else {
        /* battle request handling */
        if (g_battle_req.active && g.mode == MODE_OVERWORLD) {
            pending_req = g_battle_req;
            g_battle_req.active = 0;
            start_fade(cb_start_battle);
        } else if (g.mode == MODE_BATTLE && battle_over()) {
            start_fade(cb_end_battle);
        } else if (g.mode == MODE_TITLE) {
            title_update();
        } else if (g.mode == MODE_OVERWORLD) {
            ow_update();
        } else if (g.mode == MODE_BATTLE) {
            battle_update();
        }
        fade_update();
    }

    /* music follows game context */
    if (g.mode == MODE_TITLE)
        audio_play_music(MUS_TITLE);
    else if (g.mode == MODE_OVERWORLD)
        audio_play_music(music_for_map(g.map));

    /* render */
    if (g.mode == MODE_TITLE)
        title_draw(rend);
    else if (g.mode == MODE_OVERWORLD)
        ow_draw(rend);
    else if (g.mode == MODE_BATTLE)
        battle_draw(rend);
    fade_draw(rend);
    SDL_RenderPresent(rend);
#ifndef __EMSCRIPTEN__
    SDL_Delay(16);
#endif
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        /* keep running without sound if only audio failed */
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
                fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
            return 1;
        }
    }
    audio_init();
    srand((unsigned)time(NULL));
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_Window *win = SDL_CreateWindow("MONTERRA",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W * 3, SCREEN_H * 3,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!win) {
        fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        return 1;
    }
    rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED |
                                        SDL_RENDERER_PRESENTVSYNC);
    if (!rend)
        rend = SDL_CreateRenderer(win, -1, 0);
    SDL_RenderSetLogicalSize(rend, SCREEN_W, SCREEN_H);

    if (assets_load(rend) != 0) {
        fprintf(stderr, "asset load failed (run from the monterra dir)\n");
        return 1;
    }

    memset(&g, 0, sizeof(g));
    g.mode = MODE_TITLE;
    g.fade = 16;
    g.fade_dir = -1; /* fade in from black */

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(frame, 60, 1);
#else
    while (running)
        frame();
#endif

    assets_free();
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);
    audio_quit();
    SDL_Quit();
    return 0;
}
