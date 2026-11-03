#include "audio.h"
#include "palette.h"
#include "test.h"
#include "ui.h"

static Ui u;
static Framebuffer fb;

static void frame(void)
{
    ui_tick(&u);
    ui_render(&u, &fb);
}

static void in(InputKind k)
{
    ui_input(&u, (Input){k, 0, 0});
    frame();
}

static void tap(int x, int y)
{
    ui_input(&u, (Input){IN_TAP, x, y});
    frame();
}

static bool choose(const char *label)
{
    for (int i = 0; i < u.menu.n; i++) {
        if (strncmp(u.menu.label[i], label, strlen(label)) == 0) {
            Rect r = ui_menu_item_rect(&u, i);
            tap(r.x + 1, r.y + 1);
            return true;
        }
    }
    printf("menu entry %s not found\n", label);
    return false;
}

static bool has_entry(const char *label)
{
    for (int i = 0; i < u.menu.n; i++)
        if (strncmp(u.menu.label[i], label, strlen(label)) == 0) return true;
    return false;
}

static void tapmap(int mx, int my)
{
    int sx, sy;
    if (!ui_map_to_screen(&u, mx, my, &sx, &sy)) {
        printf("square %d,%d not in view\n", mx, my);
        t_fail++;
        return;
    }
    tap(sx, sy);
}

/* Start a 1 human vs 1 computer game on the test map, at turn 2. */
static void start_test_game(int humans, int ais)
{
    ui_init(&u, 5);
    u.humans = humans;
    u.ais = ais;
    game_init(&u.game, &TEST_SCENARIO, humans, ais, 5);
    u.game.turn = 2;
    u.game.nev = 0;
    u.mode = M_CURSOR;
    const Creature *w = &u.game.cr[u.game.pl[0].wizard];
    u.cx = w->x;
    u.cy = w->y;
    u.vx = 0;
    u.vy = 0;
    frame();
}

static int wiz(void) { return u.game.pl[0].wizard; }

static void title(void)
{
    ui_init(&u, 1);
    frame();
    CHECK_EQ(u.mode, M_TITLE);
    CHECK_STR(ui_mode_name(u.mode), "TITLE");
    CHECK_EQ(u.menu.sel, 2);
    in(IN_UP);
    CHECK_EQ(u.menu.sel, 1);
    in(IN_DOWN);
    in(IN_DOWN);
    CHECK_EQ(u.menu.sel, 0);
    in(IN_CANCEL);
    CHECK_EQ(u.mode, M_TITLE);
    tap(5, 5); /* outside the menu: ignored on the title screen */
    CHECK_EQ(u.mode, M_TITLE);
    in(IN_LEFT); /* ignored */
    /* cycle humans 1..4 and back */
    in(IN_FIRE);
    CHECK_EQ(u.humans, 2);
    CHECK_EQ(u.ais, 1);
    in(IN_FIRE);
    CHECK_EQ(u.humans, 3);
    in(IN_FIRE);
    CHECK_EQ(u.humans, 4);
    CHECK_EQ(u.ais, 0);
    in(IN_FIRE);
    CHECK_EQ(u.humans, 1);
    CHECK_EQ(u.ais, 1);
    /* computers: 1..3 then wraps to the minimum */
    choose("COMPUTER");
    CHECK_EQ(u.ais, 2);
    choose("COMPUTER");
    choose("COMPUTER");
    CHECK_EQ(u.ais, 1);
    choose("PLAYERS");
    choose("COMPUTER");
    CHECK_EQ(u.ais, 2);
    choose("COMPUTER");
    CHECK_EQ(u.ais, 0);
    /* two humans: start shows the hand-over screen */
    choose("START GAME");
    CHECK_EQ(u.mode, M_HANDOVER);
    in(IN_LEFT);
    CHECK_EQ(u.mode, M_HANDOVER);
    in(IN_FIRE);
    CHECK_EQ(u.mode, M_CURSOR);
    /* one human vs computer goes straight to the map */
    ui_init(&u, 1);
    choose("START GAME");
    CHECK_EQ(u.mode, M_CURSOR);
    CHECK_EQ(u.cx, 5);
    CHECK_EQ(u.cy, 5);
    int s, n = 0;
    while ((s = ui_pop_sfx(&u)) >= 0) n++;
    CHECK(n > 0);
    CHECK_EQ(ui_pop_sfx(&u), -1);
    CHECK_EQ(ui_viewer(&u), 0);
}

