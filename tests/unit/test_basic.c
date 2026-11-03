#include "art.h"
#include "fb.h"
#include "font.h"
#include "map.h"
#include "palette.h"
#include "rng.h"
#include "test.h"
#include <stdlib.h>

static void defs(void)
{
    CHECK_EQ(dir_from_delta(0, -1), DIR_N);
    CHECK_EQ(dir_from_delta(5, 5), DIR_SE);
    CHECK_EQ(dir_from_delta(-2, 0), DIR_W);
    CHECK_EQ(dir_from_delta(0, 0), -1);
    CHECK_EQ(clampi(5, 0, 3), 3);
    CHECK_EQ(clampi(-5, 0, 3), 0);
    CHECK_EQ(clampi(2, 0, 3), 2);
    CHECK_EQ(imax(2, 3), 3);
    CHECK_EQ(imin(2, 3), 2);
    CHECK_EQ(iabs(-4), 4);
}

static void rng(void)
{
    Rng a, b, z;
    rng_seed(&a, 42);
    rng_seed(&b, 42);
    rng_seed(&z, 0);
    CHECK(z.s != 0);
    for (int i = 0; i < 100; i++) CHECK_EQ(rng_next(&a), rng_next(&b));
    int lo = 100, hi = -1;
    for (int i = 0; i < 1000; i++) {
        int v = rng_range(&a, 3, 7);
        lo = imin(lo, v);
        hi = imax(hi, v);
    }
    CHECK_EQ(lo, 3);
    CHECK_EQ(hi, 7);
    CHECK_EQ(rng_range(&a, 5, 5), 5);
    CHECK_EQ(rng_range(&a, 5, 1), 5);
    CHECK_EQ(rng_chance(&a, 0), 0);
    CHECK_EQ(rng_chance(&a, 100), 1);
}

static void palette(void)
{
    CHECK_EQ(PALETTE_RGB[C_BLACK], 0);
    CHECK_EQ(PALETTE_RGB[C_WHITE], 0xFFFFFF);
    CHECK_EQ(palette_darken(C_WHITE, 0), C_WHITE);
    CHECK_EQ(palette_darken(C_WHITE, 1), C_LGREY);
    CHECK_EQ(palette_darken(C_WHITE, 20), C_BLACK);
    for (int c = 0; c < C_COUNT; c++) CHECK(PALETTE_DARKER[c] < C_COUNT);
}

static void map(void)
{
    Map m;
    static const char *const bad_glyph[] = {"..", ".Z"};
    static const char *const ragged[] = {"...", ".."};
    static const char *const empty[] = {""};
    CHECK(!map_load(&m, bad_glyph, 2));
    CHECK(!map_load(&m, ragged, 2));
    CHECK(!map_load(&m, empty, 1));
    CHECK(!map_load(&m, bad_glyph, 0));
    CHECK(!map_load(&m, bad_glyph, MAP_MAX + 1));
    CHECK(map_load(&m, TEST_SCENARIO.rows, 16));
    CHECK_EQ(m.w, 16);
    CHECK_EQ(map_get(&m, 2, 2), T_WALL);
    CHECK_EQ(map_get(&m, 2 + 16, 2 - 16), T_WALL);
    CHECK_EQ(map_wx(&m, -1), 15);
    CHECK_EQ(map_wy(&m, 16), 0);
    CHECK(map_has(&m, 2, 2, TF_OPAQUE));
    CHECK(!map_has(&m, 0, 0, TF_OPAQUE));
    map_set(&m, -1, -1, T_TREE);
    CHECK_EQ(map_get(&m, 15, 15), T_TREE);
    CHECK_EQ(wrap_delta(1, 15, 16), -2);
    CHECK_EQ(wrap_delta(15, 1, 16), 2);
    CHECK_EQ(map_dist(&m, 0, 0, 15, 15), 1);
    CHECK_EQ(terrain_from_glyph('#'), T_WALL);
    CHECK_EQ(terrain_from_glyph('Z'), T_COUNT);
    for (int t = 0; t < T_COUNT; t++) CHECK(TERRAIN[t].name != NULL);
}

