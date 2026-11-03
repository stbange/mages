#include "ai.h"
#include "test.h"

static Game g;

static void init_and_queries(void)
{
    CHECK(!game_init(&g, &TEST_SCENARIO, 0, 0, 1));
    CHECK(!game_init(&g, &TEST_SCENARIO, 3, 2, 1));
    CHECK(!game_init(&g, &TEST_SCENARIO, -1, 2, 1));
    Scenario bad = TEST_SCENARIO;
    static const char *const rows[] = {"Z"};
    bad.rows = rows;
    bad.h = 1;
    CHECK(!game_init(&g, &bad, 1, 1, 1));
    CHECK(game_init(&g, &TEST_SCENARIO, 2, 0, 1));
    CHECK_EQ(g.npl, 2);
    CHECK_EQ(g.turn, 1);
    CHECK_EQ(g.cur, 0);
    CHECK(!g.pl[0].ai);
    CHECK_STR(g.pl[0].name, "WIZARD 1");
    int w = g.pl[0].wizard;
    CHECK_EQ(g.cr[w].x, 4);
    CHECK_EQ(g.cr[w].y, 4);
    CHECK_EQ(game_creature_at(&g, 4, 4, false), w);
    CHECK_EQ(game_creature_at(&g, 4, 4, true), NONE);
    CHECK_EQ(game_creature_at(&g, 4 + 16, 4, false), w);
    CHECK(g.pl[0].vis[4][4]);
    CHECK(!g.pl[0].vis[0][0]); /* outside the house walls */
    CHECK(g.pl[0].seen[2][2]);  /* the wall itself is seen */
    CHECK_EQ(game_count_creatures(&g, 0), 1);
    CHECK_EQ(game_wizard_caster(&g, w), w);
    CHECK(game_can_see_creature(&g, 0, w));
    CHECK(!game_can_see_creature(&g, 0, g.pl[1].wizard));
    CHECK(!game_can_see_creature(&g, NEUTRAL, g.pl[1].wizard));
    /* the scenario objects and neutrals are placed */
    CHECK(game_init(&g, &SCENARIO_VALE, 1, 1, 1));
    CHECK(g.nobj > 0);
    CHECK(game_object_at(&g, g.obj[0].x, g.obj[0].y) == 0);
    CHECK(game_count_creatures(&g, NEUTRAL) > 0);
    CHECK_EQ(game_object_at(&g, 0, 0), NONE);
}

static void movement(void)
{
    setup(&g, 2, 0);
    int w = g.pl[0].wizard;
    /* first turn rule */
    g.turn = 1;
    CHECK_EQ(game_move(&g, w, DIR_E), R_FIRST_TURN);
    g.turn = 2;
    g.nev = 0;
    int ap = g.cr[w].ap;
    CHECK_EQ(game_move(&g, w, DIR_E), R_OK);
    CHECK_EQ(g.cr[w].x, 5);
    CHECK_EQ(g.cr[w].ap, ap - TERRAIN[T_FLOOR].cost);
    CHECK_EQ(g.cr[w].facing, DIR_E);
    CHECK_EQ(game_pop_event(&g), GE_STEP);
    CHECK_EQ(game_pop_event(&g), -1);
    /* walls and closed doors block */
    CHECK_EQ(game_move(&g, w, DIR_E), R_BLOCKED);
    CHECK_STR(g.msg, "CANNOT GO THERE");
    CHECK_EQ(game_pop_event(&g), GE_ERROR);
    /* not enough AP */
    g.cr[w].ap = 1;
    CHECK_EQ(game_move(&g, w, DIR_W), R_NO_AP);
    /* someone else's creature */
    CHECK_EQ(game_move(&g, g.pl[1].wizard, DIR_W), R_NOT_YOURS);
    /* friendly occupied */
    g.cr[w].ap = 20;
    spawn(&g, CR_GOBLIN, 0, 4, 4);
    CHECK_EQ(game_move(&g, w, DIR_W), R_BLOCKED);
    /* wrap around and water */
    setup(&g, 2, 0);
    int gob = spawn(&g, CR_GOBLIN, 0, 15, 0);
    CHECK_EQ(game_move(&g, gob, DIR_E), R_OK);
    CHECK_EQ(g.cr[gob].x, 0);
    int d = spawn(&g, CR_DWARF, 0, 7, 8);
    CHECK_EQ(game_move(&g, d, DIR_E), R_BLOCKED);
    CHECK_EQ(game_terrain_cost(&g, d, 8, 8), -1);
    CHECK_EQ(game_terrain_cost(&g, d, 4, 11), TERRAIN[T_SWAMP].cost);
    /* tireless creatures keep stamina */
    int sk = spawn(&g, CR_SKELETON, 0, 0, 13);
    int st = g.cr[sk].stamina;
    CHECK_EQ(game_move(&g, sk, DIR_E), R_OK);
    CHECK_EQ(g.cr[sk].stamina, st);
    int st2 = g.cr[d].stamina;
    CHECK_EQ(game_move(&g, d, DIR_W), R_OK);
    CHECK_EQ(g.cr[d].stamina, st2 - 1);
    /* game over blocks actions */
    g.over = true;
    CHECK_EQ(game_move(&g, d, DIR_W), R_GAME_OVER);
    CHECK_EQ(game_fly(&g, d), R_GAME_OVER);
    CHECK_EQ(game_attack(&g, d, sk), R_GAME_OVER);
    CHECK_EQ(game_dismount(&g, d), R_GAME_OVER);
    CHECK_EQ(game_cast(&g, g.pl[0].wizard, SP_BOLT, 0, 0), R_GAME_OVER);
    g.over = false;
}