static void cursor_and_scroll(void)
{
    start_test_game(1, 1);
    static const InputKind dirs[] = {IN_UP, IN_DOWN, IN_LEFT, IN_RIGHT, IN_UPLEFT, IN_UPRIGHT, IN_DOWNLEFT, IN_DOWNRIGHT};
    for (int i = 0; i < 8; i++) in(dirs[i]);
    CHECK_EQ(u.cx, 4);
    CHECK_EQ(u.cy, 4);
    for (int i = 0; i < 12; i++) in(IN_RIGHT);
    CHECK_EQ(u.cx, 0);
    int sx, sy;
    CHECK(ui_map_to_screen(&u, u.cx, u.cy, &sx, &sy));
    for (int i = 0; i < 20; i++) in(IN_LEFT);
    CHECK(ui_map_to_screen(&u, u.cx, u.cy, &sx, &sy));
    for (int i = 0; i < 20; i++) in(IN_DOWN);
    CHECK(ui_map_to_screen(&u, u.cx, u.cy, &sx, &sy));
    for (int i = 0; i < 20; i++) in(IN_UP);
    CHECK(ui_map_to_screen(&u, u.cx, u.cy, &sx, &sy));
    /* NEXT cycles own creatures */
    int gob = spawn(&u.game, CR_GOBLIN, 0, 10, 1);
    u.cx = 4;
    u.cy = 4;
    in(IN_NEXT);
    CHECK_EQ(u.cx, u.game.cr[gob].x);
    in(IN_NEXT);
    CHECK_EQ(u.cx, 4);
    u.cx = 0;
    u.cy = 0;
    in(IN_NEXT);
    CHECK_EQ(u.cx, 4);
    /* VIEW key on an empty square does nothing */
    u.cx = 0;
    in(IN_VIEW);
    CHECK_EQ(u.mode, M_CURSOR);
    in(IN_CANCEL);
    CHECK_EQ(u.mode, M_CURSOR);
    in(IN_NONE);
    /* map squares outside the view */
    u.vx = 0;
    u.vy = 0;
    CHECK(!ui_map_to_screen(&u, 12, 12, &sx, &sy));
}

