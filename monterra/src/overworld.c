#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "overworld.h"
#include "game.h"
#include "assets.h"
#include "text.h"
#include "ui.h"
#include "save.h"

/* ---- tile helpers ---- */
static int tile_at(int x, int y)
{
    const MapDef *m = &MAPS[g.map];
    if (x < 0 || y < 0 || x >= m->w || y >= m->h)
        return TI_TREE;
    return m->rows[y][x];
}

static bool tile_solid(int t)
{
    switch (t) {
    case '.': case ',': case '"': case 'P': case '=': case 'L':
    case 'D': case 'H': case 'M': case '_': case 'r': case 'g': case 's':
        return false;
    default:
        return true;
    }
}

static const NpcDef *npc_at(int x, int y)
{
    const MapDef *m = &MAPS[g.map];
    for (int i = 0; i < m->nnpcs; i++)
        if (m->npcs[i].x == x && m->npcs[i].y == y)
            return &m->npcs[i];
    return NULL;
}

static bool blocked(int x, int y)
{
    if (tile_solid(tile_at(x, y)))
        return true;
    if (npc_at(x, y))
        return true;
    return false;
}

/* ---- dialog after-actions ---- */
enum { AD_NONE, AD_TRAINER, AD_PROF, AD_NURSE, AD_SHOP };
static int after_dialog = AD_NONE;

static const TrainerDef *pending_trainer;
static const NpcDef *spotted_npc;
static int spotted_frames;

static void split_sign(const char *text, const char *lines[8], int *n)
{
    static char buf[128];
    snprintf(buf, sizeof(buf), "%s", text);
    int i = 0;
    char *p = buf;
    while (p && i < 8) {
        char *nl = strchr(p, '\n');
        if (nl)
            *nl = 0;
        lines[i++] = p;
        p = nl ? nl + 1 : NULL;
    }
    *n = i;
}

static void say(const char *const *lines)
{
    int n = 0;
    while (lines[n]) n++;
    dlg_start(lines, n);
}

/* ---- interactions ---- */
static void talk_prof(void)
{
    if (g.flags & FLAG_STARTER) {
        const char *post[] = {
            "Take good care of your partner!",
            "ROUTE 1 is just north of town.",
            "Good luck out there!",
        };
        dlg_start(post, 3);
        after_dialog = AD_NONE;
        return;
    }
    const char *intro[] = {
        "PROF. MAPLE: Ah, right on time!",
        "I study the wild creatures of",
        "the VELDT region. And you are",
        "setting out today, aren't you?",
    };
    const char *choices[] = { "EMBERIT", "DEWLIN", "SPROUTLE" };
    dlg_start_choice(intro, 4, choices, 3);
    after_dialog = AD_PROF;
}

static void talk_nurse(void)
{
    const char *intro[] = {
        "NURSE: Welcome to the CARE",
        "STATION! Shall I heal your",
        "creatures?",
    };
    const char *choices[] = { "YES", "NO" };
    dlg_start_choice(intro, 3, choices, 2);
    after_dialog = AD_NURSE;
}

static void talk_mom(void)
{
    heal_party();
    const char *lines[] = {
        "MOM: Off on an adventure?",
        "Let me tidy you up... there!",
        "Your creatures are fully",
        "rested. Be careful out there!",
    };
    dlg_start(lines, 4);
    after_dialog = AD_NONE;
}

static void talk_trainer(const NpcDef *npc)
{
    if (g.flags & npc->trainer->flag) {
        say(npc->trainer->post_lines);
        after_dialog = AD_NONE;
        return;
    }
    pending_trainer = npc->trainer;
    say(npc->lines);
    after_dialog = AD_TRAINER;
}

/* line-of-sight: trainers spot the player up to 4 tiles ahead */
static void check_trainer_sight(void)
{
    const MapDef *m = &MAPS[g.map];
    int ptx = (g.px + 8) / 16, pty = (g.py + 15) / 16;
    static const int sdx[4] = { 0, 0, -1, 1 };
    static const int sdy[4] = { 1, -1, 0, 0 };
    for (int i = 0; i < m->nnpcs; i++) {
        const NpcDef *n = &m->npcs[i];
        if (n->role != ROLE_TRAINER || !n->trainer)
            continue;
        if (g.flags & n->trainer->flag)
            continue;
        int x = n->x, y = n->y;
        for (int step = 0; step < 4; step++) {
            x += sdx[n->dir];
            y += sdy[n->dir];
            if (tile_solid(tile_at(x, y)) || npc_at(x, y))
                break;
            if (x == ptx && y == pty) {
                spotted_npc = n;
                spotted_frames = 36;
                return;
            }
        }
    }
}

