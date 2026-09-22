#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "battle.h"
#include "game.h"
#include "assets.h"
#include "text.h"
#include "ui.h"

/* ------------------------------------------------------------------
 * Battle engine. A step queue drives everything; each step either
 * executes instantly (possibly pushing more steps to run next, in
 * order) or blocks on an animation/dialog until it completes.
 * ------------------------------------------------------------------ */

enum {
    BS_MSG,       /* show text lines, wait for A */
    BS_DELAY,     /* a = frames */
    BS_WAIT_HP,   /* block until displayed HP reaches actual HP */
    BS_FAINT,     /* a=target 0 player 1 enemy: drop sprite */
    BS_SHAKE,     /* a = frames of orb-shake wobble */
    BS_ACT_MOVE,  /* a=actor, b=move slot */
    BS_END_TURN,  /* end-of-turn ailments (burn) */
    BS_CHECK,     /* faint / victory / next trainer creature */
    BS_MENU,      /* return to player input menu */
    BS_THROW,     /* a=item id: catch sequence */
    BS_XP,        /* award xp (with level/learn/evolve messages) */
    BS_SWITCH_IN, /* a=party slot: send creature out */
    BS_ENEMY_NEXT,/* trainer sends next creature */
    BS_END,       /* a=result code: battle finishes */
};

typedef struct {
    uint8_t kind, a, b;
    const char *l1, *l2;
} Step;

#define QMAX 128
#define MSGPOOL 24
static Step q[QMAX];
static int qlen, qhead, qins;
static char msgpool[MSGPOOL][2][76];
static int pool_next;

static void q_clear(void)
{
    qlen = qhead = qins = 0;
    pool_next = 0;
}

static void q_pop(void)
{
    qhead++;
    qins = qhead;
}

/* queue a step to run after everything already ordered (call order = run order) */
static void qf(uint8_t kind, uint8_t a, uint8_t b, const char *l1, const char *l2)
{
    if (qlen >= QMAX)
        return;
    memmove(&q[qins + 1], &q[qins], (size_t)(qlen - qins) * sizeof(Step));
    q[qins].kind = kind;
    q[qins].a = a;
    q[qins].b = b;
    q[qins].l1 = l1;
    q[qins].l2 = l2;
    qlen++;
    qins++;
}

/* ---- battle state ---- */
enum { BP_MENU, BP_FIGHT, BP_BAG, BP_PARTY, BP_EXEC, BP_DONE };

static struct {
    bool active, over, trainer;
    const TrainerDef *tdef;
    Creature enemy;
    int trainer_next;
    int8_t pstages[6], estages[6];
    int phase;
    int menu_cur, fight_cur;
    int16_t php_show, ehp_show;
    int faint_off[2]; /* >= 200 = hidden */
    int shake_timer;
    int run_attempts;
    uint8_t result;
    bool forced_switch;
    bool switch_done; /* set when a voluntary switch was performed */
} B;

static Creature *pc(void) { return &g.party[g.active_slot]; }
static const char *pname(void) { return SPECIES[pc()->species].name; }
static const char *ename(void) { return SPECIES[B.enemy.species].name; }

bool battle_over(void) { return B.over; }
uint8_t battle_result(void) { return B.result; }

uint8_t battle_trainer_flag(void)
{
    return (B.trainer && B.tdef) ? B.tdef->flag : 0;
}

/* ---- message helpers ---- */
static const char *pool_put(const char *s)
{
    static char fallback[76];
    if (!s)
        return NULL;
    char *dst = (pool_next < MSGPOOL) ? msgpool[pool_next++][0] : fallback;
    snprintf(dst, 76, "%s", s);
    return dst;
}

static void q_msg(const char *l1, const char *l2)
{
    qf(BS_MSG, 0, 0, pool_put(l1), pool_put(l2));
}

static void q_msgf(const char *fmt, ...)
{
    char buf[76];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    q_msg(buf, NULL);
}

static const char *const STAT_NAMES[6] = {
    "?", "ATTACK", "DEFENSE", "SP.ATK", "SP.DEF", "SPEED"
};