static void cursor_menu(void)
{
    start_test_game(1, 1);
    in(IN_FIRE);
    CHECK_EQ(u.mode, M_MENU);
    CHECK(has_entry("SELECT-G"));
    CHECK(!has_entry("SELECT-A"));
    in(IN_UP);
    in(IN_DOWN);
    in(IN_CANCEL);
    CHECK_EQ(u.mode, M_CURSOR);
    /* tap outside the menu cancels it */
    in(IN_FIRE);
    tap(20, 20);
    CHECK_EQ(u.mode, M_CURSOR);
    /* NEXT from the menu */
    in(IN_FIRE);
    choose("NEXT");
    CHECK_EQ(u.mode, M_CURSOR);
    /* end turn needs confirmation */
    in(IN_FIRE);
    choose("END TURN");
    CHECK_STR(u.menu.title, "END TURN?");
    choose("CONTINUE");
    CHECK_EQ(u.mode, M_CURSOR);
    CHECK_EQ(u.game.turn, 2);
    in(IN_FIRE);
    choose("END TURN");
    choose("END TURN");
    CHECK_EQ(u.game.turn, 3);
    CHECK_EQ(u.mode, M_CURSOR);
    CHECK_EQ(u.cx, u.game.cr[wiz()].x);
    /* SELECT-A for an airborne creature */
    int b = spawn(&u.game, CR_BAT, 0, 10, 1);
    u.game.cr[b].air = true;
    u.cx = 10;
    u.cy = 1;
    in(IN_FIRE);
    CHECK(has_entry("SELECT-A"));
    CHECK(has_entry("VIEW 3D"));
    choose("SELECT-A");
    CHECK_EQ(u.mode, M_SELECTED);
    CHECK_EQ(u.sel, b);
    frame();
    in(IN_FIRE);
    CHECK(has_entry("LAND"));
    choose("LAND");
    CHECK(!u.game.cr[b].air);
    in(IN_FIRE);
    choose("FLY");
    CHECK(u.game.cr[b].air);
    in(IN_FIRE);
    choose("END MOVE");
    CHECK_EQ(u.sel, NONE);
    /* selecting where the creature has gone */
    u.cx = 10;
    u.cy = 1;
    in(IN_FIRE);
    u.game.cr[b].alive = false;
    choose("SELECT-A");
    CHECK_EQ(u.mode, M_CURSOR);
    /* view 3D of nothing */
    u.cx = 0;
    u.cy = 0;
    u.menu_from = M_CURSOR;
    u.mode = M_MENU;
    u.menu.n = 1;
    u.menu.act[0] = A_VIEW3D;
    u.menu.disabled[0] = false;
    u.menu.sel = 0;
    in(IN_FIRE);
    CHECK_EQ(u.mode, M_CURSOR);
}

static void selected_moves(void)
{
    start_test_game(1, 1);
    in(IN_FIRE);
    choose("SELECT-G");
    CHECK_EQ(u.mode, M_SELECTED);
    CHECK_EQ(u.sel, wiz());
    /* fire on an adjacent square moves, and the cursor keeps its offset */
    in(IN_RIGHT);
    in(IN_FIRE);
    CHECK_EQ(u.game.cr[wiz()].x, 5);
    CHECK_EQ(u.cx, 6);
    in(IN_FIRE); /* door: blocked, cursor stays */
    CHECK_EQ(u.game.cr[wiz()].x, 5);
    CHECK_EQ(u.cx, 6);
    /* fire on the creature opens its menu */
    in(IN_LEFT);
    in(IN_FIRE);
    CHECK_EQ(u.mode, M_MENU);
    CHECK(has_entry("CAST"));
    CHECK(has_entry("USE"));
    CHECK(has_entry("END TURN"));
    choose("CANCEL");
    CHECK_EQ(u.mode, M_SELECTED);
    /* USE with a single door acts at once */
    in(IN_FIRE);
    choose("USE");
    CHECK_EQ(map_get(&u.game.map, 6, 4), T_DOOR_OPEN);
    CHECK_EQ(u.mode, M_SELECTED);
    /* panel tap opens the selected menu */
    tap(250, 100);
    CHECK_EQ(u.mode, M_MENU);
    choose("CANCEL");
    /* tap twice on an adjacent square walks */
    tapmap(6, 4);
    tapmap(6, 4);
    CHECK_EQ(u.game.cr[wiz()].x, 6);
    /* engaged: cannot walk away */
    int e = spawn(&u.game, CR_GOBLIN, 1, 7, 5);
    u.game.cr[e].con = 99;
    u.cx = 6;
    u.cy = 3;
    in(IN_FIRE);
    CHECK_EQ(u.game.cr[wiz()].y, 4);
    CHECK(strstr(u.msg, "ENGAGED") != NULL);
    /* attacking an enemy keeps the cursor on it */
    u.cx = 7;
    u.cy = 5;
    in(IN_FIRE);
    CHECK_EQ(u.cx, 7);
    /* cancel ends the selection */
    in(IN_CANCEL);
    CHECK_EQ(u.mode, M_CURSOR);
    CHECK_EQ(u.sel, NONE);
    /* the selected creature dies */
    u.cx = 6;
    u.cy = 4;
    in(IN_FIRE);
    choose("SELECT-G");
    game_kill(&u.game, e, 0);
    u.game.nev = 0;
    int gob = spawn(&u.game, CR_GOBLIN, 0, 10, 10);
    u.sel = gob;
    u.cx = 11;
    u.cy = 10;
    u.game.cr[gob].alive = false;
    in(IN_FIRE);
    CHECK_EQ(u.mode, M_CURSOR);
    /* a creature killed in its own attack */
    u.game.cr[gob].alive = true;
    u.sel = gob;
    u.mode = M_SELECTED;
    u.game.cr[gob].con = 1;
    int t = spawn(&u.game, CR_DRAGON, 1, 11, 10);
    (void)t;
    for (int i = 0; i < 30 && u.game.cr[gob].alive; i++) {
        u.game.cr[gob].ap = 10;
        u.mode = M_SELECTED;
        u.sel = gob;
        u.cx = 11;
        u.cy = 10;
        in(IN_FIRE);
    }
    CHECK_EQ(u.mode, M_CURSOR);
}