static void interact(void)
{
    static const int dxs[4] = { 0, 0, -1, 1 };
    static const int dys[4] = { 1, -1, 0, 0 };
    int tx = (g.px + 8) / 16, ty = (g.py + 15) / 16; /* feet tile */
    int fx = tx + dxs[g.dir], fy = ty + dys[g.dir];

    const NpcDef *npc = npc_at(fx, fy);
    if (!npc && tile_at(fx, fy) == 'N') { /* talk over a counter */
        npc = npc_at(fx + dxs[g.dir], fy + dys[g.dir]);
    }
    if (npc) {
        switch (npc->role) {
        case ROLE_MOM: talk_mom(); return;
        case ROLE_NURSE: talk_nurse(); return;
        case ROLE_PROF: talk_prof(); return;
        case ROLE_TRAINER: talk_trainer(npc); return;
        case ROLE_SHOP: {
            const char *intro[] = {
                "CLERK: Welcome to the",
                "VERDAN MART! Stock up on",
                "supplies for the road!",
            };
            dlg_start(intro, 3);
            after_dialog = AD_SHOP;
            return;
        }
        default: break;
        }
        if (npc->lines) {
            say(npc->lines);
            after_dialog = AD_NONE;
        }
        return;
    }
    int t = tile_at(fx, fy);
    if (t == 'S') {
        const MapDef *m = &MAPS[g.map];
        for (int i = 0; i < m->nsigns; i++) {
            if (m->signs[i].x == fx && m->signs[i].y == fy) {
                const char *lines[8];
                int n;
                split_sign(m->signs[i].text, lines, &n);
                dlg_start(lines, n);
                return;
            }
        }
        return;
    }
    if (t == 'C') {
        const char *lines[] = { "It's a PC full of adventure", "notes and doodles." };
        dlg_start(lines, 2);
        return;
    }
    if (t == 'B') {
        const char *lines[] = { "A cozy bed. (Talk to MOM to", "rest up!)" };
        dlg_start(lines, 2);
        return;
    }
    if (t == 'W') {
        const char *lines[] = { "The water is calm and clear." };
        dlg_start(lines, 1);
        return;
    }
}

/* ---- warps + encounters ---- */
static struct { uint8_t map, x, y; } pending_warp;
static bool warp_pending;

static void warp_do(void)
{
    g.map = pending_warp.map;
    g.px = pending_warp.x * 16;
    g.py = pending_warp.y * 16;
    g.moving = 0;
    g.dx_steps = 0;
    g.hop = 0;
    spotted_frames = 0;
    spotted_npc = NULL;
}

static void check_warp(int tx, int ty)
{
    const MapDef *m = &MAPS[g.map];
    for (int i = 0; i < m->nwarps; i++) {
        if (m->warps[i].x == tx && m->warps[i].y == ty) {
            if (m->warps[i].dest_map == MAP_ROUTE1 && !(g.flags & FLAG_STARTER)) {
                const char *lines[] = {
                    "PROF. MAPLE was looking for",
                    "you! Visit her lab first!",
                };
                dlg_start(lines, 2);
                return;
            }
            pending_warp.map = m->warps[i].dest_map;
            pending_warp.x = m->warps[i].dest_x;
            pending_warp.y = m->warps[i].dest_y;
            warp_pending = true;
            start_fade(warp_do);
            return;
        }
    }
}

static void roll_encounter(void)
{
    const MapDef *m = &MAPS[g.map];
    if (m->nencs == 0 || m->enc_rate == 0)
        return;
    if (rand() % 100 >= m->enc_rate)
        return;
    int total = 0;
    for (int i = 0; i < m->nencs; i++)
        total += m->encs[i].weight;
    int roll = rand() % total;
    const Encounter *e = &m->encs[0];
    for (int i = 0; i < m->nencs; i++) {
        if (roll < m->encs[i].weight) {
            e = &m->encs[i];
            break;
        }
        roll -= m->encs[i].weight;
    }
    uint8_t lvl = e->minlvl + rand() % (e->maxlvl - e->minlvl + 1);
    g_battle_req.active = 1;
    g_battle_req.species = (uint8_t)e->species;
    g_battle_req.level = lvl;
    g_battle_req.trainer = NULL;
}

