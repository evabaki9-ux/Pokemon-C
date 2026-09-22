/* Pure game logic: stats, xp, damage, catching, enemy AI. No SDL - unit-testable. */
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "game.h"

uint16_t xp_for_level(uint8_t level)
{
    /* medium-fast: n^3 */
    if (level > 100) level = 100;
    return (uint16_t)((uint32_t)level * level * level);
}

static uint16_t stat_calc(const Species *sp, uint8_t iv, uint8_t level, uint8_t which)
{
    uint32_t b = sp->base[which];
    if (which == ST_HP)
        return (uint16_t)(((2 * b + iv) * level) / 100 + level + 10);
    return (uint16_t)(((2 * b + iv) * level) / 100 + 5);
}

void creature_recalc_stats(Creature *c, int keep_ratio)
{
    const Species *sp = &SPECIES[c->species];
    uint16_t old_max = c->stats[ST_HP];
    uint16_t new_stats[6];
    for (int i = 0; i < 6; i++)
        new_stats[i] = stat_calc(sp, c->ivs[i], c->level, (uint8_t)i);
    uint16_t new_max = new_stats[ST_HP];
    memcpy(c->stats, new_stats, sizeof(new_stats));
    if (keep_ratio == 1) { /* keep hp fraction */
        c->hp = (uint16_t)((uint32_t)c->hp * new_max / (old_max ? old_max : 1));
        if (c->hp == 0) c->hp = 1;
    } else { /* keep hp delta (level-up style) */
        c->hp = (uint16_t)(c->hp + (new_max > old_max ? new_max - old_max : 0));
        if (c->hp > new_max) c->hp = new_max;
        if (c->hp == 0) c->hp = 1;
    }
}

void creature_init(Creature *c, uint8_t species, uint8_t level, uint8_t random_ivs)
{
    memset(c, 0, sizeof(*c));
    c->species = species;
    c->level = level;
    c->xp = xp_for_level(level);
    for (int i = 0; i < 6; i++)
        c->ivs[i] = random_ivs ? (uint8_t)(rand() % 32) : 16;
    for (int i = 0; i < 4; i++) {
        c->moves[i] = 0xFF;
        c->pp[i] = 0;
    }
    /* populate moves from learnset (last 4 usable at this level) */
    const Species *sp = &SPECIES[species];
    uint8_t chosen[4];
    int n = 0;
    for (int i = 0; i < LEARN_MAX && sp->learn[i] != LEARN_END; i++) {
        uint8_t lvl = (uint8_t)(sp->learn[i] >> 8);
        uint8_t mv = (uint8_t)(sp->learn[i] & 0xFF);
        if (lvl <= level) {
            if (n < 4) {
                chosen[n++] = mv;
            } else {
                chosen[0] = chosen[1]; chosen[1] = chosen[2];
                chosen[2] = chosen[3]; chosen[3] = mv;
            }
        }
    }
    for (int i = 0; i < n; i++) {
        c->moves[i] = chosen[i];
        c->pp[i] = MOVES[chosen[i]].pp;
    }
    c->ailment = AIL_NONE;
    c->sleep_turns = 0;
    /* stats after moves so hp is full */
    for (int i = 0; i < 6; i++)
        c->stats[i] = stat_calc(sp, c->ivs[i], level, (uint8_t)i);
    c->hp = c->stats[ST_HP];
}

/* returns moves learned at new levels via learn check by caller */
void creature_add_xp(Creature *c, uint16_t amount, int *levels_gained)
{
    int gained = 0;
    uint32_t xp = c->xp + amount;
    while (c->level < 100 && xp >= xp_for_level((uint8_t)(c->level + 1))) {
        c->level++;
        gained++;
    }
    c->xp = (uint16_t)(xp > 0xFFFF ? 0xFFFF : xp);
    if (gained > 0)
        creature_recalc_stats(c, 0);
    if (levels_gained)
        *levels_gained = gained;
}


uint16_t stat_after_stage(uint16_t v, int8_t stage)
{
    if (stage > 6) stage = 6;
    if (stage < -6) stage = -6;
    int32_t num = 2, den = 2;
    if (stage >= 0) num = 2 + stage;
    else den = 2 - stage;
    return (uint16_t)((uint32_t)v * num / den);
}

DamageResult move_damage(const Creature *att, const Creature *def,
                         const int8_t att_st[6], const int8_t def_st[6],
                         uint8_t move_id)
{
    DamageResult r = { 0, 0, 0, 0 };
    const Move *mv = &MOVES[move_id];
    const Species *a = &SPECIES[att->species];
    const Species *d = &SPECIES[def->species];

    /* accuracy */
    if (mv->accuracy > 0 && (rand() % 100) >= mv->accuracy) {
        r.missed = 1;
        return r;
    }

    if (mv->category == MC_STATUS) {
        r.damage = 0;
        return r; /* statuses applied by battle layer */
    }

    /* effectiveness: product of per-type multipliers, 10 = 1.0x
     * (chart values are x10-scaled, so divide by 10 per lookup) */
    int eff10 = 10;
    eff10 = eff10 * type_chart_lookup(mv->type, d->type1) / 10;
    if (d->type2 != TY_NONE)
        eff10 = eff10 * type_chart_lookup(mv->type, d->type2) / 10;
    if (eff10 == 0) {
        r.effectiveness = -2;
        r.damage = 0;
        return r;
    }

    uint8_t atk_stat = (mv->category == MC_PHYS) ? ST_ATK : ST_SAT;
    uint8_t def_stat = (mv->category == MC_PHYS) ? ST_DEF : ST_SDF;
    uint32_t A = att->stats[atk_stat];
    uint32_t D = def->stats[def_stat];
    if (att_st) A = stat_after_stage((uint16_t)A, att_st[atk_stat]);
    if (def_st) D = stat_after_stage((uint16_t)D, def_st[def_stat]);
    if (att->ailment == AIL_BURN && mv->category == MC_PHYS) A /= 2;

    uint32_t L = att->level;
    uint32_t dmg = ((2 * L / 5 + 2) * mv->power * A / (D ? D : 1)) / 50 + 2;

    /* STAB */
    if (mv->type == a->type1 || mv->type == a->type2) dmg = dmg * 15 / 10;
    /* type effectiveness */
    dmg = dmg * (uint32_t)eff10 / 10;
    /* random 85-100% */
    dmg = dmg * (85 + rand() % 16) / 100;
    /* crit */
    int crit_chance = (mv->effect == ME_HIGH_CRIT) ? 8 : 1;
    if (rand() % 16 < crit_chance) {
        r.crit = 1;
        dmg *= 2;
    }
    if (dmg < 1) dmg = 1;
    if (dmg > def->hp) dmg = def->hp;
    r.damage = (int)dmg;
    if (eff10 > 10) r.effectiveness = eff10 >= 20 ? 2 : 1;
    else if (eff10 < 10) r.effectiveness = -1;
    return r;
}

