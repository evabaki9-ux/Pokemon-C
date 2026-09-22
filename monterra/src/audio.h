#pragma once
/* MONTERRA - 4-channel chip synth + sequencer (SDL2 audio) */
#include <stdbool.h>

void audio_init(void);
void audio_quit(void);

/* background track, loops; restarts only when the track actually changes */
void audio_play_music(int id);

/* one-shot jingle played over the bg track; bg resumes afterwards */
void audio_play_jingle(int id);

void audio_sfx(int id);
void audio_toggle_mute(void);
bool audio_muted(void);