/* ---- start ---- */
static void battle_common_init(void)
{
    memset(&B, 0, sizeof(B));
    B.active = true;
    B.phase = BP_EXEC;
    q_clear();
    g.active_slot = 0;
    for (int i = 0; i < g.party_n; i++)
        if (g.party[i].hp > 0) { g.active_slot = (uint8_t)i; break; }
    B.php_show = (int16_t)pc()->hp;
}

void battle_start_wild(uint8_t species, uint8_t level)
{
    battle_common_init();
    creature_init(&B.enemy, species, level, 1);
    dex_see(species);
    B.ehp_show = (int16_t)B.enemy.hp;
    q_msgf("A wild %s appeared!", ename());
    q_msgf("Go! %s!", pname());
    qf(BS_MENU, 0, 0, NULL, NULL);
}

void battle_start_trainer(const TrainerDef *t)
{
    battle_common_init();
    B.trainer = true;
    B.tdef = t;
    creature_init(&B.enemy, t->species[0], t->levels[0], 1);
    dex_see(t->species[0]);
    B.ehp_show = (int16_t)B.enemy.hp;
    q_msgf("%s wants to battle!", t->name);
    q_msgf("%s sent out %s!", t->name, ename());
    q_msgf("Go! %s!", pname());
    qf(BS_MENU, 0, 0, NULL, NULL);
}

/* ---- speed helpers ---- */
static uint32_t eff_speed(const Creature *c, const int8_t *stages)
{
    uint32_t s = stat_after_stage(c->stats[ST_SPE], stages[ST_SPE]);
    if (c->ailment == AIL_PARA)
        s /= 2;
    return s;
}

/* ---- move resolution (runs instantly, queues follow-ups) ---- */
static void resolve_move(uint8_t actor, uint8_t slot)
{
    Creature *att = actor ? &B.enemy : pc();
    Creature *def = actor ? pc() : &B.enemy;
    int8_t *ast = actor ? B.estages : B.pstages;
    int8_t *dst = actor ? B.pstages : B.estages;
    const char *an = actor ? ename() : pname();
    const char *dn = actor ? pname() : ename();

    if (att->hp == 0)
        return;

    /* sleep: counts down at the sleeper's turn */
    if (att->ailment == AIL_SLEEP) {
        if (att->sleep_turns > 0) {
            att->sleep_turns--;
            q_msgf("%s is fast asleep!", an);
            if (att->sleep_turns == 0) {
                att->ailment = AIL_NONE;
                q_msgf("%s woke up!", an);
            }
            return;
        }
        att->ailment = AIL_NONE;
    }
    if (att->ailment == AIL_PARA && rand() % 4 == 0) {
        q_msgf("%s is paralyzed!", an);
        q_msg("It can't move!", NULL);
        return;
    }
    if (slot >= 4 || att->moves[slot] == 0xFF || att->pp[slot] == 0) {
        q_msgf("%s has no moves left!", an);
        return;
    }
    att->pp[slot]--;
    const Move *mv = &MOVES[att->moves[slot]];
    q_msgf("%s used %s!", an, mv->name);

    if (mv->category == MC_STATUS) {
        if (mv->stat_id >= 0) {
            int8_t *tst = (mv->target == MT_SELF) ? ast : dst;
            const char *tn = (mv->target == MT_SELF) ? an : dn;
            int8_t cur = tst[mv->stat_id];
            int8_t nx = (int8_t)(cur + mv->stat_stages);
            if (nx > 6) nx = 6;
            if (nx < -6) nx = -6;
            if (nx == cur) {
                q_msg(mv->stat_stages > 0 ? "It won't go any higher!" :
                                            "It won't go any lower!", NULL);
            } else {
                tst[mv->stat_id] = nx;
                q_msgf("%s's %s %s!", tn, STAT_NAMES[mv->stat_id],
                       mv->stat_stages > 0 ? "rose" : "fell");
            }
        } else if (mv->effect == ME_SLEEP) {
            if (def->ailment != AIL_NONE) {
                q_msg("But it failed!", NULL);
            } else {
                def->ailment = AIL_SLEEP;
                def->sleep_turns = (uint8_t)(1 + rand() % 3);
                q_msgf("%s fell asleep!", dn);
            }
        }
        return;
    }

    DamageResult r = move_damage(att, def, ast, dst, att->moves[slot]);
    if (r.missed) {
        q_msgf("%s's attack missed!", an);
        return;
    }
    if (r.effectiveness == -2) {
        q_msgf("It doesn't affect %s...", dn);
        return;
    }
    int newhp = def->hp - r.damage;
    if (newhp < 0) newhp = 0;
    def->hp = (uint16_t)newhp;
    qf(BS_WAIT_HP, 0, 0, NULL, NULL);
    if (r.crit)
        q_msg("A critical hit!", NULL);
    if (r.effectiveness > 0)
        q_msg("It's super effective!", NULL);
    else if (r.effectiveness == -1)
        q_msg("It's not very effective...", NULL);
    if (mv->effect == ME_DRAIN && r.damage > 0) {
        int heal = r.damage / 2;
        int nh = att->hp + heal;
        if (nh > att->stats[ST_HP]) nh = att->stats[ST_HP];
        att->hp = (uint16_t)nh;
        qf(BS_WAIT_HP, 0, 0, NULL, NULL);
        q_msgf("%s drained energy!", an);
    }
    /* secondary effects */
    if (def->hp > 0 && mv->effect_chance > 0 && rand() % 100 < mv->effect_chance) {
        if ((mv->effect == ME_BURN || mv->effect == ME_PARA) && def->ailment == AIL_NONE) {
            def->ailment = (mv->effect == ME_BURN) ? AIL_BURN : AIL_PARA;
            q_msgf("%s was %s!", dn,
                   mv->effect == ME_BURN ? "burned" : "paralyzed");
        } else if (mv->stat_id >= 0 && mv->target == MT_FOE) {
            int8_t cur = dst[mv->stat_id];
            int8_t nx = (int8_t)(cur + mv->stat_stages);
            if (nx > 6) nx = 6;
            if (nx < -6) nx = -6;
            if (nx != cur) {
                dst[mv->stat_id] = nx;
                q_msgf("%s's %s %s!", dn, STAT_NAMES[mv->stat_id],
                       mv->stat_stages > 0 ? "rose" : "fell");
            }
        }
    }
}

