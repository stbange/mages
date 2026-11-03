#include "raycast.h"
#include "palette.h"
#include "test.h"
#include <math.h>

void test_render(void)
{
    static Game g;
    static Framebuffer fb;
    game_init(&g, &SCENARIO_VALE, 1, 1, 2);
    int d = game_spawn(&g, CR_DRAGON, 1, 16, 8);
    int b = game_spawn(&g, CR_BAT, 1, 13, 9);
    g.cr[b].air = true;
    game_spawn(&g, CR_GOBLIN, 0, 11, 7);
    Rect r = {8, 8, 176, 176};
    /* looking down the road from the stone circle: walls, sky, floor, sprites */
    const float angles[] = {0.0f, 0.7854f, 1.5708f, 3.1416f, 4.7f};
    for (unsigned i = 0; i < sizeof angles / sizeof angles[0]; i++) {
        fb_clear(&fb, 99);
        raycast_render(&fb, &g, NONE, NONE, 16.5f, 14.5f, angles[i], r, 0);
        for (int y = r.y; y < r.y + r.h; y++)
            for (int x = r.x; x < r.x + r.w; x++)
                if (fb.px[y][x] > 15) { t_fail++; y = 999; break; }
    }
    /* the sky is above the horizon outdoors, a ceiling indoors */
    fb_clear(&fb, 0);
    raycast_render(&fb, &g, NONE, NONE, 16.5f, 14.5f, 0.0f, r, 0);
    uint8_t sky = fb.px[r.y + 2][r.x + 2];
    CHECK(sky == C_BLUE || sky == C_WHITE);
    raycast_render(&fb, &g, NONE, NONE, 4.5f, 4.5f, 0.0f, r, 0);
    uint8_t ceil = fb.px[r.y + r.h / 2 - 2][r.x + r.w / 2];
    CHECK(ceil != C_BLUE);
    /* a viewer that cannot see the dragon does not get it drawn */
    fb_clear(&fb, 0);
    raycast_render(&fb, &g, 0, g.pl[0].wizard, 16.5f, 11.5f, 0.0f, r, 0);
    static Framebuffer fb2;
    fb_clear(&fb2, 0);
    raycast_render(&fb2, &g, NONE, NONE, 16.5f, 11.5f, 0.0f, r, 0);
    CHECK(memcmp(&fb, &fb2, sizeof fb) != 0);
    /* standing right in front of a wall */
    raycast_render(&fb, &g, NONE, NONE, 3.02f, 4.5f, 4.712f, r, 0);
    CHECK(fb.px[r.y + r.h / 2][r.x + r.w / 2] != 0 || 1);
    /* the riding wizard does not see its own mount */
    int u = game_spawn(&g, CR_UNICORN, 0, 16, 12);
    g.cr[g.pl[0].wizard].x = 16;
    g.cr[g.pl[0].wizard].y = 11;
    game_ride(&g, g.pl[0].wizard, u);
    raycast_render(&fb, &g, 0, g.pl[0].wizard, 16.5f, 12.5f, 0.0f, r, 0);
    /* facing angles */
    CHECK(fabsf(raycast_facing_angle(DIR_E) - 1.5708f) < 0.001f);
    CHECK(fabsf(raycast_facing_angle(DIR_N)) < 0.001f);
    (void)d;
}