static void engagement_and_combat(void)
{
    setup(&g, 2, 0);
    int a = spawn(&g, CR_TROLL, 0, 10, 2);
    int b = spawn(&g, CR_GOBLIN, 1, 11, 2);
    CHECK(game_engaged(&g, a));
    CHECK_EQ(game_move(&g, a, DIR_W), R_ENGAGED);
    /* invisible enemies do not engage */
    g.cr[b].invisible = 2;
    CHECK(!game_engaged(&g, a));
    g.cr[b].invisible = 0;
    /* moving into an enemy attacks it */
    int ap = g.cr[a].ap;
    CHECK_EQ(game_move(&g, a, DIR_E), R_OK);
    CHECK_EQ(g.cr[a].ap, ap - ATTACK_AP);
    CHECK_EQ(g.cr[a].facing, DIR_E);
    /* attack validation */
    CHECK_EQ(game_attack(&g, b, a), R_NOT_YOURS);
    int far = spawn(&g, CR_GOBLIN, 1, 14, 14);
    CHECK_EQ(game_attack(&g, a, far), R_NO_TARGET);
    g.cr[a].ap = 1;
    if (g.cr[b].alive) CHECK_EQ(game_attack(&g, a, b), R_NO_AP);
    /* fight to the death: VP is awarded and the event raised */
    setup(&g, 2, 0);
    a = spawn(&g, CR_DRAGON, 0, 10, 2);
    b = spawn(&g, CR_GOBLIN, 1, 11, 2);
    int vp = g.pl[0].vp;
    for (int i = 0; i < 40 && g.cr[b].alive; i++) {
        g.cr[a].ap = 20;
        g.cr[a].con = 40;
        game_attack(&g, a, b);
    }
    CHECK(!g.cr[b].alive);
    CHECK_EQ(g.pl[0].vp, vp + CREATURES[CR_GOBLIN].vp);
    /* hits and misses both happen over many rolls, as do counter attacks */
    setup(&g, 2, 0);
    a = spawn(&g, CR_GOBLIN, 0, 10, 2);
    b = spawn(&g, CR_GOBLIN, 1, 11, 2);
    int hits = 0, misses = 0;
    for (int i = 0; i < 60; i++) {
        g.cr[a].ap = 20;
        g.cr[a].con = g.cr[b].con = 99;
        g.nev = 0;
        game_attack(&g, a, b);
        int e;
        while ((e = game_pop_event(&g)) >= 0) {
            if (e == GE_HIT) hits++;
            if (e == GE_MISS) misses++;
        }
    }
    CHECK(hits > 0);
    CHECK(misses > 0);
    /* attacking reveals invisible attackers; exhausted defenders do not strike back */
    g.cr[a].invisible = 3;
    g.cr[a].ap = 20;
    g.cr[b].stamina = 0;
    int con_a = g.cr[a].con;
    game_attack(&g, a, b);
    CHECK_EQ(g.cr[a].invisible, 0);
    CHECK_EQ(g.cr[a].con, con_a);
    /* air and ground do not fight */
    setup(&g, 2, 0);
    a = spawn(&g, CR_BAT, 0, 10, 2);
    g.cr[a].air = true;
    b = spawn(&g, CR_GOBLIN, 1, 11, 2);
    CHECK(!game_engaged(&g, a));
    CHECK_EQ(game_attack(&g, a, b), R_NO_TARGET);
}

