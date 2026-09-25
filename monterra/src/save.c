#include <stdio.h>
#include <string.h>
#include "save.h"
#include "game.h"

#define SAVE_MAGIC "MNTS"
#define SAVE_VERSION 3

/* ---- little writer/reader ---- */
typedef struct {
    uint8_t *p;
    size_t cap, n;
    bool err;
} Wr;

static void w8(Wr *w, uint8_t v)
{
    if (w->n + 1 > w->cap) { w->err = true; return; }
    w->p[w->n++] = v;
}

static void w16(Wr *w, uint16_t v)
{
    w8(w, (uint8_t)(v & 0xFF));
    w8(w, (uint8_t)(v >> 8));
}

static void wbytes(Wr *w, const void *src, size_t n)
{
    const uint8_t *s = src;
    for (size_t i = 0; i < n; i++)
        w8(w, s[i]);
}

typedef struct {
    const uint8_t *p;
    size_t cap, n;
    bool err;
} Rd;

static uint8_t r8(Rd *r)
{
    if (r->n + 1 > r->cap) { r->err = true; return 0; }
    return r->p[r->n++];
}

static uint16_t r16(Rd *r)
{
    uint16_t lo = r8(r), hi = r8(r);
    return (uint16_t)(lo | (hi << 8));
}

static void rbytes(Rd *r, void *dst, size_t n)
{
    uint8_t *d = dst;
    for (size_t i = 0; i < n; i++)
        d[i] = r8(r);
}

/* ---- serialize ---- */
static void save_creature(Wr *w, const Creature *c)
{
    w8(w, c->species);
    w8(w, c->level);
    w16(w, c->xp);
    w16(w, c->hp);
    for (int i = 0; i < 6; i++)
        w16(w, c->stats[i]);
    for (int i = 0; i < 6; i++)
        w8(w, c->ivs[i]);
    for (int i = 0; i < 4; i++)
        w8(w, c->moves[i]);
    for (int i = 0; i < 4; i++)
        w8(w, c->pp[i]);
    w8(w, c->ailment);
    w8(w, c->sleep_turns);
}

static bool load_creature(Rd *r, Creature *c)
{
    memset(c, 0, sizeof(*c));
    c->species = r8(r);
    c->level = r8(r);
    c->xp = r16(r);
    c->hp = r16(r);
    for (int i = 0; i < 6; i++)
        c->stats[i] = r16(r);
    for (int i = 0; i < 6; i++)
        c->ivs[i] = r8(r);
    for (int i = 0; i < 4; i++)
        c->moves[i] = r8(r);
    for (int i = 0; i < 4; i++)
        c->pp[i] = r8(r);
    c->ailment = r8(r);
    c->sleep_turns = r8(r);
    if (r->err)
        return false;
    /* sanity: species/level/moves in range, hp <= maxhp */
    if (c->species >= NUM_SPECIES_TOTAL || c->level < 1 || c->level > 100)
        return false;
    if (c->hp > c->stats[ST_HP])
        return false;
    for (int i = 0; i < 4; i++)
        if (c->moves[i] != 0xFF && c->moves[i] >= NUM_MOVES)
            return false;
    return true;
}

size_t save_encode(uint8_t *buf, size_t cap)
{
    Wr w = { buf, cap, 0, false };
    wbytes(&w, SAVE_MAGIC, 4);
    w8(&w, SAVE_VERSION);
    w8(&w, g.map);
    w16(&w, g.px);
    w16(&w, g.py);
    w8(&w, g.dir);
    w8(&w, g.party_n);
    w8(&w, g.active_slot);
    for (int i = 0; i < g.party_n; i++)
        save_creature(&w, &g.party[i]);
    w8(&w, g.bag_n);
    for (int i = 0; i < g.bag_n; i++)
        w8(&w, g.bag[i]);
    w16(&w, g.money);
    w8(&w, g.flags);
    wbytes(&w, g.dex_seen, sizeof(g.dex_seen));
    wbytes(&w, g.dex_caught, sizeof(g.dex_caught));
    w8(&w, g.heal_map);
    w8(&w, g.heal_x);
    w8(&w, g.heal_y);
    w8(&w, g.storage_n);
    for (int i = 0; i < g.storage_n; i++)
        save_creature(&w, &g.storage[i]);
    if (w.err)
        return 0;
    return w.n;
}

