/* Shared UI: dialog box, choice menu, panels, HP bars, party + bag menus. */
#ifndef UI_H
#define UI_H

#include <SDL.h>
#include "game.h"

/* ---- dialog ---- */
typedef struct {
    const char *lines[8];
    int nlines;
    int page;   /* 2 lines per page */
    int chars;  /* typewriter reveal count on current page */
    bool active;
    bool done;
    /* optional choice menu after the last page */
    bool choice;
    const char *choices[4];
    int nchoices;
    int cchoice;
    int result; /* -1 cancelled, else index */
} Dialog;

extern Dialog g_dlg;

void dlg_start(const char *const *lines, int n);
void dlg_start_choice(const char *const *lines, int n,
                      const char *const *choices, int nch);
void dlg_update(void);
void dlg_draw(SDL_Renderer *r);
bool dlg_active(void);

/* ---- panels & bars ---- */
void draw_panel(SDL_Renderer *r, int x, int y, int w, int h);
void draw_textbox(SDL_Renderer *r);
void draw_hpbar(SDL_Renderer *r, int x, int y, int w, uint16_t cur, uint16_t max);
void draw_xpbar(SDL_Renderer *r, int x, int y, int w, const Creature *c);

/* ---- party menu ---- */
enum { PM_OFF, PM_VIEW, PM_SWITCH, PM_TARGET };
void party_open(int mode);
void party_update(void);
void party_draw(SDL_Renderer *r);
bool party_active(void);
int party_result(void); /* chosen slot, -1 cancelled, -2 nothing to pick */

/* ---- bag menu ---- */
enum { BM_OFF, BM_OVERWORLD, BM_BATTLE };
void bag_open(int mode);
void bag_update(void);
void bag_draw(SDL_Renderer *r);
bool bag_active(void);
int bag_result(void); /* item id picked, -1 cancelled */

/* ---- shop ---- */
void shop_open(void);
bool shop_active(void);
void shop_update(void);
void shop_draw(SDL_Renderer *r);

#endif