static void undead_and_wounds(void)
{
    setup(&g, 2, 0);
    int a = spawn(&g, CR_GOBLIN, 0, 10, 2);
    int u = spawn(&g, CR_SKELETON, 1, 11, 2);
    for (int i = 0; i < 30; i++) {
        g.cr[a].ap = 20;
        g.cr[a].con = 8;
        game_attack(&g, a, u);
    }
    CHECK_EQ(g.cr[u].con, CREATURES[CR_SKELETON].con); /* ordinary weapons do nothing */
    /* a rune sword works */
    g.cr[a].inv[g.cr[a].ninv++] = OB_SWORD;
    CHECK_EQ(game_combat(&g, a), CREATURES[CR_GOBLIN].combat + 3);
    for (int i = 0; i < 60 && g.cr[u].alive; i++) {
        g.cr[a].ap = 20;
        g.cr[a].con = 8;
        game_attack(&g, a, u);
    }
    CHECK(g.cr[u].con < CREATURES[CR_SKELETON].con);
    /* wounds */
    int t = spawn(&g, CR_TROLL, 0, 0, 14);
    CHECK(!game_wounded(&g, t));
    CHECK_EQ(game_max_ap(&g, t), 10);
    g.cr[t].con = 5;
    CHECK(game_wounded(&g, t));
    CHECK_EQ(game_max_ap(&g, t), 7);
    CHECK_EQ(game_combat(&g, t), 6);
    CHECK_EQ(game_defence(&g, t), 4);
    g.cr[t].inv[g.cr[t].ninv++] = OB_SHIELD;
    CHECK_EQ(game_defence(&g, t), 6);
    CHECK(game_carries(&g, t, OB_SHIELD));
    CHECK(!game_carries(&g, t, OB_SWORD));
    /* damage kills and drops carried objects */
    int objs = 0;
    game_damage(&g, t, 100, 1);
    CHECK(!g.cr[t].alive);
    for (int i = 0; i < g.nobj; i++) objs += g.obj[i].used && g.obj[i].x == 0 && g.obj[i].y == 14;
    CHECK_EQ(objs, 1);
    game_kill(&g, t, 1); /* killing twice is harmless */
}

static void flying(void)
{
    setup(&g, 2, 0);
    int b = spawn(&g, CR_BAT, 0, 4, 3); /* indoors */
    CHECK_EQ(game_fly(&g, b), R_BLOCKED);
    int gob = spawn(&g, CR_GOBLIN, 0, 10, 10);
    CHECK_EQ(game_fly(&g, gob), R_CANNOT);
    CHECK_EQ(game_land(&g, gob), R_CANNOT);
    int h = spawn(&g, CR_HARPY, 0, 7, 9);
    g.cr[h].ap = 1;
    CHECK_EQ(game_fly(&g, h), R_NO_AP);
    g.cr[h].ap = 20;
    CHECK_EQ(game_fly(&g, h), R_OK);
    CHECK(g.cr[h].air);
    CHECK_EQ(game_fly(&g, h), R_CANNOT);
    CHECK_EQ(game_creature_at(&g, 7, 9, true), h);
    /* fly over water for a fixed cost */
    int ap = g.cr[h].ap;
    CHECK_EQ(game_move(&g, h, DIR_E), R_OK);
    CHECK_EQ(g.cr[h].ap, ap - AIR_MOVE_AP);
    CHECK_EQ(game_land(&g, h), R_BLOCKED); /* water */
    CHECK_EQ(game_move(&g, h, DIR_E), R_OK);
    CHECK_EQ(game_move(&g, h, DIR_E), R_OK); /* now at 10,9 on grass */
    g.cr[h].ap = 1;
    CHECK_EQ(game_land(&g, h), R_NO_AP);
    g.cr[h].ap = 20;
    /* flyers cannot enter roofed squares */
    int b2 = spawn(&g, CR_BAT, 0, 7, 4);
    g.cr[b2].air = true;
    CHECK_EQ(game_move(&g, b2, DIR_W), R_BLOCKED);
    /* air space occupied */
    int b3 = spawn(&g, CR_BAT, 0, 7, 4);
    CHECK_EQ(game_fly(&g, b3), R_BLOCKED);
    /* landing on an occupied square is refused */
    spawn(&g, CR_GOBLIN, 0, 7, 4);
    CHECK_EQ(game_land(&g, b2), R_BLOCKED);
    CHECK_EQ(game_land(&g, h), R_OK);
    CHECK(!g.cr[h].air);
}

