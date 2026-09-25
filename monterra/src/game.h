/* MONTERRA - an original monster-collecting RPG in the spirit of the GBA classics.
 * Pure data types shared by all modules. No SDL here so the logic core can be
 * unit-tested headlessly (tests/test_core.c).
 */
#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define SCREEN_W 240
#define SCREEN_H 160
#define TILE 16
#define MAX_PARTY 6
#define STORAGE_MAX 12
#define MAX_BAG 8
#define NUM_MOVES 29
#define NUM_ITEMS 4
#define NUM_MAPS 7
#define TEXTBOX_H 44
#define FONT_W 8
#define FONT_H 8

/* ---- types ---- */
enum { TY_NORMAL, TY_FIRE, TY_WATER, TY_GRASS, TY_ELECTRIC, TY_ROCK, TY_FLYING, TY_BUG, TY_COUNT };
/* secondary type "none" */
#define TY_NONE 0xFF

/* ---- stats ---- */
enum { ST_HP, ST_ATK, ST_DEF, ST_SAT, ST_SDF, ST_SPE };

/* ---- ailments ---- */
enum { AIL_NONE, AIL_BURN, AIL_PARA, AIL_SLEEP, AIL_POISON, AIL_FREEZE, AIL_CONFUSION };

/* ---- moves ---- */
enum { MC_PHYS, MC_SPEC, MC_STATUS };
enum { ME_NONE, ME_BURN, ME_PARA, ME_SLEEP, ME_DRAIN, ME_HIGH_CRIT, ME_RECOIL,
       ME_POISON, ME_FREEZE, ME_CONFUSE };
/* move ids (index into MOVES[]) */
enum {
    MV_TACKLE, MV_SCRATCH, MV_GROWL, MV_TAILWHIP, MV_HOWL, MV_HARDEN,
    MV_QUICKPECK, MV_WINGSLAP, MV_GUST, MV_EMBER, MV_FLAMEBURST,
    MV_WATERGUN, MV_BUBBLEBEAM, MV_VINEWHIP, MV_LEAFBLADE, MV_ABSORB,
    MV_SPARK, MV_THUNDERJOLT, MV_ROCKTOSS, MV_BOULDERSLAM, MV_BUGBITE,
    MV_STRINGSHOT, MV_SLEEPPOWDER, MV_HEADBUTT, MV_STRUGGLE,
    MV_VENOMSTING, MV_VENOMJAB, MV_DAZZLE, MV_COLDSNAP
};
enum { MT_FOE, MT_SELF };

typedef struct {
    const char *name;
    uint8_t type;
    uint8_t power;
    uint8_t accuracy; /* 0 = never miss */
    uint8_t pp;
    uint8_t category;
    int8_t priority;
    uint8_t effect;
    uint8_t effect_chance; /* % chance of secondary (100 for status moves) */
    int8_t stat_id;        /* >= 0 => this move changes a stat */
    int8_t stat_stages;
    uint8_t target;
} Move;

/* ---- species ---- */
#define LEARN_MAX 8
#define LEARN_END 0xFFFF

typedef struct {
    const char *name;
    uint8_t type1, type2;
    uint8_t base[6];
    uint8_t catch_rate;
    uint8_t xp_yield;
    uint8_t evolve_level; /* 0 = none */
    uint8_t evolve_to;
    uint16_t learn[LEARN_MAX]; /* (level << 8) | move_id, LEARN_END terminated */
} Species;

/* species ids */
enum {
    SP_EMBERIT, SP_DEWLIN, SP_SPROUTLE, SP_FLUFFIT, SP_PEBBLY, SP_ZEPHIRD,
    SP_MOSSLING, SP_SPARKIT, SP_PYROGON, SP_TORRENTOL, SP_VERDANTIS, SP_LOPPIN,
    NUM_SPECIES_TOTAL
};
#define NUM_SPECIES NUM_SPECIES_TOTAL

/* ---- items ---- */
enum { IK_HEAL, IK_ORB };
enum { IT_POTION, IT_SUPERPOTION, IT_ORB, IT_GREATORB, NUM_ITEM_IDS };

