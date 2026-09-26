#include <string.h>
#include <stdio.h>
#include "ui.h"
#include "audio.h"
#include "data/music.h"
#include "text.h"
#include "assets.h"

Dialog g_dlg;

#define TB_X 2
#define TB_Y (SCREEN_H - TEXTBOX_H - 2)
#define TB_W (SCREEN_W - 4)
#define TB_H TEXTBOX_H

/* ---------------------------------------------------------------- dialog */
void dlg_start(const char *const *lines, int n)
{
    memset(&g_dlg, 0, sizeof(g_dlg));
    if (n > 8) n = 8;
    g_dlg.nlines = n;
    for (int i = 0; i < n; i++)
        g_dlg.lines[i] = lines[i];
    g_dlg.active = true;
    g_dlg.done = false;
    g_dlg.result = -2; /* not finished */
}

void dlg_start_choice(const char *const *lines, int n,
                      const char *const *choices, int nch)
{
    dlg_start(lines, n);
    g_dlg.choice = true;
    g_dlg.nchoices = nch > 4 ? 4 : nch;
    for (int i = 0; i < g_dlg.nchoices; i++)
        g_dlg.choices[i] = choices[i];
    g_dlg.cchoice = 0;
}

static int page_len(void)
{
    int start = g_dlg.page * 2;
    int n = g_dlg.nlines - start;
    return n > 2 ? 2 : n;
}

static int page_total_chars(void)
{
    int len = page_len(), total = 0;
    for (int i = 0; i < len; i++)
        total += (int)strlen(g_dlg.lines[g_dlg.page * 2 + i]);
    return total;
}

void dlg_update(void)
{
    if (!g_dlg.active)
        return;
    int total = page_total_chars();
    int last_page = (g_dlg.page * 2 + page_len() >= g_dlg.nlines);

    if (g_dlg.chars < total) {
        g_dlg.chars += 2;
        if (g_dlg.chars > total)
            g_dlg.chars = total;
        if (g_in.pressed[BTN_A])
            g_dlg.chars = total;
        return;
    }
    if (g_dlg.choice && last_page) {
        if (g_in.pressed[BTN_UP]) g_dlg.cchoice--;
        if (g_in.pressed[BTN_DOWN]) g_dlg.cchoice++;
        if (g_dlg.cchoice < 0) g_dlg.cchoice = g_dlg.nchoices - 1;
        if (g_dlg.cchoice >= g_dlg.nchoices) g_dlg.cchoice = 0;
        if (g_in.pressed[BTN_A]) {
            g_dlg.result = g_dlg.cchoice;
            g_dlg.active = false;
            g_dlg.done = true;
        } else if (g_in.pressed[BTN_B]) {
            g_dlg.result = -1;
            g_dlg.active = false;
            g_dlg.done = true;
        }
        return;
    }
    if (g_in.pressed[BTN_A] || g_in.pressed[BTN_B]) {
        if (last_page) {
            g_dlg.active = false;
            g_dlg.done = true;
            g_dlg.result = -1;
        } else {
            g_dlg.page++;
            g_dlg.chars = 0;
        }
    }
}