bool save_decode(const uint8_t *buf, size_t len)
{
    Rd r = { buf, len, 0, false };
    char magic[4];
    rbytes(&r, magic, 4);
    if (memcmp(magic, SAVE_MAGIC, 4) != 0)
        return false;
    uint8_t ver = r8(&r);
    if (ver < 2 || ver > SAVE_VERSION)
        return false;

    GameState t;
    memset(&t, 0, sizeof(t));
    t.map = r8(&r);
    t.px = r16(&r);
    t.py = r16(&r);
    t.dir = r8(&r);
    t.party_n = r8(&r);
    t.active_slot = r8(&r);
    if (r.err || t.map >= NUM_MAPS || t.dir > 3 ||
        t.party_n > MAX_PARTY || t.active_slot >= MAX_PARTY ||
        (t.party_n > 0 && t.active_slot >= t.party_n))
        return false;
    for (int i = 0; i < t.party_n; i++)
        if (!load_creature(&r, &t.party[i]))
            return false;
    t.bag_n = r8(&r);
    if (t.bag_n > MAX_BAG)
        return false;
    for (int i = 0; i < t.bag_n; i++) {
        t.bag[i] = r8(&r);
        if (t.bag[i] >= NUM_ITEM_IDS)
            return false;
    }
    t.money = r16(&r);
    t.flags = r8(&r);
    rbytes(&r, t.dex_seen, sizeof(t.dex_seen));
    rbytes(&r, t.dex_caught, sizeof(t.dex_caught));
    t.heal_map = r8(&r);
    t.heal_x = r8(&r);
    t.heal_y = r8(&r);
    if (r.err || t.heal_map >= NUM_MAPS)
        return false;
    if (ver >= 3) {
        t.storage_n = r8(&r);
        if (r.err || t.storage_n > STORAGE_MAX)
            return false;
        for (int i = 0; i < t.storage_n; i++)
            if (!load_creature(&r, &t.storage[i]))
                return false;
    }

    /* position must be on the map */
    const MapDef *m = &MAPS[t.map];
    if (t.px / 16 >= m->w || t.py / 16 >= m->h)
        return false;

    /* commit */
    t.mode = MODE_OVERWORLD;
    t.moving = 0;
    t.dx_steps = 0;
    t.hop = 0;
    t.tick = g.tick;
    t.fade = 0;
    t.fade_dir = 0;
    t.fade_cb = NULL;
    g = t;
    g_battle_req.active = 0;
    return true;
}

/* ---- platform storage ---- */
#ifdef __EMSCRIPTEN__
#include <emscripten.h>

static uint8_t s_buf[SAVE_MAX];

bool save_write(void)
{
    size_t n = save_encode(s_buf, sizeof(s_buf));
    if (n == 0)
        return false;
    EM_ASM_({
        var bytes = new Uint8Array(HEAPU8.buffer, $0, $1);
        var s = "";
        for (var i = 0; i < $1; i++) s += String.fromCharCode(bytes[i]);
        try { localStorage.setItem("monterra_save", btoa(s)); } catch (e) {}
    }, s_buf, (int)n);
    return true;
}

bool save_read(void)
{
    int len = EM_ASM_INT({
        var s = null;
        try { s = localStorage.getItem("monterra_save"); } catch (e) {}
        if (!s) return 0;
        try {
            var bin = atob(s);
            var n = bin.length;
            if (n > $0) return -1;
            for (var i = 0; i < n; i++)
                HEAPU8[$1 + i] = bin.charCodeAt(i);
            return n;
        } catch (e) { return 0; }
    }, (int)sizeof(s_buf), s_buf);
    if (len <= 0)
        return false;
    return save_decode(s_buf, (size_t)len);
}

bool save_exists(void)
{
    int has = EM_ASM_INT({
        try { return localStorage.getItem("monterra_save") ? 1 : 0; }
        catch (e) { return 0; }
    });
    return has != 0;
}

#else /* native: file */

static const char *SAVE_PATH(void)
{
    return "monterra.sav";
}

bool save_write(void)
{
    uint8_t buf[SAVE_MAX];
    size_t n = save_encode(buf, sizeof(buf));
    if (n == 0)
        return false;
    FILE *f = fopen(SAVE_PATH(), "wb");
    if (!f)
        return false;
    bool ok = fwrite(buf, 1, n, f) == n;
    fclose(f);
    return ok;
}

bool save_read(void)
{
    uint8_t buf[SAVE_MAX];
    FILE *f = fopen(SAVE_PATH(), "rb");
    if (!f)
        return false;
    size_t n = fread(buf, 1, sizeof(buf), f);
    fclose(f);
    return n > 0 && save_decode(buf, n);
}

bool save_exists(void)
{
    FILE *f = fopen(SAVE_PATH(), "rb");
    if (!f)
        return false;
    fclose(f);
    return true;
}

#endif