/* ---- enemy free turn (after item / switch / failed run) ---- */
static void enemy_turn(void)
{
    int emv = pick_enemy_move(&B.enemy, pc());
    if (emv >= 0)
        qf(BS_ACT_MOVE, 1, (uint8_t)emv, NULL, NULL);
    qf(BS_CHECK, 0, 0, NULL, NULL);
    qf(BS_END_TURN, 0, 0, NULL, NULL);
    qf(BS_MENU, 0, 0, NULL, NULL);
}

/* ---- turn construction ---- */
static void build_turn(int pslot)
{
    int emv = pick_enemy_move(&B.enemy, pc());
    const Move *pm = &MOVES[pc()->moves[pslot]];
    bool player_first = true;
    if (emv >= 0) {
        const Move *em = &MOVES[B.enemy.moves[emv]];
        if (em->priority > pm->priority)
            player_first = false;
        else if (em->priority == pm->priority) {
            uint32_t ps = eff_speed(pc(), B.pstages);
            uint32_t es = eff_speed(&B.enemy, B.estages);
            if (es > ps || (es == ps && rand() % 2 == 0))
                player_first = false;
        }
    }
    if (player_first) {
        qf(BS_ACT_MOVE, 0, (uint8_t)pslot, NULL, NULL);
        qf(BS_CHECK, 0, 0, NULL, NULL);
        if (emv >= 0) {
            qf(BS_ACT_MOVE, 1, (uint8_t)emv, NULL, NULL);
            qf(BS_CHECK, 0, 0, NULL, NULL);
        }
    } else {
        qf(BS_ACT_MOVE, 1, (uint8_t)emv, NULL, NULL);
        qf(BS_CHECK, 0, 0, NULL, NULL);
        qf(BS_ACT_MOVE, 0, (uint8_t)pslot, NULL, NULL);
        qf(BS_CHECK, 0, 0, NULL, NULL);
    }
    qf(BS_END_TURN, 0, 0, NULL, NULL);
    qf(BS_MENU, 0, 0, NULL, NULL);
}