void dlg_draw(SDL_Renderer *r)
{
    if (!g_dlg.active)
        return;
    draw_textbox(r);
    int len = page_len();
    int remaining = g_dlg.chars;
    SDL_Color dark = { 56, 56, 64, 255 };
    for (int i = 0; i < len && remaining > 0; i++) {
        const char *line = g_dlg.lines[g_dlg.page * 2 + i];
        int n = (int)strlen(line);
        int show = remaining < n ? remaining : n;
        char buf[40];
        if (show > 38) show = 38;
        memcpy(buf, line, (size_t)show);
        buf[show] = 0;
        draw_text(r, TB_X + 8, TB_Y + 7 + i * 14, buf, dark, 1);
        remaining -= n;
    }
    int last_page = (g_dlg.page * 2 + len >= g_dlg.nlines);
    bool revealed = g_dlg.chars >= page_total_chars();
    if (revealed && !(g_dlg.choice && last_page)) {
        SDL_Rect arrow = { TB_X + TB_W - 12, TB_Y + TB_H - 9, 6, 3 };
        if ((g.tick >> 4) & 1)
            arrow.y += 2;
        SDL_SetRenderDrawColor(r, 56, 56, 64, 255);
        SDL_RenderFillRect(r, &arrow);
    }
    if (g_dlg.choice && last_page && revealed) {
        int w = 0;
        for (int i = 0; i < g_dlg.nchoices; i++) {
            int tw = text_width(g_dlg.choices[i], 1);
            if (tw > w) w = tw;
        }
        w += 26;
        int h = g_dlg.nchoices * 14 + 10;
        int x = TB_X + TB_W - w - 4;
        int y = TB_Y - h - 2;
        draw_panel(r, x, y, w, h);
        SDL_Color red = { 200, 48, 48, 255 };
        for (int i = 0; i < g_dlg.nchoices; i++) {
            if (i == g_dlg.cchoice)
                draw_text(r, x + 14, y + 6 + i * 14, ">", red, 1);
            draw_text(r, x + 24, y + 6 + i * 14, g_dlg.choices[i], dark, 1);
        }
    }
}

bool dlg_active(void)
{
    return g_dlg.active;
}

/* ---------------------------------------------------------------- panels */
void draw_panel(SDL_Renderer *r, int x, int y, int w, int h)
{
    SDL_SetRenderDrawColor(r, 248, 248, 248, 255);
    SDL_Rect body = { x, y, w, h };
    SDL_RenderFillRect(r, &body);
    SDL_SetRenderDrawColor(r, 24, 24, 32, 255);
    SDL_Rect b1 = { x, y, w, 1 }, b2 = { x, y + h - 1, w, 1 };
    SDL_Rect b3 = { x, y, 1, h }, b4 = { x + w - 1, y, 1, h };
    SDL_RenderFillRect(r, &b1);
    SDL_RenderFillRect(r, &b2);
    SDL_RenderFillRect(r, &b3);
    SDL_RenderFillRect(r, &b4);
    SDL_SetRenderDrawColor(r, 168, 168, 184, 255);
    SDL_Rect i1 = { x + 2, y + 2, w - 4, 1 };
    SDL_RenderFillRect(r, &i1);
}

void draw_textbox(SDL_Renderer *r)
{
    draw_panel(r, TB_X, TB_Y, TB_W, TB_H);
}

void draw_hpbar(SDL_Renderer *r, int x, int y, int w, uint16_t cur, uint16_t max)
{
    SDL_Color dark = { 56, 56, 64, 255 };
    draw_text(r, x, y - 1, "HP", dark, 1);
    int bx = x + 18, bw = w - 18;
    SDL_SetRenderDrawColor(r, 24, 24, 32, 255);
    SDL_Rect border = { bx, y, bw, 5 };
    SDL_RenderFillRect(r, &border);
    SDL_SetRenderDrawColor(r, 96, 96, 112, 255);
    SDL_Rect track = { bx + 1, y + 1, bw - 2, 3 };
    SDL_RenderFillRect(r, &track);
    uint32_t pct = max ? (uint32_t)cur * (uint32_t)(bw - 2) / max : 0;
    if (pct > (uint32_t)(bw - 2)) pct = (uint32_t)(bw - 2);
    uint8_t rr, gg;
    if ((uint32_t)cur * 2 > max) { rr = 32; gg = 168; }
    else if ((uint32_t)cur * 4 > max) { rr = 216; gg = 168; }
    else { rr = 208; gg = 48; }
    SDL_SetRenderDrawColor(r, rr, gg, 40, 255);
    SDL_Rect fill = { bx + 1, y + 1, (int)pct, 3 };
    if (pct > 0)
        SDL_RenderFillRect(r, &fill);
}

