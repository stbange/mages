#include "test.h"

static Game g;

static int wiz(void) { return g.pl[0].wizard; }

static void basics(void)
{
    setup(&g, 2, 0);
    int w = wiz();
    CHECK(!game_spell_needs_target(SP_GOBLINS));
    CHECK(game_spell_needs_target(SP_BOLT));
    CHECK(game_can_afford(&g, w, SP_DRAGON));
    g.cr[w].mana = 5;
    CHECK(!game_can_afford(&g, w, SP_BOLT));
    CHECK_EQ(game_cast(&g, w, SP_BOLT, 4, 3), R_NO_MANA);
    g.cr[w].mana = 60;
    g.cr[w].ap = 3;
    CHECK(!game_can_afford(&g, w, SP_BOLT));
    CHECK_EQ(game_cast(&g, w, SP_BOLT, 4, 3), R_NO_AP);
    g.cr[w].ap = 20;
    int gob = spawn(&g, CR_GOBLIN, 0, 10, 10);
    CHECK_EQ(game_cast(&g, gob, SP_BOLT, 4, 3), R_NOT_YOURS);
    CHECK_EQ(game_cast(&g, g.pl[1].wizard, SP_BOLT, 4, 3), R_NOT_YOURS);
    for (int s = 0; s < SP_COUNT; s++) {
        CHECK(SPELLS[s].name != NULL);
        CHECK(SPELLS[s].mana > 0);
    }
}

static void summons(void)
{
    setup(&g, 2, 0);
    int w = wiz();
    int mana = g.cr[w].mana;
    CHECK_EQ(game_cast(&g, w, SP_GOBLINS, 0, 0), R_OK);
    CHECK_EQ(game_count_creatures(&g, 0), 3);
    CHECK_EQ(g.cr[w].mana, mana - SPELLS[SP_GOBLINS].mana);
    CHECK_EQ(g.cr[w].ap, CREATURES[CR_WIZARD].ap - CAST_AP);
    int last = g.ncr - 1;
    CHECK_EQ(g.cr[last].type, CR_GOBLIN);
    CHECK_EQ(g.cr[last].ap, 0);
    CHECK_EQ(g.cr[last].owner, 0);
    /* summoning works on turn one too */
    game_init(&g, &TEST_SCENARIO, 2, 0, 1);
    CHECK_EQ(game_cast(&g, wiz(), SP_DRAGON, 0, 0), R_OK);
    /* level greater than free space: partial; no space: refused without cost */
    setup(&g, 2, 0);
    w = wiz();
    for (int d = 1; d < DIR_COUNT; d++) spawn(&g, CR_GOBLIN, 0, 4 + DIR_DX[d], 4 + DIR_DY[d]);
    CHECK_EQ(game_cast(&g, w, SP_GOBLINS, 0, 0), R_OK);
    CHECK_EQ(game_count_creatures(&g, 0), 9);
    mana = g.cr[w].mana;
    CHECK_EQ(game_cast(&g, w, SP_GOBLINS, 0, 0), R_NO_ROOM);
    CHECK_EQ(g.cr[w].mana, mana);
    /* creature table full */
    setup(&g, 2, 0);
    g.ncr = MAX_CREATURES;
    CHECK_EQ(game_cast(&g, wiz(), SP_GOBLINS, 0, 0), R_NO_ROOM);
}