static void font_and_art(void)
{
    const uint8_t *a = font_glyph('A'), *la = font_glyph('a'), *unk = font_glyph('@');
    CHECK(a == la);
    CHECK_EQ(a[0], 0x0E);
    CHECK_EQ(unk[0], 0x1F);
    CHECK_EQ(font_glyph(' ')[3], 0);
    for (int c = 0; c < CR_COUNT; c++) {
        const char *const *s = art_creature(c);
        for (int j = 0; j < SPR; j++) CHECK_EQ(strlen(s[j]), SPR);
    }
    for (int t = 0; t < T_COUNT; t++) {
        const char *const *s = art_terrain(t);
        for (int j = 0; j < SPR; j++) CHECK_EQ(strlen(s[j]), SPR);
    }
    for (int o = 0; o < OB_COUNT; o++) {
        const char *const *s = art_object(o);
        for (int j = 0; j < SPR; j++) CHECK_EQ(strlen(s[j]), SPR);
    }
    for (int i = 0; i < IC_COUNT; i++) {
        const char *const *s = art_icon(i);
        for (int j = 0; j < ICON; j++) CHECK_EQ(strlen(s[j]), ICON);
    }
    CHECK_EQ(art_pixel('.', 3), -1);
    CHECK_EQ(art_pixel('7', 3), 7);
    CHECK_EQ(art_pixel('e', 3), 14);
    CHECK_EQ(art_pixel('X', 3), 3);
    CHECK_EQ(art_pixel('x', C_WHITE), C_LGREY);
}

static void framebuffer(void)
{
    static Framebuffer fb;
    fb_clear(&fb, C_BLUE);
    CHECK_EQ(fb.px[100][100], C_BLUE);
    fb_pixel(&fb, -1, 5, C_RED); /* clipped, must not crash */
    fb_pixel(&fb, SCREEN_W, 5, C_RED);
    fb_pixel(&fb, 5, SCREEN_H, C_RED);
    fb_pixel(&fb, 5, 5, C_RED);
    CHECK_EQ(fb.px[5][5], C_RED);
    fb_rect(&fb, 10, 10, 3, 2, C_WHITE);
    CHECK_EQ(fb.px[11][12], C_WHITE);
    CHECK_EQ(fb.px[12][12], C_BLUE);
    fb_frame(&fb, 20, 20, 5, 5, C_YELLOW);
    CHECK_EQ(fb.px[20][24], C_YELLOW);
    CHECK_EQ(fb.px[22][22], C_BLUE);
    Rect clip = {0, 0, 40, 40};
    fb_sprite(&fb, art_creature(CR_WIZARD), SPR, 30, 30, 1, C_CYAN, 0, &clip);
    CHECK_EQ(fb.px[30][37], C_CYAN); /* hat tip is owner colour */
    CHECK_EQ(fb.px[45][45], C_BLUE); /* clipped */
    fb_sprite(&fb, art_creature(CR_WIZARD), SPR, 100, 100, 2, C_WHITE, 1, NULL);
    CHECK_EQ(fb.px[100][114], C_LGREY);
    CHECK_EQ(fb_text_width("ABC"), 18);
    fb_clear(&fb, C_BLACK);
    CHECK_EQ(fb_text(&fb, 0, 0, "A", C_WHITE), 6);
    CHECK_EQ(fb.px[0][1], C_WHITE);
    CHECK_EQ(fb.px[0][0], C_BLACK);
    fb_text_centre(&fb, 160, 50, "HI", C_GREEN);
    CHECK_EQ(fb.px[50][154], C_GREEN);
    Rect r = {1, 2, 3, 4};
    CHECK(rect_has(&r, 1, 2));
    CHECK(!rect_has(&r, 4, 2));
    CHECK(fb_write_ppm(&fb, "test_out.ppm"));
    CHECK(!fb_write_ppm(&fb, "/nonexistent-dir/x.ppm"));
    remove("test_out.ppm");
}

void test_basic(void)
{
    defs();
    rng();
    palette();
    map();
    font_and_art();
    framebuffer();
}