/* ---- root menu ---- */
static struct { bool open; int cursor; } menu;
static int pending_potion = -1;

static const char *const MENU_ITEMS[] = { "PARTNERS", "BAG", "CLOSE" };
#define MENU_N 3

/* ---- update ---- */
void ow_reset(uint8_t map, uint8_t x, uint8_t y)
{
    g.map = map;
    g.px = x * 16;
    g.py = y * 16;
    g.dir = 0;
    g.moving = 0;
    g.dx_steps = 0;
    g.hop = 0;
    menu.open = false;
    menu.cursor = 0;
    after_dialog = AD_NONE;
    pending_potion = -1;
}

static void handle_after_dialog(void)
{
    int result = g_dlg.result;
    switch (after_dialog) {
    case AD_TRAINER:
        after_dialog = AD_NONE;
        if (pending_trainer) {
            g_battle_req.active = 1;
            g_battle_req.species = 0;
            g_battle_req.level = 0;
            g_battle_req.trainer = pending_trainer;
            pending_trainer = NULL;
        }
        break;
    case AD_PROF: {
        after_dialog = AD_NONE;
        if (result < 0)
            break;
        static const uint8_t starters[3] = { SP_EMBERIT, SP_DEWLIN, SP_SPROUTLE };
        uint8_t sp = starters[result];
        if (g.party_n < MAX_PARTY) {
            creature_init(&g.party[g.party_n], sp, 5, 1);
            g.party_n++;
            dex_own(sp);
        }
        for (int i = 0; i < 5; i++)
            bag_add(IT_ORB);
        g.flags |= FLAG_STARTER;
        const char *give[] = {
            "PROF. MAPLE: Excellent choice!",
            "This little one is yours now!",
            "Oh - and take these 5 ORBs.",
            "Throw one at a wild creature",
            "to catch it!",
        };
        dlg_start(give, 5);
        break;
    }
    case AD_NURSE: {
        after_dialog = AD_NONE;
        if (result == 0) {
            heal_party();
            g.heal_map = MAP_HEAL;
            g.heal_x = 3;
            g.heal_y = 5;
            const char *healed[] = {
                "NURSE: ...All done!",
                "Your creatures are fully",
                "healed. Come back any time!",
            };
            dlg_start(healed, 3);
        } else if (result == -1) {
            const char *bye[] = { "NURSE: We hope to see you", "again!" };
            dlg_start(bye, 2);
        }
        break;
    }
    case AD_SHOP:
        after_dialog = AD_NONE;
        shop_open();
        break;
    default:
        break;
    }
}

static void try_move(void)
{
    int dx = 0, dy = 0;
    if (g_in.held[BTN_UP]) { dx = 0; dy = -1; g.dir = 1; }
    else if (g_in.held[BTN_DOWN]) { dx = 0; dy = 1; g.dir = 0; }
    else if (g_in.held[BTN_LEFT]) { dx = -1; dy = 0; g.dir = 2; }
    else if (g_in.held[BTN_RIGHT]) { dx = 1; dy = 0; g.dir = 3; }
    else
        return;

    int tx = (g.px + 8) / 16, ty = (g.py + 15) / 16;
    int nx = tx + dx, ny = ty + dy;
    int t = tile_at(nx, ny);

    /* ledge hop: moving down onto a ledge jumps two tiles */
    if (dy == 1 && t == 'L' && !blocked(nx, ny + 1)) {
        g.moving = 1;
        g.dx_steps = 32;
        g.dx = 0;
        g.dy = 1;
        g.hop = 1;
        return;
    }
    if (blocked(nx, ny))
        return;
    g.moving = 1;
    g.dx_steps = 16;
    g.dx = dx;
    g.dy = dy;
    g.hop = 0;
}