static void targeted(void)
{
    setup(&g, 2, 0);
    int w = wiz();
    g.cr[w].x = 8;
    g.cr[w].y = 1;
    CHECK_EQ(game_cast(&g, w, SP_BOLT, 4, 4), R_NOT_VISIBLE);
    CHECK_EQ(game_cast(&g, w, SP_BOLT, 10, 1), R_NO_TARGET);
    /* lightning hurts even undead */
    int sk = spawn(&g, CR_SKELETON, 1, 10, 1);
    CHECK_EQ(game_cast(&g, w, SP_BOLT, 10, 1), R_OK);
    CHECK(g.cr[sk].con < CREATURES[CR_SKELETON].con || !g.cr[sk].alive);
    g.cr[w].ap = 20;
    int t = spawn(&g, CR_TROLL, 1, 12, 1);
    CHECK_EQ(game_cast(&g, w, SP_SPARKS, 12, 1), R_OK);
    CHECK(g.cr[t].con < CREATURES[CR_TROLL].con);
    /* fireball hits the square and its neighbours, both layers */
    setup(&g, 2, 0);
    w = wiz();
    g.cr[w].x = 8;
    g.cr[w].y = 1;
    int a = spawn(&g, CR_TROLL, 1, 12, 2);
    int b = spawn(&g, CR_TROLL, 1, 13, 3);
    int c = spawn(&g, CR_BAT, 1, 13, 2);
    g.cr[c].air = true;
    CHECK_EQ(game_cast(&g, w, SP_FIREBALL, 12, 2), R_OK);
    CHECK(g.cr[a].con < 24);
    CHECK(g.cr[b].con < 24);
    CHECK(g.cr[c].con < 5 || !g.cr[c].alive);
    CHECK_EQ(game_pop_event(&g), GE_HIT);
    /* utility spells need your own creature */
    setup(&g, 2, 0);
    w = wiz();
    g.cr[w].x = 8;
    g.cr[w].y = 1;
    int own = spawn(&g, CR_TROLL, 0, 10, 1);
    int foe = spawn(&g, CR_TROLL, 1, 11, 1);
    g.cr[w].mana = 99;
    CHECK_EQ(game_cast(&g, w, SP_HEAL, 11, 1), R_NO_TARGET);
    CHECK_EQ(game_cast(&g, w, SP_INVIS, 11, 1), R_NO_TARGET);
    CHECK_EQ(game_cast(&g, w, SP_QUICKEN, 11, 1), R_NO_TARGET);
    CHECK_EQ(game_cast(&g, w, SP_QUICKEN, 8, 1), R_NO_TARGET); /* not on yourself */
    g.cr[own].con = 3;
    CHECK_EQ(game_cast(&g, w, SP_HEAL, 10, 1), R_OK);
    CHECK_EQ(g.cr[own].con, 13);
    g.cr[w].ap = 20;
    CHECK_EQ(game_cast(&g, w, SP_INVIS, 10, 1), R_OK);
    CHECK_EQ(g.cr[own].invisible, 3);
    g.cr[w].ap = 20;
    g.cr[own].ap = 0;
    CHECK_EQ(game_cast(&g, w, SP_QUICKEN, 10, 1), R_OK);
    CHECK_EQ(g.cr[own].ap, CREATURES[CR_TROLL].ap);
    /* casting an attack spell makes an invisible wizard visible */
    g.cr[w].ap = 20;
    g.cr[w].invisible = 2;
    CHECK_EQ(game_cast(&g, w, SP_SPARKS, 11, 1), R_OK);
    CHECK_EQ(g.cr[w].invisible, 0);
    /* teleport */
    g.cr[w].ap = 20;
    CHECK_EQ(game_cast(&g, w, SP_TELEPORT, 11, 1), R_BLOCKED);
    CHECK_EQ(game_cast(&g, w, SP_TELEPORT, 8, 8), R_BLOCKED); /* water */
    CHECK_EQ(game_cast(&g, w, SP_TELEPORT, 13, 4), R_OK);
    CHECK_EQ(g.cr[w].x, 13);
    CHECK_EQ(g.cr[w].y, 4);
    /* banish: never wizards, sometimes works */
    g.cr[w].ap = 99;
    g.cr[w].mana = 99;
    CHECK_EQ(game_cast(&g, w, SP_BANISH, 13, 3), R_NO_TARGET);
    int wy = g.pl[1].wizard;
    g.cr[wy].x = 13;
    g.cr[wy].y = 6;
    CHECK_EQ(game_cast(&g, w, SP_BANISH, 13, 6), R_NO_TARGET);
    int banished = 0, failed = 0;
    for (int i = 0; i < 20; i++) {
        int v = spawn(&g, CR_BAT, 1, 14, 4);
        g.cr[w].ap = 99;
        g.cr[w].mana = 99;
        game_cast(&g, w, SP_BANISH, 14, 4);
        if (g.cr[v].alive) {
            failed++;
            g.cr[v].alive = false;
        } else {
            banished++;
        }
    }
    CHECK(banished > 0);
    CHECK(failed > 0);
    (void)foe;
}

static void range(void)
{
    game_init(&g, &SCENARIO_VALE, 1, 1, 1);
    int w = wiz();
    CHECK_EQ(game_cast(&g, w, SP_BOLT, g.cr[w].x, g.cr[w].y + 10), R_OUT_OF_RANGE);
}

static void suicide(void)
{
    /* a wizard can kill itself with its own fireball */
    setup(&g, 2, 0);
    int w = wiz();
    g.cr[w].x = 8;
    g.cr[w].y = 1;
    g.cr[w].con = 1;
    CHECK_EQ(game_cast(&g, w, SP_FIREBALL, 9, 1), R_OK);
    CHECK(!g.cr[w].alive);
    CHECK(g.over);
}

void test_spells(void)
{
    basics();
    summons();
    targeted();
    range();
    suicide();
}