static void doors_and_objects(void)
{
    setup(&g, 2, 0);
    int w = g.pl[0].wizard;
    g.cr[w].x = 5;
    CHECK_EQ(game_use_door(&g, w, 6, 4), R_OK);
    CHECK_EQ(map_get(&g.map, 6, 4), T_DOOR_OPEN);
    CHECK(g.pl[0].vis[4][8]); /* can see out now */
    CHECK_EQ(game_move(&g, w, DIR_E), R_OK);
    CHECK_EQ(game_use_door(&g, w, 6, 4), R_OUT_OF_RANGE); /* standing in it */
    CHECK_EQ(game_move(&g, w, DIR_E), R_OK);
    g.obj[0] = (Object){true, OB_HEAL_POTION, 6, 4};
    g.nobj = 1;
    CHECK_EQ(game_use_door(&g, w, 6, 4), R_BLOCKED); /* object in doorway */
    g.nobj = 0;
    g.cr[w].ap = 1;
    CHECK_EQ(game_use_door(&g, w, 6, 4), R_NO_AP);
    g.cr[w].ap = 20;
    CHECK_EQ(game_use_door(&g, w, 6, 4), R_OK);
    CHECK_EQ(map_get(&g.map, 6, 4), T_DOOR_CLOSED);
    CHECK_EQ(game_use_door(&g, w, 8, 4), R_NO_TARGET);
    CHECK_EQ(game_use_door(&g, w, 12, 12), R_OUT_OF_RANGE);
    int wolf = spawn(&g, CR_WOLF, 0, 7, 3);
    CHECK_EQ(game_use_door(&g, wolf, 6, 4), R_CANNOT);
    CHECK_EQ(game_pickup(&g, wolf), R_CANNOT);
    /* objects */
    CHECK_EQ(game_pickup(&g, w), R_NO_TARGET);
    g.obj[0] = (Object){true, OB_HEAL_POTION, 7, 4};
    g.obj[1] = (Object){true, OB_MANA_POTION, 7, 4};
    g.obj[2] = (Object){true, OB_SWORD, 7, 4};
    g.obj[3] = (Object){true, OB_SHIELD, 7, 4};
    g.obj[4] = (Object){true, OB_SHIELD, 7, 4};
    g.nobj = 5;
    g.cr[w].ap = 1;
    CHECK_EQ(game_pickup(&g, w), R_NO_AP);
    g.cr[w].ap = 99;
    for (int i = 0; i < 4; i++) CHECK_EQ(game_pickup(&g, w), R_OK);
    CHECK_EQ(g.cr[w].ninv, 4);
    CHECK_EQ(game_pickup(&g, w), R_CANNOT);
    g.cr[w].con = 5;
    g.cr[w].mana = 10;
    CHECK_EQ(game_drink(&g, w), R_OK);
    CHECK_EQ(g.cr[w].con, 17);
    CHECK_EQ(game_drink(&g, w), R_OK);
    CHECK_EQ(g.cr[w].mana, 30);
    CHECK_EQ(game_drink(&g, w), R_NO_TARGET);
    g.cr[w].ap = 0;
    CHECK_EQ(game_drop(&g, w), R_NO_AP);
    g.cr[w].ap = 99;
    CHECK_EQ(game_drop(&g, w), R_OK);
    CHECK_EQ(game_drop(&g, w), R_OK);
    CHECK_EQ(game_drop(&g, w), R_NO_TARGET);
    /* a goblin cannot drink a mana potion but can drink healing */
    int gob = spawn(&g, CR_GOBLIN, 0, 10, 10);
    g.cr[gob].inv[0] = OB_MANA_POTION;
    g.cr[gob].ninv = 1;
    CHECK_EQ(game_drink(&g, gob), R_NO_TARGET);
    g.cr[gob].inv[1] = OB_HEAL_POTION;
    g.cr[gob].ninv = 2;
    g.cr[gob].ap = 1;
    CHECK_EQ(game_drink(&g, gob), R_NO_AP);
    g.cr[gob].ap = 9;
    CHECK_EQ(game_drink(&g, gob), R_OK);
    CHECK_EQ(g.cr[gob].ninv, 1);
    /* the object table can fill up */
    for (int i = 0; i < MAX_OBJECTS; i++) g.obj[i].used = true;
    g.nobj = MAX_OBJECTS;
    CHECK_EQ(game_drop(&g, gob), R_OK); /* silently lost */
}

