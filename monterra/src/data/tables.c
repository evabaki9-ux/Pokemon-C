/* Data tables: moves, type chart, species, items. All original content. */
#include "game.h"

/* Move ids */
enum {
    MV_TACKLE, MV_SCRATCH, MV_GROWL, MV_TAILWHIP, MV_HOWL, MV_HARDEN,
    MV_QUICKPECK, MV_WINGSLAP, MV_GUST, MV_EMBER, MV_FLAMEBURST,
    MV_WATERGUN, MV_BUBBLEBEAM, MV_VINEWHIP, MV_LEAFBLADE, MV_ABSORB,
    MV_SPARK, MV_THUNDERJOLT, MV_ROCKTOSS, MV_BOULDERSLAM, MV_BUGBITE,
    MV_STRINGSHOT, MV_SLEEPPOWDER, MV_HEADBUTT
};

#define M(name, type, power, acc, pp, cat, prio, eff, chance, stat, stages, tgt) \
    { name, type, power, acc, pp, cat, prio, eff, chance, stat, stages, tgt }

const Move MOVES[NUM_MOVES] = {
    M("TACKLE",       TY_NORMAL,  40, 100, 35, MC_PHYS, 0, ME_NONE,      0,  -1, 0, MT_FOE),
    M("SCRATCH",      TY_NORMAL,  40, 100, 35, MC_PHYS, 0, ME_NONE,      0,  -1, 0, MT_FOE),
    M("GROWL",        TY_NORMAL,   0, 100, 40, MC_STATUS, 0, ME_NONE,    0, ST_ATK, -1, MT_FOE),
    M("TAIL WHIP",    TY_NORMAL,   0, 100, 30, MC_STATUS, 0, ME_NONE,    0, ST_DEF, -1, MT_FOE),
    M("HOWL",         TY_NORMAL,   0,   0, 40, MC_STATUS, 0, ME_NONE,    0, ST_ATK, +1, MT_SELF),
    M("HARDEN",       TY_NORMAL,   0,   0, 30, MC_STATUS, 0, ME_NONE,    0, ST_DEF, +1, MT_SELF),
    M("QUICK PECK",   TY_FLYING,  40, 100, 30, MC_PHYS, 1, ME_NONE,      0,  -1, 0, MT_FOE),
    M("WING SLAP",    TY_FLYING,  60, 100, 25, MC_PHYS, 0, ME_NONE,      0,  -1, 0, MT_FOE),
    M("GUST",         TY_FLYING,  40, 100, 35, MC_SPEC, 0, ME_NONE,      0,  -1, 0, MT_FOE),
    M("EMBER",        TY_FIRE,    40, 100, 25, MC_SPEC, 0, ME_BURN,     10,  -1, 0, MT_FOE),
    M("FLAME BURST",  TY_FIRE,    65, 100, 15, MC_SPEC, 0, ME_NONE,      0,  -1, 0, MT_FOE),
    M("WATER GUN",    TY_WATER,   40, 100, 25, MC_SPEC, 0, ME_NONE,      0,  -1, 0, MT_FOE),
    M("BUBBLE BEAM",  TY_WATER,   65, 100, 15, MC_SPEC, 0, ME_NONE,     10, ST_SPE, -1, MT_FOE),
    M("VINE WHIP",    TY_GRASS,   45, 100, 25, MC_PHYS, 0, ME_NONE,      0,  -1, 0, MT_FOE),
    M("LEAF BLADE",   TY_GRASS,   70,  95, 15, MC_PHYS, 0, ME_HIGH_CRIT, 0, -1, 0, MT_FOE),
    M("ABSORB",       TY_GRASS,   40, 100, 25, MC_SPEC, 0, ME_DRAIN,     0,  -1, 0, MT_FOE),
    M("SPARK",        TY_ELECTRIC,40, 100, 30, MC_SPEC, 0, ME_PARA,     30,  -1, 0, MT_FOE),
    M("THUNDER JOLT", TY_ELECTRIC,65, 100, 15, MC_SPEC, 0, ME_PARA,     10,  -1, 0, MT_FOE),
    M("ROCK TOSS",    TY_ROCK,    50,  90, 20, MC_PHYS, 0, ME_NONE,      0,  -1, 0, MT_FOE),
    M("BOULDER SLAM", TY_ROCK,    75,  85, 10, MC_PHYS, 0, ME_NONE,      0,  -1, 0, MT_FOE),
    M("BUG BITE",     TY_BUG,     45, 100, 25, MC_PHYS, 0, ME_NONE,      0,  -1, 0, MT_FOE),
    M("STRING SHOT",  TY_BUG,      0,  95, 40, MC_STATUS, 0, ME_NONE,    0, ST_SPE, -1, MT_FOE),
    M("SLEEP POWDER", TY_GRASS,    0,  75, 15, MC_STATUS, 0, ME_SLEEP, 100,  -1, 0, MT_FOE),
    M("HEADBUTT",     TY_NORMAL,  70, 100, 15, MC_PHYS, 0, ME_NONE,      0,  -1, 0, MT_FOE),
};