/* ---- xp / level / learn / evolve ---- */
static void resolve_xp(void)
{
    Creature *c = pc();
    if (c->hp == 0)
        return; /* no xp for a fainted participant (v1: active only) */
    uint32_t xp = (uint32_t)SPECIES[B.enemy.species].xp_yield * B.enemy.level / 7;
    if (B.trainer)
        xp = xp * 3 / 2;
    if (xp < 1) xp = 1;
    uint8_t oldlvl = c->level;
    int gained = 0;
    creature_add_xp(c, (uint16_t)xp, &gained);
    q_msgf("%s gained %u EXP!", pname(), (unsigned)xp);
    if (gained > 0)
        q_msgf("%s grew to Lv%u!", pname(), c->level);
    /* new moves learned between old level and new level */
    const Species *sp = &SPECIES[c->species];
    for (int i = 0; i < LEARN_MAX && sp->learn[i] != LEARN_END; i++) {
        uint8_t lvl = (uint8_t)(sp->learn[i] >> 8);
        if (lvl > oldlvl && lvl <= c->level) {
            uint8_t mv = (uint8_t)(sp->learn[i] & 0xFF);
            int replaced = -1;
            if (learn_move(c, mv, &replaced)) {
                if (replaced >= 0)
                    q_msgf("Forgot %s...", MOVES[replaced].name);
                q_msgf("Learned %s!", MOVES[mv].name);
            }
        }
    }
    /* evolution */
    sp = &SPECIES[c->species];
    if (sp->evolve_level > 0 && c->level >= sp->evolve_level) {
        char oldname[24];
        snprintf(oldname, sizeof(oldname), "%s", sp->name);
        uint8_t evolved = sp->evolve_to;
        q_msgf("What? %s is evolving!", oldname);
        c->species = evolved;
        creature_recalc_stats(c, 1);
        dex_own(evolved);
        q_msgf("Congratulations! Your %s", oldname);
        q_msgf("evolved into %s!", SPECIES[evolved].name);
    }
}

/* ---- checks ---- */
static void resolve_check(void)
{
    if (B.enemy.hp == 0) {
        q_clear();
        qf(BS_FAINT, 1, 0, NULL, NULL);
        q_msgf(B.trainer ? "Enemy %s fainted!" : "Wild %s fainted!", ename());
        qf(BS_XP, 0, 0, NULL, NULL);
        if (B.trainer && B.trainer_next + 1 < B.tdef->count) {
            qf(BS_ENEMY_NEXT, 0, 0, NULL, NULL);
        } else if (B.trainer) {
            q_msgf("You defeated %s!", B.tdef->name);
            g.money = (uint16_t)(g.money + B.tdef->reward);
            q_msgf("You got $%u for winning!", B.tdef->reward);
            qf(BS_END, 3, 0, NULL, NULL);
        } else {
            qf(BS_END, 0, 0, NULL, NULL);
        }
        return;
    }
    if (pc()->hp == 0) {
        q_clear();
        qf(BS_FAINT, 0, 0, NULL, NULL);
        q_msgf("%s fainted!", pname());
        bool any = false;
        for (int i = 0; i < g.party_n; i++)
            if (g.party[i].hp > 0) { any = true; break; }
        if (any) {
            B.forced_switch = true;
        } else {
            q_msg("You're out of usable creatures!", NULL);
            q_msg("You panicked and rushed home...", NULL);
            qf(BS_END, 1, 0, NULL, NULL);
        }
        return;
    }
}

/* ---- end of turn ---- */
static void resolve_end_turn(void)
{
    Creature *c = pc();
    Creature *e = &B.enemy;
    if (c->hp > 0 && c->ailment == AIL_BURN) {
        int dmg = c->stats[ST_HP] / 16;
        if (dmg < 1) dmg = 1;
        c->hp = (uint16_t)(c->hp > dmg ? c->hp - dmg : 0);
        qf(BS_WAIT_HP, 0, 0, NULL, NULL);
        q_msgf("%s is hurt by its burn!", pname());
    }
    if (e->hp > 0 && e->ailment == AIL_BURN) {
        int dmg = e->stats[ST_HP] / 16;
        if (dmg < 1) dmg = 1;
        e->hp = (uint16_t)(e->hp > dmg ? e->hp - dmg : 0);
        qf(BS_WAIT_HP, 0, 0, NULL, NULL);
        q_msgf("%s is hurt by its burn!", ename());
    }
}

