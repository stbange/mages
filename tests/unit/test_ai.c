#include "ai.h"
#include "test.h"

static Game g;

static void neutral(void)
{
    setup(&g, 2, 0);
    int z = spawn(&g, CR_ZOMBIE, NEUTRAL, 10, 1);
    int gob = spawn(&g, CR_GOBLIN, 0, 11, 1);
    g.cr[gob].con = 99;
    g.cur = NEUTRAL;
    g.cr[z].ap = 8;
    ai_neutral_turn(&g);
    CHECK(g.cr[z].ap < 8); /* it attacked */
    /* approaches visible creatures within six squares */
    setup(&g, 2, 0);
    int s = spawn(&g, CR_SPIDER, NEUTRAL, 10, 13);
    spawn(&g, CR_GOBLIN, 0, 14, 13);
    g.cur = NEUTRAL;
    ai_neutral_turn(&g);
    CHECK(g.cr[s].x > 10);
    /* stays put when nothing is near */
    setup(&g, 2, 0);
    s = spawn(&g, CR_SPIDER, NEUTRAL, 5, 13);
    g.cur = NEUTRAL;
    ai_neutral_turn(&g);
    CHECK_EQ(g.cr[s].x, 5);
    /* airborne neutrals land first */
    setup(&g, 2, 0);
    int b = spawn(&g, CR_BAT, NEUTRAL, 0, 14);
    g.cr[b].air = true;
    g.cur = NEUTRAL;
    ai_neutral_turn(&g);
    CHECK(!g.cr[b].air);
    /* cannot land on water: stops */
    setup(&g, 2, 0);
    b = spawn(&g, CR_BAT, NEUTRAL, 8, 8);
    g.cr[b].air = true;
    g.cur = NEUTRAL;
    ai_neutral_turn(&g);
    CHECK(g.cr[b].air);
}

static void steps(void)
{
    setup(&g, 2, 0);
    int gob = spawn(&g, CR_GOBLIN, 0, 4, 3);
    /* path leads through the closed door, which a goblin opens */
    CHECK(ai_step_toward(&g, gob, 10, 4));
    for (int i = 0; i < 6; i++) ai_step_toward(&g, gob, 10, 4);
    CHECK(g.cr[gob].x > 6);
    /* a wolf has no hands: trapped behind a closed door */
    setup(&g, 2, 0);
    int wolf = spawn(&g, CR_WOLF, 0, 4, 3);
    CHECK(!ai_step_toward(&g, wolf, 10, 4));
    /* blocked by AP */
    setup(&g, 2, 0);
    gob = spawn(&g, CR_GOBLIN, 0, 10, 13);
    g.cr[gob].ap = 0;
    CHECK(!ai_step_toward(&g, gob, 14, 13));
}

static void computer_wizard(void)
{
    /* on turn one the computer summons */
    game_init(&g, &TEST_SCENARIO, 1, 1, 3);
    game_end_turn(&g);
    CHECK_EQ(g.cur, 0);
    CHECK(game_count_creatures(&g, 1) > 1);
    /* the computer heads for the open portal */
    setup(&g, 1, 1);
    g.portal_open = true;
    map_set(&g.map, 11, 15, T_PORTAL);
    int w = g.pl[1].wizard;
    int d0 = map_dist(&g.map, g.cr[w].x, g.cr[w].y, 11, 15);
    g.cur = 1;
    g.cr[w].ap = 14;
    ai_take_turn(&g, 1);
    CHECK(map_dist(&g.map, g.cr[w].x, g.cr[w].y, 11, 15) < d0 || g.over);
    /* it casts lightning at a visible enemy */
    setup(&g, 1, 1);
    w = g.pl[1].wizard;
    int foe = spawn(&g, CR_TROLL, 0, 12, 9);
    g.cur = 1;
    g.cr[w].mana = 10;
    game_update_vis(&g, 1);
    ai_take_turn(&g, 1);
    CHECK(g.cr[foe].con < 24 || !g.cr[foe].alive);
    /* creatures attack visible enemies and pursue homes later in the game */
    setup(&g, 1, 1);
    g.turn = 6;
    g.cur = 1;
    int t = spawn(&g, CR_TROLL, 1, 12, 13);
    spawn(&g, CR_GOBLIN, 1, 13, 13);
    spawn(&g, CR_WOLF, 1, 11, 13);
    g.cr[g.pl[1].wizard].mana = 0;
    ai_take_turn(&g, 1);
    CHECK(map_dist(&g.map, g.cr[t].x, g.cr[t].y, 4, 4) < map_dist(&g.map, 12, 13, 4, 4));
    /* full AI vs AI games always finish */
    for (uint64_t seed = 1; seed <= 4; seed++) {
        game_init(&g, &SCENARIO_VALE, 0, 2, seed);
        CHECK(g.over);
    }
    game_init(&g, &SCENARIO_VALE, 0, 4, 9);
    CHECK(g.over);
    /* identical seeds give identical games */
    static Game g2;
    game_init(&g, &SCENARIO_VALE, 0, 3, 77);
    game_init(&g2, &SCENARIO_VALE, 0, 3, 77);
    CHECK_EQ(game_hash(&g), game_hash(&g2));
}

void test_ai(void)
{
    neutral();
    steps();
    computer_wizard();
}