typedef struct {
    const char *name;
    uint8_t kind;
    int16_t power; /* heal amount or ball bonus x10 */
    uint16_t price;
} Item;

/* ---- creature instance ---- */
typedef struct {
    uint8_t species;
    uint8_t level;
    uint16_t xp; /* total xp */
    uint16_t hp;
    uint16_t stats[6];
    uint8_t ivs[6];
    uint8_t moves[4]; /* move ids, 0xFF = empty */
    uint8_t pp[4];
    uint8_t ailment;
    uint8_t sleep_turns;
    uint8_t conf_turns; /* volatile, battle-only; not saved */
} Creature;

/* ---- tiles (order mirrors tools/gen_tiles.py TILES[]) ---- */
enum {
    TI_GRASS, TI_TALL, TI_PATH, TI_FLOWER, TI_TREE, TI_WATER, TI_FENCE, TI_ROCK,
    TI_SAND, TI_BRIDGE, TI_LEDGE, TI_SIGN, TI_ROOF, TI_ROOF_L, TI_ROOF_R,
    TI_WALL, TI_WINDOW, TI_DOOR, TI_DOOR_HEAL, TI_FLOOR, TI_WALL_IN, TI_BED,
    TI_TABLE, TI_SHELF, TI_MAT, TI_PC, TI_COUNTER, TI_PLANT, TI_CARPET,
    TI_GRASS_TOWN, TI_COUNT
};

/* ---- maps ---- */
enum { MAP_HOUSE, MAP_LAB, MAP_HEAL, MAP_TOWN, MAP_ROUTE1, MAP_MART, MAP_ROUTE2 };

typedef struct {
    uint8_t x, y, dest_map, dest_x, dest_y;
} Warp;

enum { ROLE_NONE, ROLE_MOM, ROLE_NURSE, ROLE_PROF, ROLE_TRAINER, ROLE_BLOCKER, ROLE_SHOP };

typedef struct {
    const char *name;
    uint8_t sprite; /* 0 player palette set .. 4 kid */
    uint8_t species[3];
    uint8_t levels[3];
    uint8_t count;
    uint16_t reward;
    const char *const *post_lines; /* talk again after defeat */
    uint8_t flag;                  /* progress flag set on defeat */
} TrainerDef;

typedef struct {
    uint8_t x, y, dir, sprite, role;
    const char *const *lines; /* null-terminated array of const char* */
    const TrainerDef *trainer;
    uint8_t need_flags; /* blocker npc vanishes once these flags are set */
} NpcDef;

typedef struct {
    uint8_t x, y;
    const char *text;
} SignDef;

typedef struct {
    uint16_t species;
    uint8_t minlvl, maxlvl, weight;
} Encounter;

typedef struct {
    const char *name;
    uint8_t w, h;
    const char *const *rows; /* h strings of w chars */
    const Warp *warps;
    uint8_t nwarps;
    const NpcDef *npcs;
    uint8_t nnpcs;
    const SignDef *signs;
    uint8_t nsigns;
    const Encounter *encs;
    uint8_t nencs;
    uint8_t enc_rate; /* % chance per step in tall grass */
} MapDef;

/* ---- global game state (defined in main.c) ---- */
enum { MODE_TITLE, MODE_OVERWORLD, MODE_BATTLE };

/* ---- input (defined in main.c) ---- */
enum { BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT, BTN_A, BTN_B, BTN_START, BTN_COUNT };
typedef struct {
    bool held[BTN_COUNT];
    bool pressed[BTN_COUNT];
} Input;
extern Input g_in;

/* helpers defined in main.c (they touch global state) */
void bag_add(uint8_t item);
int bag_count(uint8_t item);
void bag_consume(uint8_t item);
void heal_party(void);
void dex_see(uint8_t species);
void dex_own(uint8_t species);
void start_fade(void (*cb)(void)); /* fade to black then run cb */