void draw_xpbar(SDL_Renderer *r, int x, int y, int w, const Creature *c)
{
    uint32_t pct = 0;
    if (c->level >= 100) {
        pct = (uint32_t)w;
    } else {
        uint16_t base = xp_for_level(c->level);
        uint16_t next = xp_for_level((uint8_t)(c->level + 1));
        if (next > base)
            pct = (uint32_t)(c->xp - base) * (uint32_t)w / (uint32_t)(next - base);
    }
    if (pct > (uint32_t)w) pct = w;
    SDL_SetRenderDrawColor(r, 168, 168, 184, 255);
    SDL_Rect track = { x, y, w, 2 };
    SDL_RenderFillRect(r, &track);
    if (pct > 0) {
        SDL_SetRenderDrawColor(r, 64, 136, 240, 255);
        SDL_Rect fill = { x, y, (int)pct, 2 };
        SDL_RenderFillRect(r, &fill);
    }
}

/* ---------------------------------------------------------------- party */
static struct {
    int mode;
    int cursor;
    int result;
    bool active;
} pm;

void party_open(int mode)
{
    pm.mode = mode;
    pm.cursor = 0;
    pm.result = -2;
    pm.active = true;
}

bool party_active(void) { return pm.active; }
int party_result(void) { return pm.result; }

static int pickable(int slot)
{
    if (slot >= g.party_n)
        return 0;
    if (pm.mode == PM_SWITCH)
        return slot != g.active_slot && g.party[slot].hp > 0;
    if (pm.mode == PM_TARGET)
        return g.party[slot].hp > 0;
    return 1;
}

void party_update(void)
{
    if (!pm.active)
        return;
    /* bounded cursor movement: never loops when nothing is pickable
     * (fixes the freeze when opening TEAM in battle with no healthy
     * switchable partners) */
    int exclude = (pm.mode == PM_SWITCH) ? g.active_slot : -1;
    bool need_hp = (pm.mode == PM_SWITCH || pm.mode == PM_TARGET);
    if (g_in.pressed[BTN_UP]) {
        int c = party_pick_cursor(g.party, g.party_n, exclude, need_hp,
                                  pm.cursor, -1);
        if (c >= 0) { pm.cursor = c; audio_sfx(SFX_BLIP); }
    }
    if (g_in.pressed[BTN_DOWN]) {
        int c = party_pick_cursor(g.party, g.party_n, exclude, need_hp,
                                  pm.cursor, 1);
        if (c >= 0) { pm.cursor = c; audio_sfx(SFX_BLIP); }
    }
    if (g_in.pressed[BTN_A] && pickable(pm.cursor)) {
        audio_sfx(SFX_CONFIRM);
        pm.result = pm.cursor;
        pm.active = false;
    } else if (g_in.pressed[BTN_B]) {
        pm.result = -1;
        pm.active = false;
    }
}

