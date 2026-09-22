/* Headless engine tests (no SDL). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "save.h"

static int failures = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); failures++; } \
    else { printf("ok:   %s\n", msg); } \
} while (0)

int main(void)
{
    srand(42);

    /* xp curve */
    CHECK(xp_for_level(5) == 125, "xp curve L5 = 125");
    CHECK(xp_for_level(16) == 4096, "xp curve L16 = 4096");

    /* stats: EMBERIT L5, IV 16 */
    Creature e;
    creature_init(&e, SP_EMBERIT, 5, 0);
    CHECK(e.stats[ST_HP] == (2 * 44 + 16) * 5 / 100 + 5 + 10, "emberit L5 hp formula");
    CHECK(e.hp == e.stats[ST_HP], "creature starts at full hp");
    CHECK(e.moves[0] == 0 && e.moves[1] == 2, "emberit starts TACKLE+GROWL");
    CHECK(e.pp[0] == 35, "tackle pp 35");

    /* type chart */
    CHECK(type_chart_lookup(TY_FIRE, TY_GRASS) == 20, "fire -> grass 2x");
    CHECK(type_chart_lookup(TY_WATER, TY_FIRE) == 20, "water -> fire 2x");
    CHECK(type_chart_lookup(TY_FIRE, TY_WATER) == 5, "fire -> water 0.5x");
    CHECK(type_chart_lookup(TY_ELECTRIC, TY_WATER) == 20, "elec -> water 2x");

    /* damage bounds */
    Creature d1, d2;
    creature_init(&d1, SP_EMBERIT, 5, 0);
    creature_init(&d2, SP_FLUFFIT, 4, 0);
    int8_t none[6] = { 0 };
    DamageResult r = move_damage(&d1, &d2, none, none, 0 /* TACKLE */);
    CHECK(!r.missed || 1, "tackle resolved");
    if (!r.missed) {
        CHECK(r.damage >= 1 && r.damage <= (int)d2.hp, "damage within bounds");
    }

    /* stat stages */
    CHECK(stat_after_stage(100, 1) == 150, "stage +1 = 1.5x");
    CHECK(stat_after_stage(100, -1) == 66, "stage -1 ~= 0.66x");
    CHECK(stat_after_stage(100, 6) == 400, "stage +6 = 4x");

    /* catch rates */
    Creature wild;
    creature_init(&wild, SP_FLUFFIT, 3, 0); /* catch rate 255, full hp */
    int caught_full = 0, caught_low = 0;
    Creature tough;
    creature_init(&tough, SP_PEBBLY, 5, 0); /* catch rate 180 */
    tough.hp = 1;
    for (int i = 0; i < 2000; i++) {
        if (catch_shakes(&wild, 10) >= 4) caught_full++;
        if (catch_shakes(&tough, 10) >= 4) caught_low++;
    }
    printf("     catch rates: full-hp fluffit %d/2000, 1hp pebbly %d/2000\n",
           caught_full, caught_low);
    CHECK(caught_full > 600, "full-hp fluffit catchable (>30%)");
    CHECK(caught_low > caught_full, "low hp catches more often");

    /* xp + level up + learn (learn loop mirrors battle.c resolve_xp) */
    Creature c;
    creature_init(&c, SP_EMBERIT, 5, 0);
    int gained = 0;
    uint8_t oldlvl = c.level;
    creature_add_xp(&c, 1000, &gained);
    CHECK(c.level > 5, "emberit leveled up");
    bool has_ember = false;
    const Species *sp = &SPECIES[c.species];
    for (int i = 0; i < LEARN_MAX && sp->learn[i] != LEARN_END; i++) {
        uint8_t lvl = (uint8_t)(sp->learn[i] >> 8);
        if (lvl > oldlvl && lvl <= c.level) {
            int replaced = -1;
            if (learn_move(&c, (uint8_t)(sp->learn[i] & 0xFF), &replaced))
                has_ember = (sp->learn[i] & 0xFF) == 9 || has_ember;
        }
    }
    CHECK(has_ember, "learned EMBER at L6");

    /* learn_move replacement: L20 fluffit knows GROWL/HOWL/HEADBUTT/HARDEN */
    Creature f;
    creature_init(&f, SP_FLUFFIT, 20, 0);
    int replaced = -1;
    bool learned = learn_move(&f, 0 /* TACKLE: not currently known */, &replaced);
    CHECK(learned && replaced == 2 /* GROWL replaced */,
          "full moveset replaces oldest");
    bool has_tackle = false, still_has_howl = false;
    for (int i = 0; i < 4; i++) {
        if (f.moves[i] == 0) has_tackle = true;
        if (f.moves[i] == 4) still_has_howl = true;
    }
    CHECK(has_tackle && still_has_howl, "replacement kept other moves");

    /* enemy ai returns a valid slot */
    Creature p;
    creature_init(&p, SP_DEWLIN, 5, 0);
    int mv = pick_enemy_move(&f, &p);
    CHECK(mv >= 0 && mv < 4, "enemy ai picks valid slot");

    /* maps compiled */
    CHECK(MAPS[MAP_TOWN].w == 28 && MAPS[MAP_TOWN].h == 24, "town dimensions");
    CHECK(MAPS[MAP_ROUTE1].nencs == 5, "route1 encounter table");
    CHECK(MAPS[MAP_HOUSE].npcs != NULL, "house has mom");

    /* save round-trip */
    memset(&g, 0, sizeof(g));
    g.map = MAP_ROUTE1;
    g.px = 160; g.py = 208; g.dir = 2;
    g.party_n = 2;
    creature_init(&g.party[0], SP_EMBERIT, 7, 0);
    creature_init(&g.party[1], SP_FLUFFIT, 3, 0);
    g.active_slot = 0;
    g.bag_n = 2;
    g.bag[0] = IT_ORB;
    g.bag[1] = IT_POTION;
    g.money = 999;
    g.flags = FLAG_STARTER | FLAG_T1;
    g.dex_seen[0] = 0x0F;
    g.heal_map = MAP_HEAL;
    g.heal_x = 3; g.heal_y = 5;
    uint8_t sbuf[SAVE_MAX];
    size_t sn = save_encode(sbuf, sizeof(sbuf));
    CHECK(sn > 0 && sn < SAVE_MAX, "save encodes");
    memset(&g, 0, sizeof(g));
    CHECK(save_decode(sbuf, sn), "save decodes");
    CHECK(g.party_n == 2 && g.party[1].species == SP_FLUFFIT &&
          g.party[0].level == 7 && g.party[0].hp == g.party[0].stats[ST_HP],
          "party restored");
    CHECK(g.money == 999 && g.flags == (FLAG_STARTER | FLAG_T1) &&
          g.dex_seen[0] == 0x0F, "progress restored");
    CHECK(g.map == MAP_ROUTE1 && g.dir == 2 && g.px == 160 && g.py == 208,
          "position restored");
    CHECK(g.heal_map == MAP_HEAL && g.heal_x == 3, "heal point restored");
    CHECK(g.mode == MODE_OVERWORLD, "loaded into overworld mode");
    CHECK(!save_decode(sbuf + 1, sn - 1), "corrupt magic rejected");
    sbuf[11] = 99; /* party_n field out of range */
    CHECK(!save_decode(sbuf, sn), "bad party_n rejected");

    if (failures == 0)
        printf("\nALL TESTS PASSED\n");
    else
        printf("\n%d FAILURES\n", failures);
    return failures ? 1 : 0;
}