static void spells(void)
{
    start_test_game(1, 1);
    in(IN_FIRE);
    choose("SELECT-G");
    in(IN_FIRE);
    choose("CAST");
    CHECK_EQ(u.mode, M_SPELLS);
    CHECK(has_entry("GOBLINS"));
    choose("MORE SPELLS");
    CHECK(has_entry("PEGASUS"));
    choose("MORE SPELLS");
    CHECK(has_entry("TELEPORT"));
    choose("MORE SPELLS");
    CHECK(has_entry("GOBLINS"));
    choose("CANCEL");
    CHECK_EQ(u.mode, M_SELECTED);
    /* summon */
    in(IN_FIRE);
    choose("CAST");
    choose("GOBLINS");
    CHECK_EQ(game_count_creatures(&u.game, 0), 3);
    CHECK_EQ(u.mode, M_SELECTED);
    /* unaffordable spells are refused */
    u.game.cr[wiz()].mana = 1;
    in(IN_FIRE);
    choose("CAST");
    choose("GOBLINS");
    CHECK_EQ(u.mode, M_SPELLS);
    CHECK_STR(u.msg, "CANNOT DO THAT NOW");
    in(IN_CANCEL);
    CHECK_EQ(u.mode, M_SELECTED);
    /* targeted spell through the target mode */
    u.game.cr[wiz()].mana = 60;
    int own = spawn(&u.game, CR_TROLL, 0, 3, 5);
    u.game.cr[own].con = 2;
    in(IN_FIRE);
    choose("CAST");
    choose("MORE SPELLS");
    choose("HEAL");
    CHECK_EQ(u.mode, M_TARGET);
    in(IN_DOWNLEFT);
    in(IN_FIRE);
    CHECK_EQ(u.game.cr[own].con, 12);
    CHECK_EQ(u.mode, M_SELECTED);
    /* cancel a target */
    in(IN_FIRE);
    choose("CAST");
    choose("MORE SPELLS");
    choose("LIGHTNING");
    in(IN_CANCEL);
    CHECK_EQ(u.mode, M_SELECTED);
    /* tapping the panel in target mode cancels too */
    in(IN_FIRE);
    choose("CAST");
    choose("MORE SPELLS");
    choose("LIGHTNING");
    tap(300, 100);
    CHECK_EQ(u.mode, M_SELECTED);
    /* target mode after the caster died */
    in(IN_FIRE);
    choose("CAST");
    choose("MORE SPELLS");
    choose("LIGHTNING");
    u.game.cr[wiz()].alive = false;
    in(IN_FIRE);
    CHECK_EQ(u.mode, M_CURSOR);
    in(IN_CANCEL);
    /* a fireball that ends the game */
    start_test_game(1, 1);
    int ew = u.game.pl[1].wizard;
    u.game.cr[ew].x = 6;
    u.game.cr[ew].y = 4;
    u.game.cr[ew].con = 1;
    map_set(&u.game.map, 6, 4, T_DOOR_OPEN);
    game_update_vis(&u.game, 0);
    in(IN_FIRE);
    choose("SELECT-G");
    in(IN_FIRE);
    choose("CAST");
    choose("MORE SPELLS");
    choose("FIREBALL");
    in(IN_RIGHT);
    in(IN_RIGHT);
    in(IN_FIRE);
    CHECK(u.game.over);
    CHECK_EQ(u.mode, M_GAMEOVER);
    CHECK_EQ(ui_viewer(&u), NONE);
    frame();
    in(IN_LEFT);
    CHECK_EQ(u.mode, M_GAMEOVER);
    in(IN_FIRE);
    CHECK_EQ(u.mode, M_TITLE);
}