static void riding(void)
{
    setup(&g, 2, 0);
    int w = g.pl[0].wizard;
    g.cr[w].x = 8;
    g.cr[w].y = 1;
    int u = spawn(&g, CR_UNICORN, 0, 9, 1);
    int gob = spawn(&g, CR_GOBLIN, 0, 7, 1);
    CHECK_EQ(game_ride(&g, w, gob), R_CANNOT);
    g.cr[w].ap = 1;
    CHECK_EQ(game_ride(&g, w, u), R_NO_AP);
    g.cr[w].ap = 20;
    CHECK_EQ(game_ride(&g, w, u), R_OK);
    CHECK_EQ(g.cr[w].mount, u);
    CHECK_EQ(g.cr[u].rider, w);
    CHECK(!game_on_map(&g.cr[w]));
    CHECK_EQ(game_creature_at(&g, 9, 1, false), u);
    CHECK_EQ(game_wizard_caster(&g, u), w);
    CHECK_EQ(game_wizard_caster(&g, gob), NONE);
    CHECK_EQ(game_move(&g, w, DIR_E), R_CANNOT); /* riders move with the mount */
    CHECK_EQ(game_move(&g, u, DIR_E), R_OK);
    CHECK_EQ(g.cr[w].x, 10);
    /* the wizard can cast while riding but cannot summon */
    CHECK_EQ(game_cast(&g, w, SP_GOBLINS, 0, 0), R_CANNOT);
    CHECK_EQ(game_cast(&g, w, SP_TELEPORT, 12, 1), R_CANNOT);
    CHECK_EQ(game_dismount(&g, gob), R_CANNOT);
    g.cr[w].ap = 1;
    CHECK_EQ(game_dismount(&g, u), R_NO_AP);
    g.cr[w].ap = 20;
    CHECK_EQ(game_dismount(&g, u), R_OK);
    CHECK(game_on_map(&g.cr[w]));
    CHECK_EQ(g.cr[u].rider, NONE);
    /* no room to dismount */
    CHECK_EQ(game_ride(&g, w, u), R_OK);
    for (int d = 0; d < DIR_COUNT; d++) spawn(&g, CR_GOBLIN, 0, 10 + DIR_DX[d], 1 + DIR_DY[d]);
    CHECK_EQ(game_dismount(&g, u), R_NO_ROOM);
    /* mount dies: rider is placed on its square */
    game_kill(&g, u, 1);
    CHECK(g.cr[w].alive);
    CHECK(game_on_map(&g.cr[w]));
    CHECK_EQ(g.cr[w].x, 10);
    /* a flying mount carries its rider into the air */
    setup(&g, 2, 0);
    w = g.pl[0].wizard;
    g.cr[w].x = 8;
    g.cr[w].y = 1;
    int p = spawn(&g, CR_PEGASUS, 0, 9, 1);
    CHECK_EQ(game_ride(&g, w, p), R_OK);
    CHECK_EQ(game_fly(&g, p), R_OK);
    CHECK(g.cr[w].air);
    CHECK_EQ(game_dismount(&g, p), R_CANNOT);
    g.cr[p].ap = 99;
    CHECK_EQ(game_move(&g, p, DIR_S), R_OK);
    CHECK_EQ(game_move(&g, p, DIR_S), R_OK);
    CHECK_EQ(game_move(&g, p, DIR_S), R_OK);
    CHECK_EQ(game_move(&g, p, DIR_S), R_OK);
    CHECK_EQ(game_move(&g, p, DIR_S), R_OK);
    CHECK_EQ(game_move(&g, p, DIR_S), R_OK);
    CHECK_EQ(game_move(&g, p, DIR_S), R_OK); /* over the water at 9,8 */
    CHECK_EQ(map_get(&g.map, g.cr[p].x, g.cr[p].y), T_WATER);
    /* killing the pegasus over water drowns the rider (the wizard) */
    game_kill(&g, p, 1);
    CHECK(!g.cr[w].alive);
    CHECK(!g.pl[0].alive);
    CHECK(g.over);
    CHECK_EQ(g.winner, 1);
}

