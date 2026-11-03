#include "audio.h"
#include "test.h"

void test_audio(void)
{
    static Mixer m;
    static int16_t buf[AUDIO_RATE];
    audio_init(&m);
    CHECK_EQ(audio_active_voices(&m), 0);
    audio_play(&m, -1);
    audio_play(&m, SFX_COUNT);
    CHECK_EQ(audio_active_voices(&m), 0);
    /* silence when idle */
    audio_render(&m, buf, 100);
    CHECK_EQ(buf[50], 0);
    /* every effect produces sound and ends */
    for (int s = 0; s < SFX_COUNT; s++) {
        audio_init(&m);
        audio_play(&m, s);
        CHECK_EQ(audio_active_voices(&m), 1);
        int peak = 0;
        int len = (int)(SFX_DEFS[s].dur * AUDIO_RATE) + 10;
        for (int done = 0; done < len; done += 1000) {
            int n = len - done < 1000 ? len - done : 1000;
            audio_render(&m, buf, n);
            for (int i = 0; i < n; i++) peak = buf[i] > peak ? buf[i] : (-buf[i] > peak ? -buf[i] : peak);
        }
        CHECK(peak > 100);
        CHECK_EQ(audio_active_voices(&m), 0);
    }
    /* many voices: stealing, and no clipping overflow */
    audio_init(&m);
    m.master = 50.0f;
    for (int i = 0; i < AUDIO_VOICES + 3; i++) audio_play(&m, SFX_HIT);
    CHECK_EQ(audio_active_voices(&m), AUDIO_VOICES);
    audio_render(&m, buf, 2000);
    int extremes = 0;
    for (int i = 0; i < 2000; i++) extremes += buf[i] == 32767 || buf[i] == -32768;
    CHECK(extremes > 0);
    /* envelope shape */
    const SfxDef *d = &SFX_DEFS[SFX_PORTAL];
    uint32_t len = (uint32_t)(d->dur * AUDIO_RATE);
    CHECK(audio_envelope(d, 0, len) < 0.01f);
    CHECK(audio_envelope(d, (uint32_t)(d->attack * AUDIO_RATE), len) > 0.95f);
    CHECK(audio_envelope(d, len - (uint32_t)(d->release * AUDIO_RATE) - 10, len) > 0.5f);
    CHECK(audio_envelope(d, len, len) < 0.01f);
    CHECK(audio_envelope(d, len + AUDIO_RATE, len) == 0.0f);
}