static const char *TYPE_NAMES[TY_COUNT] = {
    "NORMAL", "FIRE", "WATER", "GRASS", "ELEC", "ROCK", "FLYING", "BUG"
};

const char *type_name(uint8_t t)
{
    if (t >= TY_COUNT) return "???";
    return TYPE_NAMES[t];
}

/* type chart: atk x def => multiplier x10 (10 = 1.0) */
static const int8_t TYPE_CHART[TY_COUNT][TY_COUNT] = {
    /*             NRM  FIR  WAT  GRS  ELC  RCK  FLY  BUG  */
    /* NORMAL */ {  10,  10,  10,  10,  10,  10,  10,  10 },
    /* FIRE   */ {  10,   5,   5,  20,  10,   5,  10,  20 },
    /* WATER  */ {  10,  20,   5,   5,  10,  20,  10,  10 },
    /* GRASS  */ {  10,   5,  20,   5,  10,  20,   5,   5 },
    /* ELEC   */ {  10,  10,  20,   5,   5,  10,  20,  10 },
    /* ROCK   */ {  10,  20,  10,  10,  10,  10,  20,  10 },
    /* FLYING */ {  10,  10,  10,  20,   5,   5,  10,  20 },
    /* BUG    */ {  10,   5,  10,  20,  10,  10,   5,  10 },
};

int type_chart_lookup(uint8_t atk, uint8_t def)
{
    if (atk >= TY_COUNT || def >= TY_COUNT)
        return 10;
    return TYPE_CHART[atk][def];
}

/* species art keys: file in assets/creatures, fallback chain for evolutions */
const Item ITEMS[NUM_ITEM_IDS] = {
    { "POTION", IK_HEAL, 20, 300 },
    { "SUPER POTION", IK_HEAL, 50, 700 },
    { "ORB", IK_ORB, 10, 200 },      /* ball bonus x10 */
    { "GREAT ORB", IK_ORB, 15, 600 },
};

const char *const SPECIES_ART[NUM_SPECIES] = {
    "emberit", "dewlin", "sproutle", "fluffit", "pebbly", "zephird",
    "mossling", "sparkit", "emberit", "dewlin", "sproutle", "fluffit",
};
/* tint evolved placeholders so they read as different creatures */
const uint8_t SPECIES_TINT[NUM_SPECIES][3] = {
    {255,255,255}, {255,255,255}, {255,255,255}, {255,255,255},
    {255,255,255}, {255,255,255}, {255,255,255}, {255,255,255},
    {255, 200, 160},   /* PYROGON: warmer */
    {160, 210, 255},   /* TORRENTOL: cooler */
    {190, 255, 170},   /* VERDANTIS: greener */
    {255, 190, 230},   /* LOPPIN: pinker */
};

#define S(name, t1, t2, hp, at, df, sa, sd, sp, cr, xp, el, et, ...) \
    { name, t1, t2, { hp, at, df, sa, sd, sp }, cr, xp, el, et, { __VA_ARGS__ } }
#define L(lvl, mv) ((uint16_t)((lvl) << 8 | (mv)))
#define END LEARN_END