void party_draw(SDL_Renderer *r)
{
    if (!pm.active)
        return;
    draw_panel(r, 2, 2, SCREEN_W - 4, SCREEN_H - 4);
    SDL_Color dark = { 56, 56, 64, 255 };
    SDL_Color red = { 200, 48, 48, 255 };
    const char *title = (pm.mode == PM_VIEW) ? "PARTNERS" : "CHOOSE ONE!";
    draw_text(r, 10, 8, title, dark, 1);
    for (int i = 0; i < g.party_n && i < MAX_PARTY; i++) {
        const Creature *c = &g.party[i];
        int y = 20 + i * 20;
        if (i == pm.cursor) {
            SDL_SetRenderDrawColor(r, 232, 240, 248, 255);
            SDL_Rect hl = { 6, y - 2, SCREEN_W - 12, 19 };
            SDL_RenderFillRect(r, &hl);
            draw_text(r, 10, y + 1, ">", red, 1);
        }
        char buf[32];
        snprintf(buf, sizeof(buf), "%s", SPECIES[c->species].name);
        draw_text(r, 20, y + 1, buf, dark, 1);
        snprintf(buf, sizeof(buf), "Lv%-3u", c->level);
        draw_text(r, 130, y + 1, buf, dark, 1);
        snprintf(buf, sizeof(buf), "%3u/%-3u", c->hp, c->stats[ST_HP]);
        draw_text(r, 20, y + 12, buf, dark, 1);
        if (c->ailment == AIL_BURN) draw_text(r, 86, y + 12, "BRN", (SDL_Color){ 224, 96, 32, 255 }, 1);
        else if (c->ailment == AIL_PARA) draw_text(r, 86, y + 12, "PAR", (SDL_Color){ 200, 176, 32, 255 }, 1);
        else if (c->ailment == AIL_SLEEP) draw_text(r, 86, y + 12, "SLP", (SDL_Color){ 120, 120, 168, 255 }, 1);
        else if (c->ailment == AIL_POISON) draw_text(r, 86, y + 12, "PSN", (SDL_Color){ 168, 72, 208, 255 }, 1);
        else if (c->ailment == AIL_FREEZE) draw_text(r, 86, y + 12, "FRZ", (SDL_Color){ 96, 184, 224, 255 }, 1);
        else if (c->conf_turns > 0) draw_text(r, 86, y + 12, "CNF", (SDL_Color){ 208, 144, 48, 255 }, 1);
        draw_hpbar(r, 140, y + 13, 90, c->hp, c->stats[ST_HP]);
    }
    if (pm.mode == PM_SWITCH &&
        !party_any_pickable(g.party, g.party_n, g.active_slot, true)) {
        SDL_Color warn = { 176, 48, 48, 255 };
        draw_text(r, 10, SCREEN_H - 12, "NO HEALTHY SWAP! X:BACK", warn, 1);
    } else {
        draw_text(r, 10, SCREEN_H - 12, "Z:OK  X:BACK", dark, 1);
    }
}

/* ---------------------------------------------------------------- bag */
static struct {
    int mode;
    int cursor;
    int result;
    bool active;
} bm;

void bag_open(int mode)
{
    bm.mode = mode;
    bm.cursor = 0;
    bm.result = -2;
    bm.active = true;
}

bool bag_active(void) { return bm.active; }
int bag_result(void) { return bm.result; }

void bag_update(void)
{
    if (!bm.active)
        return;
    int n = g.bag_n;
    if (n > 0) {
        if (g_in.pressed[BTN_UP]) { bm.cursor = (bm.cursor + n - 1) % n; audio_sfx(SFX_BLIP); }
        if (g_in.pressed[BTN_DOWN]) { bm.cursor = (bm.cursor + 1) % n; audio_sfx(SFX_BLIP); }
    }
    if (g_in.pressed[BTN_A] && n > 0) {
        audio_sfx(SFX_CONFIRM);
        bm.result = g.bag[bm.cursor];
        bm.active = false;
    } else if (g_in.pressed[BTN_B]) {
        bm.result = -1;
        bm.active = false;
    }
}

void bag_draw(SDL_Renderer *r)
{
    if (!bm.active)
        return;
    draw_panel(r, 2, 2, SCREEN_W - 4, SCREEN_H - 4);
    SDL_Color dark = { 56, 56, 64, 255 };
    SDL_Color red = { 200, 48, 48, 255 };
    draw_text(r, 10, 8, "BAG", dark, 1);
    if (g.bag_n == 0)
        draw_text(r, 20, 30, "It's empty...", dark, 1);
    for (int i = 0; i < g.bag_n; i++) {
        int y = 24 + i * 16;
        if (i == bm.cursor)
            draw_text(r, 10, y, ">", red, 1);
        char buf[40];
        snprintf(buf, sizeof(buf), "%-14s x%u", ITEMS[g.bag[i]].name,
                 bag_count(g.bag[i]));
        draw_text(r, 22, y, buf, dark, 1);
    }
    char buf[24];
    snprintf(buf, sizeof(buf), "MONEY $%u", g.money);
    draw_text(r, 10, SCREEN_H - 12, buf, dark, 1);
}