/* ---- catch ---- */
static void resolve_throw(uint8_t item)
{
    if (B.trainer) {
        q_msg("The trainer blocked the ORB!", NULL);
        q_msg("Don't be a thief!", NULL);
        enemy_turn();
        return;
    }
    bag_consume(item);
    int bonus = ITEMS[item].power; /* x10 */
    int shakes = catch_shakes(&B.enemy, bonus);
    q_msgf("You threw a %s!", ITEMS[item].name);
    for (int i = 0; i < shakes && i < 3; i++)
        qf(BS_SHAKE, 18, 0, NULL, NULL);
    if (shakes >= 4) {
        qf(BS_SHAKE, 18, 0, NULL, NULL);
        dex_own(B.enemy.species);
        bool stored = false;
        if (g.party_n < MAX_PARTY) {
            g.party[g.party_n] = B.enemy;
            g.party[g.party_n].ailment = AIL_NONE;
            g.party_n++;
        } else {
            stored = true;
        }
        q_msgf("Gotcha! %s was caught!", ename());
        if (stored)
            q_msg("Party full! Sent to STORAGE.", NULL);
        qf(BS_END, 2, 0, NULL, NULL);
    } else {
        q_msg("Oh no! It broke free!", NULL);
        enemy_turn();
    }
}

/* ---- update ---- */
static void advance_hp_anim(void)
{
    Creature *c = pc();
    int pspeed = c->stats[ST_HP] / 24;
    if (pspeed < 1) pspeed = 1;
    if (B.php_show < (int)c->hp) B.php_show = (int16_t)(B.php_show + pspeed);
    if (B.php_show > (int)c->hp) B.php_show = (int16_t)(B.php_show - pspeed);
    int espeed = B.enemy.stats[ST_HP] / 24;
    if (espeed < 1) espeed = 1;
    if (B.ehp_show < (int)B.enemy.hp) B.ehp_show = (int16_t)(B.ehp_show + espeed);
    if (B.ehp_show > (int)B.enemy.hp) B.ehp_show = (int16_t)(B.ehp_show - espeed);
}

static bool hp_settled(void)
{
    return B.php_show == (int)pc()->hp && B.ehp_show == (int)B.enemy.hp;
}

static void send_out(int slot)
{
    g.active_slot = (uint8_t)slot;
    memset(B.pstages, 0, sizeof(B.pstages));
    B.php_show = (int16_t)pc()->hp;
    B.faint_off[0] = 0;
}

static void do_switch(int slot)
{
    q_msgf("Come back, %s!", pname());
    qf(BS_SWITCH_IN, (uint8_t)slot, 0, NULL, NULL);
    q_msgf("Go! %s!", SPECIES[g.party[slot].species].name);
    enemy_turn();
}

