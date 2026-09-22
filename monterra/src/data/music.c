/* MONTERRA - original chip music + sfx data.
 * All compositions are original works made for this project. */
#include "../game.h"
#include "music.h"

/* note symbols (MIDI numbers) */
#define R  0   /* rest */
#define H  (-1) /* hold */
enum {
    E2 = 40, F2s = 42, G2 = 43, A2 = 45, B2 = 47,
    C3 = 48, D3 = 50, E3 = 52, F3 = 53, G3 = 55, A3 = 57, B3 = 59,
    C4 = 60, D4 = 62, E4 = 64, F4 = 65, G4 = 67, A4 = 69, B4 = 71,
    C5 = 72, D5 = 74, E5 = 76, F5 = 77, G5 = 79, A5 = 81, B5 = 83,
    C6 = 84, D6 = 86
};
#define Fs5 78 /* f-sharp 5 */

/* ---- MUS_TITLE: calm arpeggio ---- */
static const int8_t t_title_p1[] = {
    C4, R, E4, R, G4, R, C5, R, E5, R, C5, R, G4, R, E4, R,
    A3, R, C4, R, E4, R, A4, R, C5, R, A4, R, E4, R, C4, R,
    F3, R, A3, R, C4, R, F4, R, A4, R, F4, R, C4, R, A3, R,
    G3, R, B3, R, D4, R, G4, R, B4, R, G4, R, D4, R, B3, R,
};
static const int8_t t_title_p2[] = {
    E5, H, H, H, D5, H, H, H, C5, H, H, H, H, H, H, H,
    C5, H, H, H, B4, H, H, H, A4, H, H, H, H, H, H, H,
    A4, H, H, H, G4, H, H, H, F4, H, H, H, H, H, H, H,
    G4, H, H, H, F4, H, H, H, E4, H, H, H, H, H, H, H,
};
static const int8_t t_title_tri[] = {
    C3, H, H, H, H, H, H, H, G2, H, H, H, H, H, H, H,
    A2, H, H, H, H, H, H, H, E2, H, H, H, H, H, H, H,
    F2s, H, H, H, H, H, H, H, C3, H, H, H, H, H, H, H,
    G2, H, H, H, H, H, H, H, G2, H, H, H, H, H, H, H,
};
static const int8_t t_title_dr[] = { 0 };

/* ---- MUS_TOWN: cheerful walk ---- */
static const int8_t t_town_p1[] = {
    C5, H, E5, H, G5, H, E5, H, A5, H, G5, H, E5, H, D5, H,
    C5, H, E5, H, G5, H, C6, H, B5, H, A5, H, G5, H, H, H,
    F5, H, A5, H, C6, H, A5, H, G5, H, E5, H, C5, H, H, H,
    D5, H, F5, H, A5, H, F5, H, E5, H, D5, H, C5, H, H, H,
};
static const int8_t t_town_p2[] = { /* parallel sixths below */
    A4, H, C5, H, E5, H, C5, H, F5, H, E5, H, C5, H, B4, H,
    A4, H, C5, H, E5, H, A5, H, G5, H, F5, H, E5, H, H, H,
    D5, H, F5, H, A5, H, F5, H, E5, H, C5, H, A4, H, H, H,
    B4, H, D5, H, F5, H, D5, H, C5, H, B4, H, A4, H, H, H,
};
static const int8_t t_town_tri[] = {
    C3, H, H, H, G2, H, H, H, A2, H, H, H, E2, H, H, H,
    C3, H, H, H, G2, H, H, H, F2s, H, H, H, C3, H, H, H,
    F2s, H, H, H, C3, H, H, H, G2, H, H, H, C3, H, H, H,
    D3, H, H, H, A2, H, H, H, G2, H, H, H, C3, H, H, H,
};
static const int8_t t_town_dr[] = {
    1, 0, 0, 3, 0, 0, 1, 0, 3, 0, 0, 0, 1, 0, 3, 2,
    1, 0, 0, 3, 0, 0, 1, 0, 3, 0, 0, 0, 1, 0, 3, 2,
    1, 0, 0, 3, 0, 0, 1, 0, 3, 0, 0, 0, 1, 0, 3, 2,
    1, 0, 0, 3, 0, 0, 1, 0, 3, 0, 0, 0, 1, 0, 3, 2,
};

/* ---- MUS_ROUTE: upbeat travel ---- */
static const int8_t t_route_p1[] = {
    D5, H, D5, H, G5, H, A5, H, B5, H, A5, H, G5, H, D5, H,
    E5, H, E5, H, A5, H, B5, H, D6, H, B5, H, A5, H, H, H,
    G5, H, G5, H, B5, H, D6, H, B5, H, A5, H, G5, H, E5, H,
    D5, H, E5, H, Fs5, H, A5, H, G5, H, Fs5, H, D5, H, H, H,
};
static const int8_t t_route_p2[] = {
    B4, H, B4, H, D5, H, D5, H, G5, H, D5, H, B4, H, B4, H,
    C5, H, C5, H, E5, H, E5, H, F5, H, E5, H, C5, H, H, H,
    B4, H, B4, H, D5, H, F5, H, E5, H, D5, H, B4, H, C5, H,
    A4, H, B4, H, C5, H, E5, H, D5, H, C5, H, A4, H, H, H,
};
static const int8_t t_route_tri[] = {
    G2, H, D3, H, G2, H, D3, H, G2, H, D3, H, G2, H, D3, H,
    C3, H, G2, H, C3, H, G2, H, C3, H, G2, H, C3, H, G2, H,
    G2, H, D3, H, G2, H, D3, H, G2, H, D3, H, G2, H, D3, H,
    D3, H, A2, H, D3, H, A2, H, G2, H, D3, H, G2, H, D3, H,
};
static const int8_t t_route_dr[] = {
    1, 0, 3, 0, 1, 0, 3, 0, 1, 0, 3, 0, 1, 0, 3, 2,
    1, 0, 3, 0, 1, 0, 3, 0, 1, 0, 3, 0, 1, 0, 3, 2,
    1, 0, 3, 0, 1, 0, 3, 0, 1, 0, 3, 0, 1, 0, 3, 2,
    1, 0, 3, 0, 1, 0, 3, 0, 1, 0, 3, 0, 2, 0, 2, 2,
};