/* ---------------------------------------------------------------- shop */
static struct {
    bool active;
    int cursor;
    int msg_frames;
    char msg[40];
} sm;

void shop_open(void)
{
    sm.active = true;
    sm.cursor = 0;
    sm.msg[0] = 0;
    sm.msg_frames = 0;
}

bool shop_active(void) { return sm.active; }

void shop_update(void)
{
    if (!sm.active)
        return;
    if (sm.msg_frames > 0)
        sm.msg_frames--;
    else
        sm.msg[0] = 0;
    if (g_in.pressed[BTN_UP]) {
        sm.cursor = (sm.cursor + NUM_ITEM_IDS - 1) % NUM_ITEM_IDS;
        audio_sfx(SFX_BLIP);
    }
    if (g_in.pressed[BTN_DOWN]) {
        sm.cursor = (sm.cursor + 1) % NUM_ITEM_IDS;
        audio_sfx(SFX_BLIP);
    }
    if (g_in.pressed[BTN_A]) {
        uint16_t price = ITEMS[sm.cursor].price;
        if (g.money < price) {
            snprintf(sm.msg, sizeof(sm.msg), "Not enough money!");
        } else if (g.bag_n >= MAX_BAG) {
            snprintf(sm.msg, sizeof(sm.msg), "Bag is full!");
        } else {
            bag_add((uint8_t)sm.cursor);
            g.money = (uint16_t)(g.money - price);
            snprintf(sm.msg, sizeof(sm.msg), "Here you go! Thanks!");
        }
        sm.msg_frames = 90;
    } else if (g_in.pressed[BTN_B]) {
        sm.active = false;
    }
}

void shop_draw(SDL_Renderer *r)
{
    if (!sm.active)
        return;
    draw_panel(r, 2, 2, SCREEN_W - 4, SCREEN_H - 4);
    SDL_Color dark = { 56, 56, 64, 255 };
    SDL_Color red = { 200, 48, 48, 255 };
    draw_text(r, 10, 8, "MART", dark, 1);
    for (int i = 0; i < NUM_ITEM_IDS; i++) {
        int y = 26 + i * 18;
        if (i == sm.cursor)
            draw_text(r, 10, y, ">", red, 1);
        char buf[48];
        snprintf(buf, sizeof(buf), "%-13s $%u", ITEMS[i].name, ITEMS[i].price);
        draw_text(r, 22, y, buf, dark, 1);
        snprintf(buf, sizeof(buf), "HAVE x%u", bag_count((uint8_t)i));
        draw_text(r, 168, y, buf, dark, 1);
    }
    char buf[40];
    snprintf(buf, sizeof(buf), "MONEY $%u", g.money);
    draw_text(r, 10, SCREEN_H - 22, buf, dark, 1);
    draw_text(r, 10, SCREEN_H - 12, "Z:BUY  X:EXIT", dark, 1);
    if (sm.msg[0])
        draw_text(r, 10, SCREEN_H - 34, sm.msg, red, 1);
}


/* ---- PC storage ---- */
static struct {
    bool active;
    int cursor;
    char msg[48];
    int msg_frames;
} stm;

void storage_open(void)
{
    stm.active = true;
    stm.cursor = 0;
    stm.msg[0] = 0;
    stm.msg_frames = 0;
}

bool storage_active(void)
{
    return stm.active;
}