void battle_update(void)
{
    if (!B.active || B.over)
        return;

    /* dialogs run to completion first */
    if (dlg_active()) {
        dlg_update();
        if (g_dlg.done) {
            g_dlg.done = false;
            if (qhead < qlen && q[qhead].kind == BS_MSG)
                q_pop();
        }
        return;
    }

    /* animations that belong to the current head step */
    if (qhead < qlen) {
        Step *s = &q[qhead];
        switch (s->kind) {
        case BS_MSG:
            if (s->l1) {
                const char *lines[2] = { s->l1, s->l2 };
                dlg_start(lines, s->l2 ? 2 : 1);
                return; /* dialog now active; pops on completion */
            }
            q_pop();
            return;
        case BS_DELAY:
            if (s->a > 0) { s->a--; return; }
            q_pop();
            return;
        case BS_SHAKE:
            if (s->a > 0) { s->a--; B.shake_timer = s->a; return; }
            B.shake_timer = 0;
            q_pop();
            return;
        case BS_WAIT_HP:
            advance_hp_anim();
            if (hp_settled())
                q_pop();
            return;
        case BS_FAINT:
            if (s->a == 0) {
                B.faint_off[0] += 6;
                if (B.faint_off[0] >= 64) { B.faint_off[0] = 200; q_pop(); }
            } else {
                B.faint_off[1] += 6;
                if (B.faint_off[1] >= 64) { B.faint_off[1] = 200; q_pop(); }
            }
            return;
        default:
            break;
        }
    }

    /* execute steps */
    while (qhead < qlen) {
        Step s = q[qhead];
        q_pop();
        switch (s.kind) {
        case BS_ACT_MOVE:
            resolve_move(s.a, s.b);
            break;
        case BS_CHECK:
            resolve_check();
            break;
        case BS_END_TURN:
            resolve_end_turn();
            break;
        case BS_THROW:
            resolve_throw(s.a);
            break;
        case BS_XP:
            resolve_xp();
            break;
        case BS_SWITCH_IN:
            send_out(s.a);
            break;
        case BS_ENEMY_NEXT: {
            B.trainer_next++;
            creature_init(&B.enemy, B.tdef->species[B.trainer_next],
                          B.tdef->levels[B.trainer_next], 1);
            dex_see(B.tdef->species[B.trainer_next]);
            memset(B.estages, 0, sizeof(B.estages));
            B.ehp_show = (int16_t)B.enemy.hp;
            B.faint_off[1] = 0;
            q_msgf("%s sent out %s!", B.tdef->name, ename());
            qf(BS_MENU, 0, 0, NULL, NULL);
            break;
        }
        case BS_MENU:
            B.phase = BP_MENU;
            break;
        case BS_END:
            B.over = true;
            B.result = s.a;
            B.phase = BP_DONE;
            break;
        default:
            break;
        }
        /* blocking steps (dialog started, etc.) stop the loop */
        if (dlg_active() || (qhead < qlen && (q[qhead].kind == BS_MSG && q[qhead].l1)))
            return;
        if (qhead < qlen) {
            uint8_t k = q[qhead].kind;
            if (k == BS_DELAY || k == BS_SHAKE || k == BS_WAIT_HP || k == BS_FAINT)
                return;
        }
    }

    /* queue drained: forced switch or menus */
    if (B.forced_switch) {
        if (!party_active())
            party_open(PM_SWITCH);
        party_update();
        if (!party_active()) {
            int slot = party_result();
            if (slot >= 0) {
                B.forced_switch = false;
                send_out(slot);
                q_msgf("Go! %s!", pname());
                enemy_turn();
            } else {
                party_open(PM_SWITCH); /* must choose */
            }
        }
        return;
    }

    switch (B.phase) {
    case BP_MENU:
        if (g_in.pressed[BTN_LEFT]) B.menu_cur = (B.menu_cur + 2) % 4;
        if (g_in.pressed[BTN_RIGHT]) B.menu_cur = (B.menu_cur + 2) % 4;
        if (g_in.pressed[BTN_UP]) B.menu_cur = B.menu_cur < 2 ? B.menu_cur : (uint8_t)(B.menu_cur - 2);
        if (g_in.pressed[BTN_DOWN]) B.menu_cur = B.menu_cur < 2 ? (uint8_t)(B.menu_cur + 2) : B.menu_cur;
        if (g_in.pressed[BTN_A]) {
            B.phase = BP_EXEC;
            if (B.menu_cur == 0) {
                B.phase = BP_FIGHT;
                B.fight_cur = 0;
            } else if (B.menu_cur == 1) {
                B.phase = BP_BAG;
                bag_open(BM_BATTLE);
            } else if (B.menu_cur == 2) {
                B.phase = BP_PARTY;
                party_open(PM_SWITCH);
            } else {
                if (B.trainer) {
                    q_msg("No! There's no running", NULL);
                    q_msg("from a trainer battle!", NULL);
                    enemy_turn();
                } else {
                    B.run_attempts++;
                    uint32_t ps = eff_speed(pc(), B.pstages);
                    uint32_t es = eff_speed(&B.enemy, B.estages);
                    uint32_t odds = (es ? ps * 128 / es : 256) + 30u * (uint32_t)B.run_attempts;
                    odds %= 256;
                    if ((uint32_t)(rand() % 256) < odds) {
                        q_msg("Got away safely!", NULL);
                        qf(BS_END, 0, 0, NULL, NULL);
                    } else {
                        q_msg("Can't escape!", NULL);
                        enemy_turn();
                    }
                }
            }
        }
        break;
    case BP_FIGHT: {
        int n = 0;
        int slots[4];
        for (int i = 0; i < 4; i++)
            if (pc()->moves[i] != 0xFF)
                slots[n++] = i;
        if (n == 0) { /* shouldn't happen */
            B.phase = BP_MENU;
            break;
        }
        if (g_in.pressed[BTN_UP]) B.fight_cur--;
        if (g_in.pressed[BTN_DOWN]) B.fight_cur++;
        if (B.fight_cur < 0) B.fight_cur = n - 1;
        if (B.fight_cur >= n) B.fight_cur = 0;
        if (g_in.pressed[BTN_B]) {
            B.phase = BP_MENU;
        } else if (g_in.pressed[BTN_A]) {
            int slot = slots[B.fight_cur];
            if (pc()->pp[slot] == 0) {
                q_msg("No PP left for this move!", NULL);
                qf(BS_MENU, 0, 0, NULL, NULL);
            } else {
                build_turn(slot);
            }
            B.phase = BP_EXEC;
        }
        break;
    }
    case BP_BAG:
        bag_update();
        if (!bag_active()) {
            int item = bag_result();
            B.phase = BP_EXEC;
            if (item < 0) {
                B.phase = BP_MENU;
            } else if (ITEMS[item].kind == IK_ORB) {
                resolve_throw((uint8_t)item);
            } else { /* potion on active creature */
                Creature *c = pc();
                if (c->hp >= c->stats[ST_HP]) {
                    q_msg("It won't have any effect.", NULL);
                    qf(BS_MENU, 0, 0, NULL, NULL);
                } else {
                    bag_consume((uint8_t)item);
                    uint16_t before = c->hp;
                    c->hp = (uint16_t)(c->hp + ITEMS[item].power);
                    if (c->hp > c->stats[ST_HP]) c->hp = c->stats[ST_HP];
                    (void)before;
                    q_msgf("You used a %s!", ITEMS[item].name);
                    qf(BS_WAIT_HP, 0, 0, NULL, NULL);
                    q_msgf("%s recovered HP!", pname());
                    enemy_turn();
                }
            }
        }
        break;
    case BP_PARTY:
        party_update();
        if (!party_active()) {
            int slot = party_result();
            B.phase = BP_EXEC;
            if (slot < 0) {
                B.phase = BP_MENU;
            } else {
                do_switch(slot);
            }
        }
        break;
    default:
        break;
    }
}