void ow_update(void)
{
    /* trainer spotted the player: pause with a "!" emote */
    if (spotted_frames > 0) {
        spotted_frames--;
        if (spotted_frames == 0 && spotted_npc) {
            pending_trainer = spotted_npc->trainer;
            say(spotted_npc->lines);
            after_dialog = AD_TRAINER;
        }
        return;
    }
    /* menus + dialogs take priority */
    if (dlg_active()) {
        dlg_update();
        if (g_dlg.done) {
            g_dlg.done = false;
            handle_after_dialog();
        }
        return;
    }
    if (party_active()) {
        party_update();
        if (!party_active()) {
            int slot = party_result();
            if (pending_potion >= 0 && slot >= 0) {
                Creature *c = &g.party[slot];
                uint16_t before = c->hp;
                uint16_t heal = (uint16_t)ITEMS[pending_potion].power;
                c->hp = (uint16_t)(c->hp + heal);
                if (c->hp > c->stats[ST_HP]) c->hp = c->stats[ST_HP];
                bag_consume((uint8_t)pending_potion);
                char buf[64];
                snprintf(buf, sizeof(buf), "%s recovered %u HP!",
                         SPECIES[c->species].name, c->hp - before);
                const char *lines[1];
                lines[0] = buf;
                dlg_start(lines, 1);
                pending_potion = -1;
            } else {
                pending_potion = -1;
            }
        }
        return;
    }
    if (bag_active()) {
        bag_update();
        if (!bag_active()) {
            int item = bag_result();
            if (ITEMS[item].kind == IK_HEAL && item >= 0) {
                pending_potion = item;
                party_open(PM_TARGET);
            } else if (item >= 0) {
                const char *lines[] = { "Can't use that here." };
                dlg_start(lines, 1);
            }
        }
        return;
    }
    if (shop_active()) {
        shop_update();
        return;
    }
    if (menu.open) {
        if (g_in.pressed[BTN_UP]) menu.cursor = (menu.cursor + MENU_N - 1) % MENU_N;
        if (g_in.pressed[BTN_DOWN]) menu.cursor = (menu.cursor + 1) % MENU_N;
        if (g_in.pressed[BTN_B] || g_in.pressed[BTN_START]) menu.open = false;
        if (g_in.pressed[BTN_A]) {
            menu.open = false;
            if (menu.cursor == 0) party_open(PM_VIEW);
            else if (menu.cursor == 1) bag_open(BM_OVERWORLD);
        }
        return;
    }
    if (g.moving) {
        g.px = (uint16_t)(g.px + g.dx * 2);
        g.py = (uint16_t)(g.py + g.dy * 2);
        g.dx_steps = (uint16_t)(g.dx_steps - 2);
        if (g.dx_steps <= 0) {
            g.moving = 0;
            g.dx_steps = 0;
            int hop = g.hop;
            g.hop = 0;
            int tx = (g.px + 8) / 16, ty = (g.py + 15) / 16;
            check_warp(tx, ty);
            if (!warp_pending && !hop)
                roll_encounter();
            if (!warp_pending && g_battle_req.active == 0)
                check_trainer_sight();
        }
        return;
    }
    if (g_in.pressed[BTN_A]) {
        interact();
        return;
    }
    if (g_in.pressed[BTN_START]) {
        menu.open = true;
        menu.cursor = 0;
        return;
    }
    try_move();
}

/* ---- draw ---- */
static void camera(int *cx, int *cy)
{
    const MapDef *m = &MAPS[g.map];
    int mw = m->w * 16, mh = m->h * 16;
    int pcx = g.px + 8 - SCREEN_W / 2;
    int pcy = g.py + 8 - SCREEN_H / 2;
    if (mw <= SCREEN_W) *cx = (mw - SCREEN_W) / 2;
    else *cx = pcx < 0 ? 0 : (pcx > mw - SCREEN_W ? mw - SCREEN_W : pcx);
    if (mh <= SCREEN_H) *cy = (mh - SCREEN_H) / 2;
    else *cy = pcy < 0 ? 0 : (pcy > mh - SCREEN_H ? mh - SCREEN_H : pcy);
}