static void objects_and_riding(void)
{
    start_test_game(1, 1);
    Game *g = &u.game;
    g->obj[0] = (Object){true, OB_HEAL_POTION, 4, 4};
    g->obj[1] = (Object){true, OB_MANA_POTION, 4, 4};
    g->nobj = 2;
    in(IN_FIRE);
    choose("SELECT-G");
    in(IN_FIRE);
    choose("PICK UP");
    in(IN_FIRE);
    choose("PICK UP");
    CHECK_EQ(g->cr[wiz()].ninv, 2);
    frame();
    g->cr[wiz()].con = 3;
    in(IN_FIRE);
    choose("DRINK");
    CHECK(g->cr[wiz()].con > 3);
    in(IN_FIRE);
    choose("DROP");
    CHECK_EQ(g->cr[wiz()].ninv, 0);
    /* riding a single adjacent mount acts at once, then dismount */
    int m = spawn(g, CR_UNICORN, 0, 4, 5);
    in(IN_FIRE);
    CHECK(has_entry("RIDE"));
    choose("RIDE");
    CHECK_EQ(u.sel, m);
    CHECK_EQ(g->cr[wiz()].mount, m);
    frame();
    in(IN_FIRE);
    CHECK(has_entry("CAST"));
    CHECK(has_entry("DISMOUNT"));
    choose("DISMOUNT");
    CHECK_EQ(u.sel, wiz());
    CHECK_EQ(g->cr[wiz()].mount, NONE);
    /* dismount refused for lack of AP */
    game_ride(g, wiz(), m);
    u.sel = m;
    u.cx = g->cr[m].x;
    u.cy = g->cr[m].y;
    g->cr[wiz()].ap = 0;
    in(IN_FIRE);
    choose("DISMOUNT");
    CHECK_EQ(u.sel, m);
    /* two mounts: pick one in target mode; empty square refuses */
    start_test_game(1, 1);
    g = &u.game;
    spawn(g, CR_UNICORN, 0, 4, 5);
    int m2 = spawn(g, CR_PEGASUS, 0, 5, 4);
    in(IN_FIRE);
    choose("SELECT-G");
    in(IN_FIRE);
    choose("RIDE");
    CHECK_EQ(u.mode, M_TARGET);
    in(IN_UPLEFT);
    in(IN_FIRE);
    CHECK_EQ(u.mode, M_SELECTED);
    CHECK_EQ(g->cr[wiz()].mount, NONE);
    in(IN_FIRE);
    choose("RIDE");
    in(IN_RIGHT);
    in(IN_FIRE);
    CHECK_EQ(g->cr[wiz()].mount, m2);
    /* a creature with two doors next to it chooses with the cursor */
    start_test_game(1, 1);
    g = &u.game;
    map_set(&g->map, 6, 3, T_DOOR_CLOSED);
    g->cr[wiz()].x = 5;
    in(IN_RIGHT);
    in(IN_FIRE);
    choose("SELECT-G");
    in(IN_FIRE);
    choose("USE");
    CHECK_EQ(u.mode, M_TARGET);
    in(IN_RIGHT);
    in(IN_FIRE);
    CHECK_EQ(map_get(&g->map, 6, 4), T_DOOR_OPEN);
}