void storage_update(void)
{
    if (!stm.active)
        return;
    if (stm.msg_frames > 0)
        stm.msg_frames--;
    else
        stm.msg[0] = 0;
    int total = g.party_n + g.storage_n;
    if (total > 0) {
        if (g_in.pressed[BTN_UP]) {
            stm.cursor = (stm.cursor + total - 1) % total;
            audio_sfx(SFX_BLIP);
        }
        if (g_in.pressed[BTN_DOWN]) {
            stm.cursor = (stm.cursor + 1) % total;
            audio_sfx(SFX_BLIP);
        }
    }
    if (g_in.pressed[BTN_B]) {
        stm.active = false;
        return;
    }
    if (g_in.pressed[BTN_A]) {
        if (stm.cursor < g.party_n) {
            char name[24];
            snprintf(name, sizeof(name), "%s",
                     SPECIES[g.party[stm.cursor].species].name);
            int rc = storage_deposit(stm.cursor);
            if (rc == 0) {
                audio_sfx(SFX_CONFIRM);
                snprintf(stm.msg, sizeof(stm.msg), "%s was stored!", name);
            } else if (rc == -1) {
                snprintf(stm.msg, sizeof(stm.msg), "Keep your last partner!");
            } else if (rc == -3) {
                snprintf(stm.msg, sizeof(stm.msg), "Keep a healthy partner!");
            } else {
                snprintf(stm.msg, sizeof(stm.msg), "The box is full!");
            }
        } else {
            int idx = stm.cursor - g.party_n;
            char name[24];
            snprintf(name, sizeof(name), "%s",
                     SPECIES[g.storage[idx].species].name);
            int rc = storage_withdraw(idx);
            if (rc == 0) {
                audio_sfx(SFX_CONFIRM);
                snprintf(stm.msg, sizeof(stm.msg), "%s joined the party!", name);
                stm.cursor = g.party_n - 1;
            } else {
                snprintf(stm.msg, sizeof(stm.msg), "Your party is full!");
            }
        }
        stm.msg_frames = 90;
        int total2 = g.party_n + g.storage_n;
        if (stm.cursor >= total2)
            stm.cursor = total2 > 0 ? total2 - 1 : 0;
    }
}

void storage_draw(SDL_Renderer *r)
{
    if (!stm.active)
        return;
    draw_panel(r, 2, 2, SCREEN_W - 4, SCREEN_H - 4);
    SDL_Color dark = { 56, 56, 64, 255 };
    SDL_Color red = { 200, 48, 48, 255 };
    SDL_Color gray = { 120, 120, 132, 255 };
    SDL_Color head = { 56, 96, 160, 255 };
    char buf[32];

    draw_text(r, 10, 6, "STORAGE PC", dark, 1);
    draw_text(r, 10, 18, "PARTY", head, 1);
    draw_text(r, 124, 18, "BOX", head, 1);

    for (int i = 0; i < g.party_n; i++) {
        int y = 30 + i * 12;
        if (i == stm.cursor)
            draw_text(r, 4, y, ">", red, 1);
        snprintf(buf, sizeof(buf), "%s", SPECIES[g.party[i].species].name);
        draw_text(r, 12, y, buf, dark, 1);
        snprintf(buf, sizeof(buf), "L%u", g.party[i].level);
        draw_text(r, 96, y, buf, gray, 1);
    }
    if (g.party_n == 0)
        draw_text(r, 12, 30, "(empty)", gray, 1);

    for (int j = 0; j < g.storage_n; j++) {
        int y = 30 + j * 10;
        int sel = g.party_n + j;
        if (sel == stm.cursor)
            draw_text(r, 118, y, ">", red, 1);
        snprintf(buf, sizeof(buf), "%s", SPECIES[g.storage[j].species].name);
        draw_text(r, 126, y, buf, dark, 1);
        snprintf(buf, sizeof(buf), "L%u", g.storage[j].level);
        draw_text(r, 210, y, buf, gray, 1);
    }
    if (g.storage_n == 0)
        draw_text(r, 126, 30, "(empty)", gray, 1);

    if (stm.msg[0])
        draw_text(r, 10, SCREEN_H - 12, stm.msg, dark, 1);
    else
        draw_text(r, 10, SCREEN_H - 12, "Z:STORE/TAKE  X:EXIT", dark, 1);
}