/* ---- MUS_BATTLE: driving minor ---- */
static const int8_t t_bat_p1[] = {
    A4, H, A4, H, C5, H, A4, H, E5, H, D5, H, C5, H, B4, H,
    A4, H, A4, H, C5, H, E5, H, G5, H, E5, H, D5, H, C5, H,
    F5, H, F5, H, A5, H, F5, H, E5, H, C5, H, A4, H, C5, H,
    E5, H, D5, H, B4, H, G4, H, A4, H, B4, H, C5, H, D5, H,
};
static const int8_t t_bat_p2[] = {
    E4, H, E4, H, E4, H, E4, H, A4, H, A4, H, A4, H, A4, H,
    E4, H, E4, H, E4, H, E4, H, C5, H, C5, H, B4, H, A4, H,
    F4, H, F4, H, F4, H, F4, H, C5, H, C5, H, C5, H, C5, H,
    B4, H, B4, H, G4, H, G4, H, A4, H, A4, H, A4, H, A4, H,
};
static const int8_t t_bat_tri[] = {
    A2, H, A2, H, A2, H, A2, H, F2s, H, F2s, H, F2s, H, F2s, H,
    A2, H, A2, H, A2, H, A2, H, C3, H, C3, H, E2, H, E2, H,
    F2s, H, F2s, H, F2s, H, F2s, H, A2, H, A2, H, A2, H, A2, H,
    G2, H, G2, H, G2, H, G2, H, A2, H, A2, H, B2, H, B2, H,
};
static const int8_t t_bat_dr[] = {
    1, 0, 3, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 3, 3, 2,
    1, 0, 3, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 3, 3, 2,
    1, 0, 3, 2, 1, 0, 3, 2, 1, 0, 3, 2, 1, 3, 3, 2,
    1, 0, 3, 2, 1, 0, 3, 2, 1, 3, 1, 3, 2, 2, 2, 2,
};

/* ---- MUS_VICTORY: short fanfare (used as jingle) ---- */
static const int8_t t_vic_p1[] = {
    G4, H, G4, H, C5, H, C5, H, E5, G5, C6, H, H, H, H, H,
    D6, H, B5, H, G5, H, E5, H, C5, H, H, H, H, H, H, H,
};
static const int8_t t_vic_p2[] = {
    E4, H, E4, H, E4, H, E4, H, G4, C5, E5, H, H, H, H, H,
    B4, H, G4, H, D5, H, B4, H, G4, H, H, H, H, H, H, H,
};
static const int8_t t_vic_tri[] = {
    C3, H, G2, H, C3, H, C3, H, C3, G2, C3, H, H, H, H, H,
    G2, H, G2, H, G2, H, G2, H, C3, H, H, H, H, H, H, H,
};
static const int8_t t_vic_dr[] = {
    1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 3, 2, 0, 0, 0, 0,
    1, 0, 1, 0, 1, 0, 3, 2, 1, 0, 0, 0, 0, 0, 0, 0,
};

const MusicTrack MUSIC[MUS_COUNT] = {
    { { NULL, NULL, NULL, NULL }, { 0, 0, 0, 0 }, 120 }, /* MUS_NONE */
    { { t_title_p1, t_title_p2, t_title_tri, t_title_dr },
      { 64, 64, 64, 1 }, 88 },
    { { t_town_p1, t_town_p2, t_town_tri, t_town_dr },
      { 64, 64, 64, 64 }, 112 },
    { { t_route_p1, t_route_p2, t_route_tri, t_route_dr },
      { 64, 64, 64, 64 }, 140 },
    { { t_bat_p1, t_bat_p2, t_bat_tri, t_bat_dr },
      { 64, 64, 64, 64 }, 160 },
    { { t_vic_p1, t_vic_p2, t_vic_tri, t_vic_dr },
      { 32, 32, 32, 32 }, 132 },
};

const SfxProgram SFX[SFX_COUNT] = {
    { 0, 0, 0, 0, 0 },                        /* SFX_NONE */
    { 0, 880, 1320, 45, 6 },                  /* SFX_BLIP: menu tick */
    { 0, 660, 1760, 90, 8 },                  /* SFX_CONFIRM */
    { 0, 110, 70, 90, 8 },                    /* SFX_BUMP: low thud */
    { 3, 900, 300, 120, 10 },                 /* SFX_HIT: noise crunch */
    { 2, 600, 80, 350, 9 },                   /* SFX_FAINT: falling sweep */
    { 0, 523, 1046, 300, 8 },                 /* SFX_HEAL: rising chime */
    { 0, 300, 1200, 250, 8 },                 /* SFX_THROW: toss */
    { 0, 1200, 1600, 160, 12 },               /* SFX_SPOT: alert */
};

int music_for_map(uint8_t map)
{
    switch (map) {
    case MAP_ROUTE1:
    case MAP_ROUTE2:
        return MUS_ROUTE;
    case MAP_HOUSE:
    case MAP_LAB:
    case MAP_HEAL:
    case MAP_TOWN:
    case MAP_MART:
    default:
        return MUS_TOWN;
    }
}