enum {
    FLAG_NONE = 0,
    FLAG_STARTER = 1 << 0,   /* chose a starter */
    FLAG_T1 = 1 << 1,        /* route trainer defeated */
    FLAG_T2 = 1 << 2,        /* route exit hiker */
    FLAG_T3 = 1 << 3,        /* route 2: scout denim */
    FLAG_T4 = 1 << 4,        /* route 2: hiker bram */
    FLAG_KEEPER = 1 << 5,    /* route 2: keeper iris beaten */
};

typedef struct {
    uint8_t mode;
    uint8_t map;         /* current map id */
    uint16_t px, py;     /* player pixel pos (top-left of 16x16 sprite) */
    uint8_t dir;         /* 0 down 1 up 2 left 3 right */
    uint8_t walk_frame;  /* anim frame while moving */
    uint8_t moving;
    uint16_t dx_steps;   /* distance remaining in current step (pixels) */
    int8_t dx, dy;       /* direction of current step */
    uint8_t hop;         /* ledge hop height */
    uint32_t tick;

    Creature party[MAX_PARTY];
    Creature storage[STORAGE_MAX];
    uint8_t storage_n;
    uint8_t party_n;
    uint8_t active_slot; /* battle: which party member is out */
    uint8_t bag[MAX_BAG];  /* item ids */
    uint8_t bag_n;
    uint16_t money;
    uint8_t flags;
    uint8_t dex_seen[NUM_SPECIES_TOTAL / 8 + 1];
    uint8_t dex_caught[NUM_SPECIES_TOTAL / 8 + 1];

    uint8_t heal_map, heal_x, heal_y;

    /* fade transition */
    uint8_t fade;       /* 0 = clear, 16 = black */
    int8_t fade_dir;    /* +1 darkening, -1 clearing, 0 idle */
    void (*fade_cb)(void);
} GameState;

extern GameState g;

/* ---- core.c (pure logic, no SDL) ---- */
void creature_init(Creature *c, uint8_t species, uint8_t level, uint8_t random_ivs);
uint16_t xp_for_level(uint8_t level);
void creature_add_xp(Creature *c, uint16_t amount, int *levels_gained);
void creature_recalc_stats(Creature *c, int keep_ratio);
int type_chart_lookup(uint8_t atk, uint8_t def); /* returns multiplier x10 */
uint16_t stat_after_stage(uint16_t v, int8_t stage);
typedef struct {
    int damage;
    uint8_t crit;
    int8_t effectiveness; /* 0 normal, >0 super (n x), -1 not very, -2 immune */
    uint8_t missed;
} DamageResult;
DamageResult move_damage(const Creature *att, const Creature *def, const int8_t att_st[6],
                         const int8_t def_st[6], uint8_t move_id);
int catch_shakes(const Creature *wild, int ball_x10);
bool learn_move(Creature *c, uint8_t move_id, int *replaced);
int pick_enemy_move(const Creature *enemy, const Creature *player);
const char *type_name(uint8_t t);
/* PC storage */
int storage_deposit(int party_slot);  /* 0 ok, -1 last partner, -2 box full */
int storage_withdraw(int box_idx);    /* 0 ok, -1 bad idx, -2 party full */

/* party cursor helpers: bounded, never loop when nothing is pickable */
int party_pick_cursor(const Creature *party, int n, int exclude_slot,
                      bool require_hp, int from, int dir);
bool party_any_pickable(const Creature *party, int n, int exclude_slot,
                        bool require_hp);

/* ---- tables.c ---- */
extern const Move MOVES[NUM_MOVES];
extern const Species SPECIES[NUM_SPECIES];
extern const Item ITEMS[NUM_ITEM_IDS];
extern const MapDef MAPS[NUM_MAPS];
extern const char *const SPECIES_ART[NUM_SPECIES];
extern const uint8_t SPECIES_TINT[NUM_SPECIES][3];

/* battle launch requests from overworld (consumed by main.c) */
typedef struct {
    uint8_t active;
    uint8_t species, level;
    const TrainerDef *trainer;
} BattleRequest;
extern BattleRequest g_battle_req;

#endif /* GAME_H */
