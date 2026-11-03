/* Small SID-inspired synthesiser for retro sound effects. */
#ifndef MAGES_AUDIO_H
#define MAGES_AUDIO_H
#include <stdint.h>

#define AUDIO_RATE 22050
#define AUDIO_VOICES 8

typedef enum {
    SFX_CURSOR, SFX_SELECT, SFX_STEP, SFX_HIT, SFX_MISS, SFX_DEATH, SFX_CAST,
    SFX_SUMMON, SFX_DOOR, SFX_PORTAL, SFX_TURN, SFX_ERROR, SFX_PICKUP, SFX_FLY,
    SFX_WIN, SFX_COUNT
} Sfx;

typedef enum { W_PULSE, W_TRI, W_SAW, W_NOISE } Wave;

typedef struct {
    uint8_t wave;
    float f0, f1;     /* start / end frequency in Hz (exponential slide) */
    float dur;        /* seconds, including release */
    float attack, decay, sustain, release; /* seconds, seconds, level, seconds */
    float vol;
    int8_t arp[4];    /* semitone steps cycled every arp_rate seconds (0 = none) */
    float arp_rate;
} SfxDef;

extern const SfxDef SFX_DEFS[SFX_COUNT];

typedef struct {
    int active;
    const SfxDef *def;
    uint32_t t, len;
    float phase;
    uint32_t noise;
    float noise_val;
} Voice;

typedef struct {
    Voice v[AUDIO_VOICES];
    float master;
    uint32_t next; /* round-robin for voice stealing */
} Mixer;

void audio_init(Mixer *m);
void audio_play(Mixer *m, int sfx);
void audio_render(Mixer *m, int16_t *out, int frames);
int audio_active_voices(const Mixer *m);
/* Envelope level (0..1) of def at sample t; exposed for tests. */
float audio_envelope(const SfxDef *d, uint32_t t, uint32_t len);

#endif