const Species SPECIES[NUM_SPECIES] = {
    S("EMBERIT", TY_FIRE, TY_NONE, 44, 52, 43, 60, 50, 65, 45, 62, 16, SP_PYROGON,
      L(1, MV_TACKLE), L(1, MV_GROWL), L(6, MV_EMBER), L(12, MV_HEADBUTT),
      L(17, MV_FLAMEBURST), L(22, MV_HOWL), END),
    S("DEWLIN", TY_WATER, TY_NONE, 44, 48, 52, 56, 56, 48, 45, 63, 16, SP_TORRENTOL,
      L(1, MV_TACKLE), L(1, MV_TAILWHIP), L(6, MV_WATERGUN), L(12, MV_HEADBUTT),
      L(17, MV_BUBBLEBEAM), L(22, MV_HARDEN), END),
    S("SPROUTLE", TY_GRASS, TY_NONE, 45, 49, 49, 52, 56, 45, 45, 64, 16, SP_VERDANTIS,
      L(1, MV_TACKLE), L(1, MV_GROWL), L(6, MV_ABSORB), L(11, MV_VINEWHIP),
      L(17, MV_SLEEPPOWDER), L(20, MV_LEAFBLADE), END),
    S("FLUFFIT", TY_NORMAL, TY_NONE, 46, 40, 38, 28, 34, 56, 255, 48, 18, SP_LOPPIN,
      L(1, MV_TACKLE), L(4, MV_GROWL), L(9, MV_HOWL), L(14, MV_HEADBUTT),
      L(19, MV_HARDEN), END),
    S("PEBBLY", TY_ROCK, TY_NONE, 48, 62, 80, 28, 40, 26, 180, 58, 0, 0,
      L(1, MV_SCRATCH), L(5, MV_HARDEN), L(9, MV_ROCKTOSS), L(15, MV_BOULDERSLAM), END),
    S("ZEPHIRD", TY_FLYING, TY_NONE, 40, 45, 38, 35, 34, 62, 255, 50, 0, 0,
      L(1, MV_QUICKPECK), L(5, MV_GUST), L(11, MV_WINGSLAP), L(17, MV_TAILWHIP), END),
    S("MOSSLING", TY_BUG, TY_GRASS, 50, 46, 58, 38, 52, 28, 255, 52, 0, 0,
      L(1, MV_BUGBITE), L(5, MV_STRINGSHOT), L(9, MV_ABSORB), L(15, MV_SLEEPPOWDER), END),
    S("SPARKIT", TY_ELECTRIC, TY_NONE, 38, 42, 34, 58, 42, 70, 190, 54, 0, 0,
      L(1, MV_SCRATCH), L(5, MV_SPARK), L(10, MV_HOWL), L(15, MV_THUNDERJOLT), END),
    S("PYROGON", TY_FIRE, TY_NONE, 58, 64, 53, 78, 62, 80, 45, 142, 0, 0,
      L(1, MV_TACKLE), L(1, MV_GROWL), L(6, MV_EMBER), L(12, MV_HEADBUTT),
      L(17, MV_FLAMEBURST), L(24, MV_HOWL), END),
    S("TORRENTOL", TY_WATER, TY_NONE, 58, 60, 64, 74, 70, 58, 45, 143, 0, 0,
      L(1, MV_TACKLE), L(1, MV_TAILWHIP), L(6, MV_WATERGUN), L(12, MV_HEADBUTT),
      L(17, MV_BUBBLEBEAM), L(24, MV_HARDEN), END),
    S("VERDANTIS", TY_GRASS, TY_NONE, 60, 62, 64, 66, 72, 55, 45, 144, 0, 0,
      L(1, MV_TACKLE), L(1, MV_GROWL), L(6, MV_ABSORB), L(11, MV_VINEWHIP),
      L(17, MV_SLEEPPOWDER), L(22, MV_LEAFBLADE), END),
    S("LOPPIN", TY_NORMAL, TY_NONE, 70, 56, 50, 42, 48, 78, 120, 118, 0, 0,
      L(1, MV_TACKLE), L(4, MV_GROWL), L(9, MV_HOWL), L(14, MV_HEADBUTT),
      L(19, MV_HARDEN), END),
};
