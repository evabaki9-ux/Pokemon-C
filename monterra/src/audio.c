/* MONTERRA - audio engine: pulse/triangle/noise synth with a step
 * sequencer driving the music data in src/data/music.c.
 * All sound is synthesized at runtime (no sample files). */
#include <SDL.h>
#include <math.h>
#include <string.h>
#include "audio.h"
#include "data/music.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define RATE 44100

static struct {
    SDL_AudioDeviceID dev;
    bool muted;
    /* background + jingle */
    int bg;           /* bg track id */
    int cur;          /* currently sounding track (bg or jingle) */
    bool jingle;      /* cur is a jingle; fall back to bg at its end */
    /* sequencer */
    double sps;       /* samples per 16th step */
    double acc;
    uint8_t step[MUS_CHAN];
    /* voices */
    double phase[MUS_CHAN]; /* osc phase 0..1 (chan 3 = drum osc) */
    double freq[MUS_CHAN];
    double amp[MUS_CHAN];
    double target[MUS_CHAN];
    uint32_t lfsr;    /* shared noise source */
    /* drum voice */
    int drum;         /* 0 none, 1 kick, 2 snare, 3 hat */
    double drum_t;    /* seconds since drum start */
    /* sfx voice */
    struct {
        int active;
        int wave;
        double f0, f1, dur, t;
        double vol;
    } sfx;
} A;

static double note_hz(int midi)
{
    return 440.0 * pow(2.0, (midi - 69) / 12.0);
}

static void seq_load(int id)
{
    if (id <= MUS_NONE || id >= MUS_COUNT)
        id = MUS_NONE;
    A.cur = id;
    memset(A.step, 0, sizeof(A.step));
    memset(A.phase, 0, sizeof(A.phase));
    memset(A.freq, 0, sizeof(A.freq));
    memset(A.amp, 0, sizeof(A.amp));
    memset(A.target, 0, sizeof(A.target));
    A.acc = 0;
    A.drum = 0;
    A.sps = (double)RATE * 60.0 / (MUSIC[id].bpm * 4.0);
}

static void step_channel(int ch, int8_t v)
{
    if (ch == 3) { /* drums */
        if (v > 0 && v <= 3) {
            A.drum = v;
            A.drum_t = 0;
        }
        return;
    }
    if (v == 0) {         /* rest */
        A.target[ch] = 0;
    } else if (v > 0) {   /* new note */
        A.freq[ch] = note_hz(v);
        A.target[ch] = (ch == 1) ? 0.55 : (ch == 2 ? 0.80 : 0.85);
    } /* v < 0: hold */
}

static double wave(int which, double phase)
{
    switch (which) {
    case 1: return phase < 0.25 ? 1.0 : -1.0;   /* pulse 25% */
    case 2: {                                    /* triangle */
        double t = phase < 0.5 ? phase * 2.0 : 2.0 - phase * 2.0;
        return t * 2.0 - 1.0;
    }
    default: return phase < 0.5 ? 1.0 : -1.0;   /* pulse 50% */
    }
}

static double noise(void)
{
    /* 15-bit LFSR */
    uint32_t b = ((A.lfsr & 1) ^ ((A.lfsr >> 1) & 1));
    A.lfsr = (A.lfsr >> 1) | (b << 14);
    return (A.lfsr & 2) ? 0.7 : -0.7;
}

static double drum_sample(void)
{
    if (!A.drum)
        return 0;
    double t = A.drum_t;
    double s = 0;
    if (A.drum == 1) { /* kick: 110->45 Hz sweep */
        double f = 110.0 - 65.0 * (t / 0.1);
        A.phase[3] += f / RATE;
        if (A.phase[3] >= 1.0) A.phase[3] -= 1.0;
        double tri = A.phase[3] < 0.5 ? A.phase[3] * 4.0 - 1.0
                                      : 3.0 - A.phase[3] * 4.0;
        s = tri * (1.0 - t / 0.1) * 0.9;
        if (t > 0.1) A.drum = 0;
    } else if (A.drum == 2) { /* snare */
        s = noise() * (1.0 - t / 0.09) * 0.8;
        if (t > 0.09) A.drum = 0;
    } else { /* hat */
        s = noise() * (1.0 - t / 0.03) * 0.35;
        if (t > 0.03) A.drum = 0;
    }
    return s;
}