static void victory(void)
{
    /* killing a wizard eliminates the player and all creatures */
    setup(&g, 1, 2);
    int gob = spawn(&g, CR_GOBLIN, 1, 0, 0);
    game_kill(&g, g.pl[1].wizard, 0);
    CHECK(!g.pl[1].alive);
    CHECK(!g.cr[gob].alive);
    CHECK(!g.over);
    CHECK_EQ(g.pl[0].vp, CREATURES[CR_WIZARD].vp);
    /* a rider of an eliminated player vanishes with its mount */
    int mnt = spawn(&g, CR_UNICORN, 2, 0, 5);
    int rdr = spawn(&g, CR_GOBLIN, 2, 1, 5);
    g.cur = 2;
    CHECK_EQ(game_ride(&g, rdr, mnt), R_OK);
    g.cur = 0;
    game_kill(&g, g.pl[2].wizard, 0);
    CHECK(!g.cr[rdr].alive);
    CHECK(!g.cr[mnt].alive);
    CHECK(g.over);
    CHECK_EQ(g.winner, 0);
    CHECK_EQ(game_pop_event(&g), GE_DEATH);
    /* all humans dead ends the game */
    setup(&g, 1, 2);
    game_kill(&g, g.pl[0].wizard, 1);
    CHECK(g.over);
    CHECK_EQ(g.winner, NONE);
    /* escape through the portal */
    setup(&g, 2, 0);
    int w = g.pl[0].wizard;
    g.portal_open = true;
    map_set(&g.map, 11, 15, T_PORTAL);
    g.cr[w].x = 10;
    g.cr[w].y = 15;
    int dg = spawn(&g, CR_GOBLIN, 0, 11, 14);
    CHECK_EQ(game_move(&g, dg, DIR_S), R_OK); /* goblins do not escape */
    CHECK(!g.over);
    game_move(&g, dg, DIR_N);
    CHECK_EQ(game_move(&g, w, DIR_E), R_OK);
    CHECK(g.over);
    CHECK_EQ(g.winner, 0);
    CHECK_EQ(g.pl[0].vp, 100);
    /* a wizard flying over the portal does not escape until it lands */
    setup(&g, 2, 0);
    w = g.pl[0].wizard;
    g.portal_open = true;
    g.cr[w].x = 11;
    g.cr[w].y = 15;
    int p = spawn(&g, CR_PEGASUS, 0, 11, 14);
    g.cr[w].y = 13;
    CHECK_EQ(game_ride(&g, w, p), R_OK);
    CHECK_EQ(game_fly(&g, p), R_OK);
    CHECK_EQ(game_move(&g, p, DIR_S), R_OK);
    CHECK(!g.over);
    CHECK_EQ(game_land(&g, p), R_OK);
    CHECK(g.over);
    /* a single-player game has no last-man-standing win */
    game_init(&g, &TEST_SCENARIO, 1, 0, 1);
    game_end_turn(&g);
    CHECK(!g.over);
    /* everyone dies: draw */
    setup(&g, 0 + 2, 0);
    g.humans = 0;
    g.pl[0].alive = false;
    game_kill(&g, g.pl[1].wizard, NEUTRAL);
    CHECK(g.over);
    CHECK_EQ(g.winner, NONE);
}