static void view3d(void)
{
    start_test_game(1, 1);
    Game *g = &u.game;
    in(IN_VIEW);
    CHECK_EQ(u.mode, M_VIEW3D);
    CHECK_EQ(u.pov, wiz());
    for (int i = 0; i < 10; i++) frame();
    in(IN_LEFT);
    CHECK_EQ(u.pov_facing, DIR_SE);
    in(IN_RIGHT);
    in(IN_UPLEFT);
    in(IN_UPRIGHT);
    in(IN_DOWN);
    CHECK_EQ(u.pov_facing, DIR_N);
    in(IN_UP);
    CHECK_EQ(g->cr[wiz()].y, 3);
    for (int i = 0; i < 20; i++) frame();
    in(IN_NEXT); /* ignored */
    /* taps: left, right, middle, exit strip */
    tap(VIEW_X + 5, VIEW_Y + 50);
    CHECK_EQ(u.pov_facing, DIR_NW);
    tap(VIEW_X + VIEW_PX - 5, VIEW_Y + 50);
    CHECK_EQ(u.pov_facing, DIR_N);
    tap(VIEW_X + VIEW_PX / 2, VIEW_Y + 50);
    tap(VIEW_X + VIEW_PX / 2, VIEW_Y + VIEW_PX - 3);
    CHECK_EQ(u.mode, M_CURSOR);
    /* from the selected menu, and back to selected */
    in(IN_FIRE);
    choose("SELECT-G");
    in(IN_FIRE);
    choose("VIEW 3D");
    CHECK_EQ(u.mode, M_VIEW3D);
    in(IN_FIRE);
    CHECK_EQ(u.mode, M_SELECTED);
    in(IN_VIEW);
    CHECK_EQ(u.mode, M_VIEW3D);
    tap(300, 100);
    CHECK_EQ(u.mode, M_SELECTED);
    /* from the cursor menu */
    in(IN_CANCEL);
    in(IN_FIRE);
    choose("VIEW 3D");
    CHECK_EQ(u.mode, M_VIEW3D);
    in(IN_CANCEL);
    /* camera follows across the wrap-around edge */
    int gob = spawn(g, CR_GOBLIN, 0, 0, 0);
    u.cx = 0;
    u.cy = 0;
    in(IN_VIEW);
    u.pov_facing = DIR_NW;
    in(IN_UP);
    for (int i = 0; i < 40; i++) frame();
    CHECK(u.cam_x > 14.0f);
    CHECK(u.cam_y > 14.0f);
    u.pov_facing = DIR_SE;
    in(IN_UP);
    for (int i = 0; i < 40; i++) frame();
    CHECK(u.cam_x < 2.0f);
    /* the viewed creature dies while stepping forward */
    g->cr[gob].con = 1;
    int d = spawn(g, CR_DRAGON, 1, 1, 1);
    for (int i = 0; i < 40 && g->cr[gob].alive; i++) {
        g->cr[gob].ap = 10;
        u.mode = M_VIEW3D;
        u.pov = gob;
        u.pov_facing = DIR_SE;
        in(IN_UP);
    }
    CHECK(!g->cr[gob].alive);
    CHECK_EQ(u.mode, M_CURSOR);
    (void)d;
    /* not your creature: forward does nothing */
    u.mode = M_VIEW3D;
    u.pov = g->pl[1].wizard;
    u.pov_facing = DIR_N;
    int y0 = g->cr[u.pov].y;
    in(IN_UP);
    CHECK_EQ(g->cr[u.pov].y, y0);
    in(IN_CANCEL);
    /* escaping through the portal from the 3D view */
    start_test_game(1, 1);
    g = &u.game;
    g->portal_open = true;
    map_set(&g->map, 11, 15, T_PORTAL);
    g->cr[wiz()].x = 11;
    g->cr[wiz()].y = 14;
    u.cx = 11;
    u.cy = 14;
    in(IN_VIEW);
    u.pov_facing = DIR_S;
    in(IN_UP);
    CHECK_EQ(u.mode, M_GAMEOVER);
}