int catch_shakes(const Creature *wild, int ball_x10)
{
    const Species *sp = &SPECIES[wild->species];
    uint32_t maxhp = wild->stats[ST_HP];
    uint32_t a = ((3 * maxhp - 2 * wild->hp) * sp->catch_rate * (uint32_t)ball_x10)
                 / (3 * maxhp) / 10;
    if (wild->ailment == AIL_SLEEP) a *= 2;
    else if (wild->ailment != AIL_NONE) a = a * 3 / 2;
    if (a > 255) a = 255;
    if (a < 1) a = 1;
    /* gen-3 style shake threshold */
    double b = 1048560.0 / pow(16711680.0 / (double)a, 0.25);
    double p = b / 65536.0;
    if (p > 1.0) p = 1.0;
    int shakes = 0;
    for (int i = 0; i < 4; i++) {
        if ((double)rand() / ((double)RAND_MAX + 1.0) < p)
            shakes++;
        else
            break;
    }
    return shakes; /* 4 == caught */
}

bool learn_move(Creature *c, uint8_t move_id, int *replaced)
{
    for (int i = 0; i < 4; i++) {
        if (c->moves[i] == move_id) {
            if (replaced) *replaced = -1;
            return false;
        }
    }
    int slot = -1;
    for (int i = 0; i < 4; i++)
        if (c->moves[i] == 0xFF) { slot = i; break; }
    if (slot < 0) { /* replace oldest (index 0): shift left, new move at 3 */
        if (replaced) *replaced = c->moves[0];
        for (int i = 0; i < 3; i++) {
            c->moves[i] = c->moves[i + 1];
            c->pp[i] = c->pp[i + 1];
        }
        slot = 3;
    } else if (replaced) {
        *replaced = -1;
    }
    c->moves[slot] = move_id;
    c->pp[slot] = MOVES[move_id].pp;
    return true;
}

/* rough expected damage for AI ranking */
static int expected_damage(const Creature *att, const Creature *def, uint8_t move_id)
{
    const Move *mv = &MOVES[move_id];
    if (mv->category == MC_STATUS || mv->power == 0) return 0;
    const Species *d = &SPECIES[def->species];
    int eff10 = type_chart_lookup(mv->type, d->type1);
    if (d->type2 != TY_NONE) eff10 *= type_chart_lookup(mv->type, d->type2);
    uint8_t as = (mv->category == MC_PHYS) ? ST_ATK : ST_SAT;
    uint8_t ds = (mv->category == MC_PHYS) ? ST_DEF : ST_SDF;
    int dmg = ((2 * att->level / 5 + 2) * mv->power * att->stats[as] / (def->stats[ds] ? def->stats[ds] : 1)) / 50;
    if (mv->type == SPECIES[att->species].type1 || mv->type == SPECIES[att->species].type2) dmg = dmg * 15 / 10;
    dmg = dmg * eff10 / 10;
    return dmg;
}

static bool slot_pickable(const Creature *party, int n, int exclude_slot,
                          bool require_hp, int slot)
{
    if (slot < 0 || slot >= n)
        return false;
    if (slot == exclude_slot)
        return false;
    if (require_hp && party[slot].hp == 0)
        return false;
    return true;
}

int party_pick_cursor(const Creature *party, int n, int exclude_slot,
                      bool require_hp, int from, int dir)
{
    if (n <= 0)
        return -1;
    for (int i = 1; i <= n; i++) {
        int slot = (((from + dir * i) % n) + n) % n;
        if (slot_pickable(party, n, exclude_slot, require_hp, slot))
            return slot;
    }
    return -1;
}

bool party_any_pickable(const Creature *party, int n, int exclude_slot,
                        bool require_hp)
{
    for (int i = 0; i < n; i++)
        if (slot_pickable(party, n, exclude_slot, require_hp, i))
            return true;
    return false;
}

int pick_enemy_move(const Creature *enemy, const Creature *player)
{
    int usable[4], n = 0;
    for (int i = 0; i < 4; i++)
        if (enemy->moves[i] != 0xFF && enemy->pp[i] > 0) usable[n++] = i;
    if (n == 0) return -1; /* struggle not implemented: fallback to first move */
    /* 65%: best expected damage; else random */
    if (rand() % 100 < 65) {
        int best = usable[0], best_d = -1;
        for (int i = 0; i < n; i++) {
            int d = expected_damage(enemy, player, enemy->moves[usable[i]]);
            if (d > best_d) { best_d = d; best = usable[i]; }
        }
        return best;
    }
    return usable[rand() % n];
}
