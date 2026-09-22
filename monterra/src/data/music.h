#pragma once
/* MONTERRA - music & sfx data (pure data, no engine deps) */
#include <stdint.h>

/* music track ids */
enum { MUS_NONE, MUS_TITLE, MUS_TOWN, MUS_ROUTE, MUS_BATTLE, MUS_VICTORY, MUS_COUNT };

/* sfx ids */
enum {
    SFX_NONE, SFX_BLIP, SFX_CONFIRM, SFX_BUMP, SFX_HIT, SFX_FAINT,
    SFX_HEAL, SFX_THROW, SFX_SPOT, SFX_COUNT
};

#define MUS_CHAN   4 /* pulse50, pulse25, triangle, drums */
#define MUS_MAXLEN 96

/* one track: per-channel step sequences.
 * melodic channels: 0 = rest, -1 = hold previous, else MIDI note number.
 * drum channel (3): 0 = rest, 1 = kick, 2 = snare, 3 = hat.
 * one step = a 16th note at the track's bpm. */
typedef struct {
    const int8_t *mel[MUS_CHAN];
    uint8_t len[MUS_CHAN];
    uint8_t bpm;
} MusicTrack;

/* one-shot sound effect: a single oscillator sweeping f0 -> f1 */
typedef struct {
    uint8_t wave;    /* 0 pulse50, 1 pulse25, 2 triangle, 3 noise */
    uint16_t f0, f1; /* start / end frequency in Hz */
    uint16_t ms;     /* duration */
    uint8_t vol;     /* 1..15 */
} SfxProgram;

extern const MusicTrack MUSIC[MUS_COUNT];
extern const SfxProgram SFX[SFX_COUNT];

/* which track plays on a given map */
int music_for_map(uint8_t map);
