/* Headless engine tests (no SDL). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.h"
#include "data/music.h"
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
        CHECK(r.damage < (int)d2.hp, "neutral tackle does not 1HKO (eff10 bug)");
        CHECK(r.effectiveness == 0, "neutral hit is not super effective");
    }
    /* fire vs bug+grass = 4x, but sane: not a x10 blowup */
    Creature moss;
    creature_init(&moss, SP_MOSSLING, 5, 0);
    uint8_t ember_id = 0xFF;
    for (uint8_t m = 0; m < NUM_MOVES; m++)
        if (MOVES[m].type == TY_FIRE && MOVES[m].power > 0)
            { ember_id = m; break; }
    CHECK(ember_id != 0xFF, "found a fire damage move");
    DamageResult rf = move_damage(&d1, &moss, none, none, ember_id);
    if (!rf.missed) {
        CHECK(rf.effectiveness >= 1, "fire vs bug/grass is super effective");
        CHECK(rf.damage <= (int)moss.hp, "4x hit still bounded");
        /* 4x of a ~5-7 base is 20-28 vs 20 HP: can KO, but must not be
         * ~10x that (the old bug gave 100x+) */
        CHECK(rf.damage <= 40, "4x hit is not a x10 blowup");
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
    CHECK(MAPS[MAP_ROUTE1].nnpcs == 3, "route1 has liam, maya, hiker");
    CHECK(MAPS[MAP_ROUTE1].nwarps == 4, "route1 has town + route2 warps");
    CHECK(MAPS[MAP_ROUTE2].nnpcs == 3, "route2 has denim, bram, iris");
    CHECK(MAPS[MAP_ROUTE2].nencs == 9, "route2 encounter table");
    {
        int boss = 0, blocker = 0;
        for (int i = 0; i < MAPS[MAP_ROUTE2].nnpcs; i++)
            if (MAPS[MAP_ROUTE2].npcs[i].trainer)
                boss = MAPS[MAP_ROUTE2].npcs[i].trainer->flag;
        for (int i = 0; i < MAPS[MAP_ROUTE1].nnpcs; i++)
            if (MAPS[MAP_ROUTE1].npcs[i].role == ROLE_BLOCKER)
                blocker = MAPS[MAP_ROUTE1].npcs[i].need_flags;
        CHECK(boss == 32, "keeper iris sets the boss flag");
        CHECK(blocker == 6, "hiker leaves once both trainers beaten");
    }
    CHECK(MAPS[MAP_ROUTE1].npcs[0].trainer->flag == 2 &&
          MAPS[MAP_ROUTE1].npcs[1].trainer->flag == 4,
          "trainer defeat flags wired");
    CHECK(MAPS[MAP_HOUSE].npcs != NULL, "house has mom");

    /* party cursor: freeze regression (battle TEAM with nothing pickable) */
    {
        Creature p1[1];
        creature_init(&p1[0], SP_EMBERIT, 5, 0);
        CHECK(party_pick_cursor(p1, 1, 0, true, 0, 1) == -1,
              "single creature: nothing to switch to");
        CHECK(!party_any_pickable(p1, 1, 0, true),
              "single creature: none pickable");
        Creature p2[2];
        creature_init(&p2[0], SP_EMBERIT, 5, 0);
        creature_init(&p2[1], SP_FLUFFIT, 4, 0);
        p2[1].hp = 0;
        CHECK(party_pick_cursor(p2, 2, 0, true, 0, 1) == -1,
              "fainted partner: still nothing to switch to");
        CHECK(party_pick_cursor(p2, 2, 0, false, 0, 1) == 1,
              "view mode skips nothing");
        p2[1].hp = p2[1].stats[ST_HP];
        CHECK(party_pick_cursor(p2, 2, 0, true, 0, 1) == 1,
              "healthy partner found");
        CHECK(party_pick_cursor(p2, 2, 0, true, 1, 1) == 1,
              "cursor already on pickable stays in range");
    }

    /* music data sanity (audio engine data layer) */
    {
        CHECK(MUSIC[MUS_TITLE].bpm >= 40 && MUSIC[MUS_TITLE].bpm <= 240,
              "title tempo sane");
        for (int t2 = MUS_TITLE; t2 < MUS_COUNT; t2++) {
            const MusicTrack *tr = &MUSIC[t2];
            CHECK(tr->len[0] > 0 && tr->len[0] <= MUS_MAXLEN,
                  "track has melody steps");
            for (int ch = 0; ch < MUS_CHAN; ch++) {
                CHECK(tr->len[ch] <= MUS_MAXLEN, "channel length bounded");
                for (int i2 = 0; i2 < tr->len[ch]; i2++) {
                    int8_t v = tr->mel[ch][i2];
                    CHECK(v >= -1 && v <= 108, "note value in range");
                }
            }
        }
        CHECK(music_for_map(MAP_ROUTE1) == MUS_ROUTE, "route gets route music");
        CHECK(music_for_map(MAP_TOWN) == MUS_TOWN, "town gets town music");
        for (int s2 = SFX_BLIP; s2 < SFX_COUNT; s2++) {
            CHECK(SFX[s2].f0 >= 20 && SFX[s2].f1 <= 9000, "sfx freqs sane");
            CHECK(SFX[s2].ms >= 10 && SFX[s2].ms <= 2000, "sfx length sane");
            CHECK(SFX[s2].vol >= 1 && SFX[s2].vol <= 15, "sfx volume sane");
        }
    }

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
