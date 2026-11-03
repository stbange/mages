#include "audio.h"
#include <math.h>
#include <string.h>

/*                         wave     f0     f1    dur   att    dec   sus   rel   vol  arp            rate */
const SfxDef SFX_DEFS[SFX_COUNT] = {
    [SFX_CURSOR] = {W_PULSE, 1200, 1200, 0.03f, 0.001f, 0.02f, 0.0f, 0.01f, 0.15f, {0}, 0},
    [SFX_SELECT] = {W_PULSE, 660, 990, 0.08f, 0.002f, 0.04f, 0.4f, 0.03f, 0.30f, {0}, 0},
    [SFX_STEP] = {W_NOISE, 900, 300, 0.06f, 0.001f, 0.05f, 0.0f, 0.01f, 0.25f, {0}, 0},
    [SFX_HIT] = {W_NOISE, 2500, 200, 0.25f, 0.001f, 0.20f, 0.0f, 0.04f, 0.60f, {0}, 0},
    [SFX_MISS] = {W_TRI, 500, 150, 0.15f, 0.002f, 0.12f, 0.0f, 0.03f, 0.35f, {0}, 0},
    [SFX_DEATH] = {W_SAW, 400, 40, 0.70f, 0.005f, 0.60f, 0.0f, 0.10f, 0.50f, {0}, 0},
    [SFX_CAST] = {W_PULSE, 300, 1800, 0.45f, 0.01f, 0.30f, 0.3f, 0.12f, 0.40f, {0, 4, 7, 12}, 0.03f},
    [SFX_SUMMON] = {W_TRI, 200, 800, 0.60f, 0.05f, 0.40f, 0.5f, 0.15f, 0.55f, {0, 7, 12, 7}, 0.05f},
    [SFX_DOOR] = {W_SAW, 120, 90, 0.25f, 0.01f, 0.20f, 0.0f, 0.04f, 0.35f, {0}, 0},
    [SFX_PORTAL] = {W_TRI, 220, 880, 1.50f, 0.20f, 1.00f, 0.6f, 0.30f, 0.60f, {0, 3, 7, 10}, 0.08f},
    [SFX_TURN] = {W_PULSE, 523, 523, 0.40f, 0.005f, 0.30f, 0.4f, 0.08f, 0.35f, {0, 4, 7, 12}, 0.08f},
    [SFX_ERROR] = {W_PULSE, 160, 140, 0.18f, 0.002f, 0.15f, 0.5f, 0.03f, 0.35f, {0}, 0},
    [SFX_PICKUP] = {W_PULSE, 880, 1760, 0.12f, 0.002f, 0.08f, 0.3f, 0.03f, 0.30f, {0, 12}, 0.04f},
    [SFX_FLY] = {W_NOISE, 300, 1200, 0.35f, 0.08f, 0.25f, 0.2f, 0.08f, 0.30f, {0}, 0},
    [SFX_WIN] = {W_PULSE, 392, 392, 1.60f, 0.01f, 1.20f, 0.6f, 0.30f, 0.50f, {0, 4, 7, 12}, 0.12f},
};

void audio_init(Mixer *m)
{
    memset(m, 0, sizeof *m);
    m->master = 0.8f;
}

void audio_play(Mixer *m, int sfx)
{
    if (sfx < 0 || sfx >= SFX_COUNT) return;
    int slot = -1;
    for (int i = 0; i < AUDIO_VOICES; i++) {
        if (!m->v[i].active) { slot = i; break; }
    }
    if (slot < 0) slot = (int)(m->next++ % AUDIO_VOICES);
    Voice *v = &m->v[slot];
    memset(v, 0, sizeof *v);
    v->active = 1;
    v->def = &SFX_DEFS[sfx];
    v->len = (uint32_t)(v->def->dur * AUDIO_RATE);
    v->noise = 0x12345u + (uint32_t)sfx * 7919u;
}

int audio_active_voices(const Mixer *m)
{
    int n = 0;
    for (int i = 0; i < AUDIO_VOICES; i++) n += m->v[i].active;
    return n;
}

float audio_envelope(const SfxDef *d, uint32_t t, uint32_t len)
{
    float s = (float)t / AUDIO_RATE;
    float rel_start = (float)len / AUDIO_RATE - d->release;
    float lvl;
    if (s < d->attack) lvl = s / d->attack;
    else if (s < d->attack + d->decay) lvl = 1.0f - (1.0f - d->sustain) * (s - d->attack) / d->decay;
    else lvl = d->sustain;
    if (s >= rel_start) {
        float k = 1.0f - (s - rel_start) / d->release;
        lvl *= k < 0 ? 0 : k;
    }
    return lvl;
}

static float voice_sample(Voice *v)
{
    const SfxDef *d = v->def;
    float p = (float)v->t / (float)v->len;
    float f = d->f0 * powf(d->f1 / d->f0, p);
    if (d->arp_rate > 0) {
        int step = (int)((float)v->t / (d->arp_rate * AUDIO_RATE)) & 3;
        f *= powf(2.0f, d->arp[step] / 12.0f);
    }
    float prev = v->phase;
    v->phase += f / AUDIO_RATE;
    v->phase -= floorf(v->phase);
    float s;
    switch (d->wave) {
    case W_PULSE: s = v->phase < 0.5f ? 1.0f : -1.0f; break;
    case W_TRI: s = v->phase < 0.5f ? 4.0f * v->phase - 1.0f : 3.0f - 4.0f * v->phase; break;
    case W_SAW: s = 2.0f * v->phase - 1.0f; break;
    default:
        if (v->phase < prev) { /* new period: next noise value (LFSR) */
            v->noise ^= v->noise << 13;
            v->noise ^= v->noise >> 17;
            v->noise ^= v->noise << 5;
            v->noise_val = (float)(v->noise & 0xFFFF) / 32768.0f - 1.0f;
        }
        s = v->noise_val;
        break;
    }
    return s * d->vol * audio_envelope(d, v->t, v->len);
}

void audio_render(Mixer *m, int16_t *out, int frames)
{
    for (int i = 0; i < frames; i++) {
        float acc = 0;
        for (int k = 0; k < AUDIO_VOICES; k++) {
            Voice *v = &m->v[k];
            if (!v->active) continue;
            acc += voice_sample(v);
            if (++v->t >= v->len) v->active = 0;
        }
        acc *= m->master * 32767.0f * 0.5f;
        if (acc > 32767.0f) acc = 32767.0f;
        if (acc < -32768.0f) acc = -32768.0f;
        out[i] = (int16_t)acc;
    }
}