static void taps_and_handover(void)
{
    start_test_game(2, 0);
    tapmap(5, 5);
    CHECK_EQ(u.cx, 5);
    tapmap(5, 5);
    CHECK_EQ(u.mode, M_MENU);
    tap(20, 20);
    tap(250, 100); /* panel opens the cursor menu */
    CHECK_EQ(u.mode, M_MENU);
    tap(20, 20);
    tap(186, 195); /* neither map nor panel */
    CHECK_EQ(u.mode, M_CURSOR);
    in(IN_FIRE);
    choose("END TURN");
    choose("END TURN");
    CHECK_EQ(u.mode, M_HANDOVER);
    CHECK_EQ(u.game.cur, 1);
    tap(10, 10);
    CHECK_EQ(u.mode, M_CURSOR);
    /* landing on the portal ends the game */
    start_test_game(1, 1);
    Game *g = &u.game;
    g->portal_open = true;
    map_set(&g->map, 11, 15, T_PORTAL);
    int p = spawn(g, CR_PEGASUS, 0, 11, 15);
    g->cr[wiz()].x = 11;
    g->cr[wiz()].y = 14;
    game_ride(g, wiz(), p);
    g->cr[p].air = true;
    g->cr[wiz()].air = true;
    u.sel = p;
    u.mode = M_SELECTED;
    u.cx = 11;
    u.cy = 15;
    in(IN_FIRE);
    choose("LAND");
    CHECK_EQ(u.mode, M_GAMEOVER);
    /* a game that ends while the computer moves */
    start_test_game(1, 1);
    g = &u.game;
    g->cr[wiz()].con = 1;
    int ed = spawn(g, CR_DRAGON, 1, 4, 3);
    (void)ed;
    for (int i = 0; i < 50 && !g->over; i++) {
        u.mode = M_MENU;
        u.menu.n = 1;
        u.menu.act[0] = A_CONFIRM_END;
        u.menu.disabled[0] = false;
        u.menu.sel = 0;
        in(IN_FIRE);
        g->cr[wiz()].con = 1;
    }
    CHECK(g->over);
    CHECK_EQ(u.mode, M_GAMEOVER);
    /* a game that is over immediately (all computers) */
    ui_init(&u, 3);
    u.humans = 0;
    u.ais = 2;
    ui_start_game(&u);
    CHECK_EQ(u.mode, M_GAMEOVER);
    frame();
    /* message and sound queue limits */
    ui_message(&u, "HELLO");
    CHECK_STR(u.msg, "HELLO");
    u.nsfx = 0;
    for (int i = 0; i < 40; i++) {
        u.game.nev = 0;
        game_event(&u.game, GE_STEP);
        u.mode = M_MENU;
        u.menu.n = 1;
        u.menu.act[0] = A_CANCEL;
        u.menu.sel = 0;
        in(IN_FIRE);
    }
    CHECK_EQ(u.nsfx, 16);
}