static void fill(void *udata, Uint8 *stream, int len)
{
    (void)udata;
    int16_t *out = (int16_t *)stream;
    int n = len / 2;
    static const int ch_wave[MUS_CHAN] = { 0, 1, 2, 3 };

    for (int i = 0; i < n; i++) {
        /* sequencer: advance one sample */
        if (A.cur != MUS_NONE) {
            A.acc += 1.0;
            while (A.acc >= A.sps) {
                A.acc -= A.sps;
                const MusicTrack *t = &MUSIC[A.cur];
                for (int ch = 0; ch < MUS_CHAN; ch++) {
                    if (t->len[ch] == 0)
                        continue;
                    int8_t v = t->mel[ch][A.step[ch]];
                    step_channel(ch, v);
                    A.step[ch] = (uint8_t)((A.step[ch] + 1) % t->len[ch]);
                }
                /* jingle ended -> back to bg */
                if (A.jingle) {
                    int done = 1;
                    for (int ch = 0; ch < 3; ch++)
                        if (A.step[ch] != 0)
                            done = 0;
                    if (done) {
                        A.jingle = false;
                        seq_load(A.bg);
                    }
                }
            }
            A.drum_t += 1.0 / RATE;
        }

        /* music mix */
        double s = 0;
        for (int ch = 0; ch < 3; ch++) {
            if (A.freq[ch] > 0) {
                A.phase[ch] += A.freq[ch] / RATE;
                if (A.phase[ch] >= 1.0) A.phase[ch] -= 1.0;
            }
            /* one-pole amp smoothing avoids clicks */
            A.amp[ch] += (A.target[ch] - A.amp[ch]) * 0.004;
            s += wave(ch_wave[ch], A.phase[ch]) * A.amp[ch];
        }
        s += drum_sample();
        s *= 0.22;

        /* sfx mix */
        if (A.sfx.active) {
            double f = A.sfx.f0 + (A.sfx.f1 - A.sfx.f0) *
                        (A.sfx.t / A.sfx.dur);
            double sv;
            if (A.sfx.wave == 3) {
                sv = noise();
            } else {
                A.phase[3] += f / RATE;
                if (A.phase[3] >= 1.0) A.phase[3] -= 1.0;
                sv = wave(A.sfx.wave, A.phase[3]);
            }
            s += sv * A.sfx.vol * 0.30;
            A.sfx.t += 1.0 / RATE;
            if (A.sfx.t >= A.sfx.dur)
                A.sfx.active = 0;
        }

        if (A.muted)
            out[i] = 0;
        else {
            double c = s > 1.0 ? 1.0 : (s < -1.0 ? -1.0 : s);
            out[i] = (int16_t)(c * 32000.0);
        }
    }
}

void audio_init(void)
{
    memset(&A, 0, sizeof(A));
    A.lfsr = 0x4001;
    A.bg = MUS_NONE;
    A.cur = MUS_NONE;
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = RATE;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 1024;
    want.callback = fill;
    A.dev = SDL_OpenAudioDevice(NULL, 0, &want, &have,
                                SDL_AUDIO_ALLOW_FREQUENCY_CHANGE |
                                SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
    if (A.dev == 0) {
#ifdef __EMSCRIPTEN__
        EM_ASM({ console.warn("MONTERRA: audio device unavailable - running silent"); });
#endif
        return; /* run silently */
    }
#ifdef __EMSCRIPTEN__
    EM_ASM({ console.info("MONTERRA: audio ready - sound starts on first key press"); });
#endif
    SDL_PauseAudioDevice(A.dev, 0);
}

void audio_quit(void)
{
    if (A.dev) {
        SDL_CloseAudioDevice(A.dev);
        A.dev = 0;
    }
}

void audio_play_music(int id)
{
    if (A.dev == 0 || id == A.bg)
        return; /* no device, or already the background track */
    SDL_LockAudioDevice(A.dev);
    A.bg = id;
    if (!A.jingle)
        seq_load(id); /* a jingle keeps playing; new bg resumes after */
    SDL_UnlockAudioDevice(A.dev);
}

void audio_play_jingle(int id)
{
    if (A.dev == 0)
        return;
    SDL_LockAudioDevice(A.dev);
    A.jingle = true;
    seq_load(id);
    SDL_UnlockAudioDevice(A.dev);
}

void audio_sfx(int id)
{
    if (A.dev == 0 || id <= SFX_NONE || id >= SFX_COUNT)
        return;
    SDL_LockAudioDevice(A.dev);
    A.sfx.active = 1;
    A.sfx.wave = SFX[id].wave;
    A.sfx.f0 = SFX[id].f0;
    A.sfx.f1 = SFX[id].f1;
    A.sfx.dur = SFX[id].ms / 1000.0;
    A.sfx.t = 0;
    A.sfx.vol = SFX[id].vol / 15.0;
    SDL_UnlockAudioDevice(A.dev);
}

void audio_toggle_mute(void)
{
    A.muted = !A.muted;
}

bool audio_muted(void)
{
    return A.muted;
}