/* ---- draw ---- */
static void draw_sprite_with_faint(SDL_Renderer *r, int species, int x, int y,
                                   int size, bool flip, int off, int shake)
{
    if (off >= 200)
        return;
    int sx = x + (shake ? (((shake >> 2) & 1) ? 4 : -4) : 0);
    draw_creature(r, species, sx, y + off, size, flip);
}

void battle_draw(SDL_Renderer *r)
{
    if (!B.active)
        return;
    SDL_SetRenderDrawColor(r, 248, 248, 248, 255);
    SDL_RenderClear(r);

    /* platforms */
    if (A.platforms.tex) {
        SDL_Rect e_src = { 0, 0, 80, 20 };
        SDL_Rect e_dst = { 148, 64, 68, 17 };
        SDL_RenderCopy(r, A.platforms.tex, &e_src, &e_dst);
        SDL_Rect p_src = { 80, 0, 80, 20 };
        SDL_Rect p_dst = { 26, 100, 100, 25 };
        SDL_RenderCopy(r, A.platforms.tex, &p_src, &p_dst);
    }

    /* sprites */
    draw_sprite_with_faint(r, B.enemy.species, 158, 6, 64, false,
                           B.faint_off[1], B.shake_timer);
    draw_sprite_with_faint(r, pc()->species, 12, 38, 96, true,
                           B.faint_off[0], 0);

    /* enemy panel */
    SDL_Color dark = { 56, 56, 64, 255 };
    SDL_Color red = { 200, 48, 48, 255 };
    char buf[40];
    draw_panel(r, 4, 4, 108, 30);
    snprintf(buf, sizeof(buf), "%s", ename());
    draw_text(r, 10, 8, buf, dark, 1);
    snprintf(buf, sizeof(buf), "Lv%u", B.enemy.level);
    draw_text(r, 74, 8, buf, dark, 1);
    draw_hpbar(r, 8, 22, 98, (uint16_t)(B.ehp_show < 0 ? 0 : B.ehp_show),
               B.enemy.stats[ST_HP]);

    /* player panel */
    draw_panel(r, 122, 68, 114, 40);
    snprintf(buf, sizeof(buf), "%s", pname());
    draw_text(r, 128, 72, buf, dark, 1);
    snprintf(buf, sizeof(buf), "Lv%u", pc()->level);
    draw_text(r, 196, 72, buf, dark, 1);
    draw_hpbar(r, 126, 86, 104, (uint16_t)(B.php_show < 0 ? 0 : B.php_show),
               pc()->stats[ST_HP]);
    snprintf(buf, sizeof(buf), "%3u/%-3u", (unsigned)(B.php_show < 0 ? 0 : B.php_show),
             pc()->stats[ST_HP]);
    draw_text(r, 128, 94, buf, dark, 1);
    if (pc()->ailment == AIL_BURN) draw_text(r, 196, 94, "BRN", (SDL_Color){ 224, 96, 32, 255 }, 1);
    else if (pc()->ailment == AIL_PARA) draw_text(r, 196, 94, "PAR", (SDL_Color){ 200, 176, 32, 255 }, 1);
    else if (pc()->ailment == AIL_SLEEP) draw_text(r, 196, 94, "SLP", (SDL_Color){ 120, 120, 168, 255 }, 1);
    draw_xpbar(r, 128, 103, 104, pc());

    /* trainer indicator */
    if (B.trainer) {
        draw_text(r, 158, 6, "!", red, 1);
    }

    /* textbox / menus */
    bool menu_up = (B.phase == BP_MENU) && !dlg_active();
    if (menu_up) {
        draw_textbox(r);
        draw_text(r, 10, SCREEN_H - TEXTBOX_H + 5,
                  "What will you do?", dark, 1);
        draw_panel(r, 122, SCREEN_H - TEXTBOX_H - 2, 116, TEXTBOX_H);
        const char *items[4] = { "FIGHT", "BAG", "TEAM", "RUN" };
        for (int i = 0; i < 4; i++) {
            int col = i % 2, row = i / 2;
            int x = 130 + col * 56, y = SCREEN_H - TEXTBOX_H + 3 + row * 16;
            if (i == B.menu_cur)
                draw_text(r, x, y, ">", red, 1);
            draw_text(r, x + 10, y, items[i], dark, 1);
        }
    } else if (B.phase == BP_FIGHT && !dlg_active()) {
        draw_textbox(r);
        draw_panel(r, 2, SCREEN_H - TEXTBOX_H - 2, 118, TEXTBOX_H);
        int shown = 0;
        for (int i = 0; i < 4; i++) {
            if (pc()->moves[i] == 0xFF)
                continue;
            int y = SCREEN_H - TEXTBOX_H + 1 + shown * 11;
            if (shown == B.fight_cur)
                draw_text(r, 8, y, ">", red, 1);
            draw_text(r, 18, y, MOVES[pc()->moves[i]].name, dark, 1);
            shown++;
        }
        /* info panel */
        draw_panel(r, 122, SCREEN_H - TEXTBOX_H - 2, 116, TEXTBOX_H);
        if (pc()->moves[B.fight_cur] != 0xFF) {
            const Move *mv = &MOVES[pc()->moves[B.fight_cur]];
            snprintf(buf, sizeof(buf), "%s/", type_name(mv->type));
            draw_text(r, 130, SCREEN_H - TEXTBOX_H + 3, buf, dark, 1);
            snprintf(buf, sizeof(buf), "PP %u/%u", pc()->pp[B.fight_cur], mv->pp);
            draw_text(r, 130, SCREEN_H - TEXTBOX_H + 15, buf, dark, 1);
        }
    } else {
        dlg_draw(r);
    }

    party_draw(r);
    bag_draw(r);
}