static void turns(void)
{
    game_init(&g, &TEST_SCENARIO, 2, 0, 1);
    int w0 = g.pl[0].wizard, w1 = g.pl[1].wizard;
    g.cr[w0].ap = 0;
    game_end_turn(&g);
    CHECK_EQ(g.cur, 1);
    CHECK_EQ(g.turn, 1);
    CHECK_EQ(g.cr[w1].ap, CREATURES[CR_WIZARD].ap);
    game_end_turn(&g);
    CHECK_EQ(g.cur, 0);
    CHECK_EQ(g.turn, 2);
    CHECK_EQ(g.cr[w0].ap, CREATURES[CR_WIZARD].ap);
    CHECK_EQ(g.cr[w0].mana, START_MANA + 2 * MANA_REGEN);
    /* exhaustion halves AP; stamina recovers */
    g.cr[w0].stamina = 1;
    game_end_turn(&g);
    game_end_turn(&g);
    CHECK_EQ(g.cr[w0].ap, CREATURES[CR_WIZARD].ap / 2);
    CHECK_EQ(g.cr[w0].stamina, 1 + CREATURES[CR_WIZARD].stamina / 5);
    /* invisibility wears off */
    g.cr[w0].invisible = 1;
    game_end_turn(&g);
    game_end_turn(&g);
    CHECK_EQ(g.cr[w0].invisible, 0);
    /* the portal opens on its turn */
    while (g.turn < TEST_SCENARIO.portal_turn) game_end_turn(&g);
    CHECK(g.portal_open);
    CHECK_EQ(map_get(&g.map, 11, 15), T_PORTAL);
    /* dead players are skipped */
    game_init(&g, &TEST_SCENARIO, 3, 0, 1);
    g.pl[1].alive = false;
    game_end_turn(&g);
    CHECK_EQ(g.cur, 2);
    game_end_turn(&g);
    CHECK_EQ(g.cur, 0);
    game_end_turn(&g);
    game_end_turn(&g);
    /* the turn limit ends the game */
    game_init(&g, &TEST_SCENARIO, 2, 0, 1);
    g.turn = MAX_TURNS;
    game_end_turn(&g);
    game_end_turn(&g);
    CHECK(g.over);
    game_end_turn(&g); /* no-op once over */
    /* events overflow quietly */
    for (int i = 0; i < MAX_EVENTS + 5; i++) game_event(&g, GE_STEP);
    CHECK_EQ(g.nev, MAX_EVENTS);
    /* hash changes with state */
    uint32_t h = game_hash(&g);
    g.cr[0].x++;
    CHECK(h != game_hash(&g));
    /* creature table full */
    g.ncr = MAX_CREATURES;
    CHECK_EQ(game_spawn(&g, CR_GOBLIN, 0, 1, 1), NONE);
}

static void visibility(void)
{
    Map m;
    map_load(&m, TEST_SCENARIO.rows, 16);
    CHECK(game_los(&m, 0, 0, 10, 0));
    CHECK(!game_los(&m, 1, 4, 8, 4));  /* through the house */
    CHECK(game_los(&m, 0, 0, 2, 2));   /* the wall itself is visible */
    CHECK(game_los(&m, 4, 4, 4, 4));
    CHECK(!game_los(&m, 4, 9, 4, 11)); /* tree at 4,10 */
    CHECK(game_los(&m, 15, 0, 1, 0));  /* wraps */
    setup(&g, 2, 0);
    int gob = spawn(&g, CR_GOBLIN, 0, 10, 0);
    game_update_vis(&g, 0);
    CHECK(g.pl[0].vis[0][13]);
    int e = spawn(&g, CR_GOBLIN, 1, 12, 0);
    CHECK(game_can_see_creature(&g, 0, e));
    g.cr[e].invisible = 2;
    CHECK(!game_can_see_creature(&g, 0, e));
    CHECK(game_can_see_creature(&g, 1, e));
    g.cr[gob].alive = false;
    CHECK(!game_can_see_creature(&g, 0, gob));
}

void test_game(void)
{
    init_and_queries();
    movement();
    engagement_and_combat();
    undead_and_wounds();
    flying();
    doors_and_objects();
    riding();
    victory();
    turns();
    visibility();
}