static int tile_char_to_id(int c)
{
    switch (c) {
    case '.': return TI_GRASS;
    case '"': return TI_TALL;
    case 'P': return TI_PATH;
    case ',': return TI_FLOWER;
    case 'T': return TI_TREE;
    case 'W': return TI_WATER;
    case 'F': return TI_FENCE;
    case 'R': return TI_ROCK;
    case 's': return TI_SAND;
    case '=': return TI_BRIDGE;
    case 'L': return TI_LEDGE;
    case 'S': return TI_SIGN;
    case '^': return TI_ROOF;
    case '(': return TI_ROOF_L;
    case ')': return TI_ROOF_R;
    case '#': return TI_WALL;
    case 'O': return TI_WINDOW;
    case 'D': return TI_DOOR;
    case 'H': return TI_DOOR_HEAL;
    case '_': return TI_FLOOR;
    case 'I': return TI_WALL_IN;
    case 'B': return TI_BED;
    case 't': return TI_TABLE;
    case 'K': return TI_SHELF;
    case 'M': return TI_MAT;
    case 'C': return TI_PC;
    case 'N': return TI_COUNTER;
    case 'p': return TI_PLANT;
    case 'r': return TI_CARPET;
    case 'g': return TI_GRASS_TOWN;
    default: return TI_GRASS;
    }
}

static void draw_entity(SDL_Renderer *r, int sprite_id, int dir, int frame,
                        int px, int py, int cx, int cy)
{
    draw_char(r, sprite_id, dir, frame, px - cx, py - cy);
}

void ow_draw(SDL_Renderer *r)
{
    const MapDef *m = &MAPS[g.map];
    int cx, cy;
    camera(&cx, &cy);
    SDL_SetRenderDrawColor(r, 16, 16, 24, 255);
    SDL_RenderClear(r);

    int x0 = cx / 16, y0 = cy / 16;
    int x1 = (cx + SCREEN_W) / 16, y1 = (cy + SCREEN_H) / 16;
    for (int ty = y0; ty <= y1; ty++) {
        for (int tx = x0; tx <= x1; tx++) {
            int t = tile_at(tx, ty);
            draw_tile(r, tile_char_to_id(t), tx * 16 - cx, ty * 16 - cy, g.tick);
        }
    }
    /* entities sorted by y */
    int hop_off = 0;
    if (g.moving && g.hop) {
        int prog = 32 - g.dx_steps; /* 0..32 */
        float s = 3.14159265f * prog / 32.0f;
        hop_off = (int)(-(8.0f * s * (3.14159265f - s)) / 3.0f); /* arc */
    }
    /* npcs */
    for (int i = 0; i < m->nnpcs; i++) {
        const NpcDef *n = &m->npcs[i];
        if (n->y * 16 < g.py)
            draw_entity(r, n->sprite, n->dir, 0, n->x * 16, n->y * 16, cx, cy);
    }
    int pframe = g.moving ? ((g.tick >> 3) & 1) : 0;
    draw_entity(r, 0, g.dir, pframe, g.px, g.py + hop_off, cx, cy);
    for (int i = 0; i < m->nnpcs; i++) {
        const NpcDef *n = &m->npcs[i];
        if (n->y * 16 >= g.py)
            draw_entity(r, n->sprite, n->dir, 0, n->x * 16, n->y * 16, cx, cy);
    }

    if (spotted_frames > 0 && spotted_npc) {
        int ex = spotted_npc->x * 16 - cx + 3;
        int ey = spotted_npc->y * 16 - cy - 13;
        SDL_Color red = { 200, 48, 48, 255 };
        draw_panel(r, ex, ey, 10, 12);
        draw_text(r, ex + 1, ey + 2, "!", red, 1);
    }

    if (menu.open) {
        int w = 92, h = MENU_N * 14 + 12;
        draw_panel(r, SCREEN_W - w - 4, 4, w, h);
        SDL_Color dark = { 56, 56, 64, 255 };
        SDL_Color red = { 200, 48, 48, 255 };
        for (int i = 0; i < MENU_N; i++) {
            if (i == menu.cursor)
                draw_text(r, SCREEN_W - w + 2, 10 + i * 14, ">", red, 1);
            draw_text(r, SCREEN_W - w + 12, 10 + i * 14, MENU_ITEMS[i], dark, 1);
        }
    }

    dlg_draw(r);
    party_draw(r);
    bag_draw(r);
    shop_draw(r);
}