static void render_checks(void)
{
    start_test_game(1, 1);
    Game *g = &u.game;
    /* every pixel uses the palette in every mode */
    int b = spawn(g, CR_BAT, 0, 5, 1);
    g->cr[b].air = true;
    int m = spawn(g, CR_UNICORN, 0, 9, 1);
    int r = spawn(g, CR_GOBLIN, 0, 8, 1);
    g->cr[r].ap = 9;
    game_ride(g, r, m);
    int sk = spawn(g, CR_SKELETON, 0, 10, 1);
    g->cr[sk].con = 1;
    g->cr[sk].invisible = 2;
    g->cr[sk].inv[0] = OB_SWORD;
    g->cr[sk].ninv = 1;
    g->obj[0] = (Object){true, OB_SHIELD, 12, 1};
    g->nobj = 1;
    int e = spawn(g, CR_GOBLIN, 1, 12, 2);
    (void)e;
    map_set(&g->map, 6, 4, T_DOOR_OPEN);
    game_update_vis(g, 0);
    u.vx = 2;
    u.vy = 0;
    for (int t = 0; t < 30; t++) {
        u.tick = (uint32_t)t;
        ui_render(&u, &fb);
    }
    for (int y = 0; y < SCREEN_H; y++)
        for (int x = 0; x < SCREEN_W; x++)
            if (fb.px[y][x] > 15) t_fail++;
    /* info panel content for several creatures */
    static const int at[][2] = {{5, 1}, {9, 1}, {10, 1}, {12, 1}, {12, 2}, {0, 15}, {4, 4}};
    for (unsigned i = 0; i < sizeof at / sizeof at[0]; i++) {
        u.cx = at[i][0];
        u.cy = at[i][1];
        ui_render(&u, &fb);
    }
    /* portal open text and selection marker */
    g->portal_open = true;
    map_set(&g->map, 11, 15, T_PORTAL);
    u.cx = 11;
    u.cy = 15;
    u.vx = 6;
    u.vy = 8;
    u.sel = wiz();
    u.mode = M_TARGET;
    for (int t = 0; t < 20; t++) {
        u.tick = (uint32_t)t;
        ui_render(&u, &fb);
    }
    u.mode = M_SELECTED;
    u.cx = g->cr[wiz()].x;
    u.cy = g->cr[wiz()].y;
    u.vx = 0;
    u.vy = 0;
    ui_render(&u, &fb);
    u.sel = b;
    u.cx = 5;
    u.cy = 0;
    u.tick = 1;
    ui_render(&u, &fb);
    /* hit flash border */
    u.fx_timer = 3;
    ui_render(&u, &fb);
    CHECK_EQ(fb.px[0][0], C_RED);
    /* menus with disabled entries */
    u.sel = wiz();
    g->cr[wiz()].mana = 0;
    u.mode = M_MENU;
    u.menu_from = M_SELECTED;
    u.menu.n = 1;
    u.menu.act[0] = A_CAST;
    u.menu.disabled[0] = false;
    u.menu.sel = 0;
    in(IN_FIRE);
    CHECK_EQ(u.mode, M_SPELLS);
    in(IN_DOWN);
    ui_render(&u, &fb);
    /* hand-over and game over screens */
    u.mode = M_HANDOVER;
    for (int t = 0; t < 41; t += 20) {
        u.tick = (uint32_t)t;
        ui_render(&u, &fb);
    }
    g->over = true;
    g->winner = 0;
    g->pl[1].alive = false;
    u.mode = M_GAMEOVER;
    for (int t = 0; t < 41; t += 20) {
        u.tick = (uint32_t)t;
        ui_render(&u, &fb);
    }
    g->winner = NONE;
    ui_render(&u, &fb);
    /* 3D view renders with the panel */
    g->over = false;
    u.mode = M_VIEW3D;
    u.pov = wiz();
    u.msg_timer = 0;
    ui_render(&u, &fb);
    CHECK(fb.px[VIEW_Y + 10][VIEW_X + 10] < 16);
    /* square panel for an unseen square */
    u.mode = M_CURSOR;
    u.cx = 15;
    u.cy = 15;
    g->pl[0].seen[15][15] = 0;
    ui_render(&u, &fb);
    (void)audio_play;
}

void test_ui(void)
{
    title();
    cursor_and_scroll();
    cursor_menu();
    selected_moves();
    spells();
    objects_and_riding();
    view3d();
    taps_and_handover();
    render_checks();
}
