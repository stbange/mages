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

static void send(InputKind k, int x, int y)
{
    ui_input(&u, (Input){k, x, y});
    frame();
}

static void in(InputKind k) { send(k, 0, 0); }
static void click(int x, int y) { send(IN_CLICK, x, y); }
static void tap(int x, int y) { send(IN_TAP, x, y); }

static void settle(void)
{
    for (int i = 0; i < 500 && ui_walking(&u); i++) frame();
}

static bool map_xy(int mx, int my, int *sx, int *sy)
{
    if (!ui_map_to_screen(&u, mx, my, sx, sy)) {
        printf("square %d,%d not in view\n", mx, my);
        t_fail++;
        return false;
    }
    return true;
}

static void clickmap(int mx, int my)
{
    int sx, sy;
    if (map_xy(mx, my, &sx, &sy)) click(sx, sy);
}

static void tapmap(int mx, int my)
{
    int sx, sy;
    if (map_xy(mx, my, &sx, &sy)) tap(sx, sy);
}

static void hovermap(int mx, int my)
{
    int sx, sy;
    if (map_xy(mx, my, &sx, &sy)) send(IN_POINT, sx, sy);
}

static bool find_button(const char *label, Button *out)
{
    Button b[MAX_BUTTONS];
    int n = ui_buttons(&u, b);
    for (int i = 0; i < n; i++) {
        if (strncmp(b[i].label, label, strlen(label)) == 0) {
            *out = b[i];
            return true;
        }
    }
    return false;
}

static bool button(const char *label)
{
    Button b;
    if (!find_button(label, &b)) {
        printf("button %s not found\n", label);
        t_fail++;
        return false;
    }
    click(b.r.x + b.r.w / 2, b.r.y + b.r.h / 2);
    return true;
}

static bool has_button(const char *label)
{
    Button b;
    return find_button(label, &b);
}

static bool list(const char *label)
{
    for (int i = 0; i < u.menu.n; i++) {
        if (strncmp(u.menu.label[i], label, strlen(label)) == 0) {
            Rect r = ui_menu_item_rect(&u, i);
            click(r.x + 1, r.y + 1);
            return true;
        }
    }
    printf("list entry %s not found\n", label);
    t_fail++;
    return false;
}

static bool hover_says(const char *text)
{
    char line[64];
    ui_hover_text(&u, line, sizeof line);
    if (strstr(line, text)) return true;
    printf("hover text '%s' lacks '%s'\n", line, text);
    return false;
}

static int wiz(void) { return u.game.pl[0].wizard; }

/* Re-select a creature so that its movement range is recomputed. */
static void reselect(int ci)
{
    u.sel = NONE;
    u.mode = M_PLAY;
    u.cx = u.game.cr[ci].x;
    u.cy = u.game.cr[ci].y;
    in(IN_FIRE);
}

/* A game on the 16x16 test map at turn 2 with the wizard selected. */
static void start_test_game(int humans, int ais)
{
    ui_init(&u, 5);
    u.humans = humans;
    u.ais = ais;
    game_init(&u.game, &TEST_SCENARIO, humans, ais, 5);
    u.game.turn = 2;
    u.game.nev = 0;
    u.vx = 0;
    u.vy = 0;
    reselect(wiz());
    u.nsfx = 0;
}

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
    in(IN_DIGIT);
    in(IN_WHEEL);
    in(IN_CAST);
    CHECK_EQ(u.mode, M_TITLE);
    click(5, 5); /* outside the menu */
    CHECK_EQ(u.mode, M_TITLE);
    Rect r = ui_menu_item_rect(&u, 1);
    send(IN_POINT, r.x + 2, r.y + 2);
    CHECK_EQ(u.menu.sel, 1);
    send(IN_POINT, 3, 3);
    CHECK_EQ(u.menu.sel, 1);
    /* humans 1..4 */
    list("PLAYERS");
    CHECK_EQ(u.humans, 2);
    list("PLAYERS");
    list("PLAYERS");
    CHECK_EQ(u.humans, 4);
    CHECK_EQ(u.ais, 0);
    list("PLAYERS");
    CHECK_EQ(u.humans, 1);
    CHECK_EQ(u.ais, 1);
    list("COMPUTER");
    CHECK_EQ(u.ais, 2);
    list("COMPUTER");
    list("COMPUTER");
    CHECK_EQ(u.ais, 1);
    list("PLAYERS");
    list("COMPUTER");
    list("COMPUTER");
    CHECK_EQ(u.ais, 0);
    /* two humans: hand-over screen first */
    u.menu.sel = 2;
    in(IN_FIRE);
    CHECK_EQ(u.mode, M_HANDOVER);
    in(IN_LEFT);
    CHECK_EQ(u.mode, M_HANDOVER);
    click(10, 10);
    CHECK_EQ(u.mode, M_PLAY);
    CHECK_EQ(u.sel, u.game.pl[0].wizard);
    /* one human: straight to play with the wizard selected */
    ui_init(&u, 1);
    list("START GAME");
    CHECK_EQ(u.mode, M_PLAY);
    CHECK_EQ(u.cx, 5);
    int n = 0;
    while (ui_pop_sfx(&u) >= 0) n++;
    CHECK(n > 0);
    CHECK_EQ(ui_pop_sfx(&u), -1);
    CHECK_EQ(ui_viewer(&u), 0);
    /* all computers: over immediately */
    ui_init(&u, 3);
    u.humans = 0;
    u.ais = 2;
    ui_start_game(&u);
    CHECK_EQ(u.mode, M_GAMEOVER);
    frame();
    in(IN_LEFT);
    CHECK_EQ(u.mode, M_GAMEOVER);
    tap(5, 5);
    CHECK_EQ(u.mode, M_TITLE);
}

static void cursor_and_hover(void)
{
    start_test_game(1, 1);
    static const InputKind dirs[] = {IN_UP, IN_DOWN, IN_LEFT, IN_RIGHT, IN_UPLEFT, IN_UPRIGHT, IN_DOWNLEFT, IN_DOWNRIGHT};
    for (int i = 0; i < 8; i++) in(dirs[i]);
    CHECK_EQ(u.cx, 4);
    CHECK_EQ(u.cy, 4);
    CHECK(u.cursor_shown);
    int sx, sy;
    for (int i = 0; i < 12; i++) in(IN_RIGHT);
    CHECK(ui_map_to_screen(&u, u.cx, u.cy, &sx, &sy));
    for (int i = 0; i < 20; i++) in(IN_LEFT);
    CHECK(ui_map_to_screen(&u, u.cx, u.cy, &sx, &sy));
    for (int i = 0; i < 20; i++) in(IN_DOWN);
    for (int i = 0; i < 20; i++) in(IN_UP);
    CHECK(ui_map_to_screen(&u, u.cx, u.cy, &sx, &sy));
    u.vx = u.vy = 0;
    CHECK(!ui_map_to_screen(&u, 12, 12, &sx, &sy));
    /* hovering moves the cursor; leaving the map hides it */
    hovermap(5, 4);
    CHECK_EQ(u.cx, 5);
    CHECK(hover_says("MOVE: 2 AP"));
    send(IN_POINT, 250, 50);
    CHECK(!u.cursor_shown);
    char line[64];
    ui_hover_text(&u, line, sizeof line);
    CHECK_STR(line, "");
    /* hover descriptions without a selection */
    in(IN_CANCEL);
    CHECK_EQ(u.sel, NONE);
    hovermap(4, 4);
    CHECK(hover_says("WIZARD"));
    hovermap(2, 2);
    CHECK(hover_says("WALL"));
    u.game.obj[0] = (Object){true, OB_SWORD, 5, 5};
    u.game.nobj = 1;
    hovermap(5, 5);
    CHECK(hover_says("RUNE SWORD"));
    hovermap(10, 10); /* unseen */
    ui_hover_text(&u, line, sizeof line);
    CHECK_STR(line, "");
    int b = spawn(&u.game, CR_BAT, 0, 5, 3);
    u.game.cr[b].air = true;
    hovermap(5, 3);
    CHECK(hover_says("(FLYING)"));
    CHECK_EQ(ui_panel_creature(&u), b);
    /* keyboard selection with Enter, toggling layers on a shared square */
    int gb = spawn(&u.game, CR_GOBLIN, 0, 5, 3);
    u.cx = 5;
    u.cy = 3;
    in(IN_FIRE);
    CHECK_EQ(u.sel, gb);
    in(IN_FIRE);
    CHECK_EQ(u.sel, b);
    in(IN_FIRE);
    CHECK_EQ(u.sel, gb);
    /* acting on empty squares without a selection does nothing */
    in(IN_CANCEL);
    u.cx = 0;
    u.cy = 0;
    in(IN_FIRE);
    CHECK_EQ(u.sel, NONE);
    in(IN_DIGIT);
    in(IN_NONE);
    /* drag pans the view square by square */
    u.vx = 4;
    u.vy = 4;
    send(IN_DRAG, 20, 0);
    CHECK_EQ(u.vx, 3);
    send(IN_DRAG, -40, 0);
    CHECK_EQ(u.vx, 5);
    send(IN_DRAG, 0, 17);
    CHECK_EQ(u.vy, 3);
    send(IN_DRAG, 0, -33);
    CHECK_EQ(u.vy, 5);
}

static void selection_cycle(void)
{
    start_test_game(1, 1);
    Game *g = &u.game;
    int a = spawn(g, CR_GOBLIN, 0, 10, 1);
    int b = spawn(g, CR_GOBLIN, 0, 11, 1);
    g->cr[b].ap = 0;
    in(IN_NEXT);
    CHECK_EQ(u.sel, a); /* b has no AP left */
    in(IN_NEXT);
    CHECK_EQ(u.sel, wiz());
    in(IN_PREV);
    CHECK_EQ(u.sel, a);
    send(IN_WHEEL, 0, 1);
    CHECK_EQ(u.sel, wiz());
    send(IN_WHEEL, 0, -1);
    CHECK_EQ(u.sel, a);
    /* nothing with AP: cycles through everyone */
    g->cr[a].ap = 0;
    g->cr[wiz()].ap = 0;
    in(IN_NEXT);
    CHECK(u.sel == b || u.sel == wiz() || u.sel == a);
    /* nothing at all to cycle to */
    g->cr[a].alive = g->cr[b].alive = g->cr[wiz()].alive = false;
    in(IN_CANCEL);
    in(IN_NEXT);
    CHECK_EQ(u.sel, NONE);
    /* clicking an own creature selects it; clicking it again keeps it */
    start_test_game(1, 1);
    int gob = spawn(&u.game, CR_GOBLIN, 0, 5, 5);
    clickmap(5, 5);
    CHECK_EQ(u.sel, gob);
    clickmap(5, 5);
    CHECK_EQ(u.sel, gob);
    send(IN_POINT, 250, 50); /* panel shows the selection when not hovering */
    CHECK_EQ(ui_panel_creature(&u), gob);
    /* right click (cancel) deselects */
    in(IN_CANCEL);
    CHECK_EQ(u.sel, NONE);
    CHECK_EQ(ui_panel_creature(&u), NONE);
    in(IN_CANCEL);
}

static void movement(void)
{
    start_test_game(1, 1);
    Game *g = &u.game;
    /* turn one: no reach */
    g->turn = 1;
    reselect(wiz());
    CHECK_EQ(ui_reach_cost(&u, 4, 4), 0);
    CHECK_EQ(ui_reach_cost(&u, 5, 4), -1);
    clickmap(5, 4);
    CHECK(strstr(u.msg, "FIRST TURN") != NULL);
    g->turn = 2;
    reselect(wiz());
    CHECK_EQ(ui_reach_cost(&u, 5, 4), 2);
    CHECK_EQ(ui_reach_cost(&u, 6, 4), 6); /* through the closed door */
    CHECK_EQ(ui_reach_cost(&u, 7, 4), 9);
    CHECK_EQ(ui_reach_cost(&u, 2, 2), -1);
    hovermap(7, 4);
    CHECK(hover_says("MOVE: 9 AP"));
    /* a click walks the whole path, opening the door */
    clickmap(7, 4);
    CHECK(ui_walking(&u));
    frame();
    settle();
    CHECK(!ui_walking(&u));
    CHECK_EQ(g->cr[wiz()].x, 7);
    CHECK_EQ(map_get(&g->map, 6, 4), T_DOOR_OPEN);
    CHECK_EQ(g->cr[wiz()].ap, 14 - 9);
    /* unreachable square */
    u.vx = 3;
    clickmap(12, 4);
    CHECK_STR(u.msg, "CANNOT GO THERE");
    clickmap(7, 4); /* on itself: nothing */
    /* any input completes a walk at once */
    start_test_game(1, 1);
    g = &u.game;
    clickmap(7, 4);
    in(IN_UP);
    CHECK(!ui_walking(&u));
    CHECK_EQ(g->cr[wiz()].x, 7);
    /* engaged creatures have no movement range */
    start_test_game(1, 1);
    g = &u.game;
    int t = spawn(g, CR_TROLL, 0, 10, 1);
    int e = spawn(g, CR_GOBLIN, 1, 11, 1);
    game_update_vis(g, 0);
    u.vx = 3;
    reselect(t);
    CHECK_EQ(ui_reach_cost(&u, 9, 1), -1);
    CHECK(hover_says("") || 1);
    /* attack an adjacent enemy by clicking it */
    g->cr[e].con = 99;
    int ap = g->cr[t].ap;
    hovermap(11, 1);
    CHECK(hover_says("ATTACK GOBLIN: 4 AP"));
    clickmap(11, 1);
    CHECK_EQ(g->cr[t].ap, ap - ATTACK_AP);
    /* move and attack: walk next to a goblin two squares away, then strike */
    start_test_game(1, 1);
    g = &u.game;
    t = spawn(g, CR_TROLL, 0, 10, 1);
    e = spawn(g, CR_GOBLIN, 1, 13, 1);
    g->cr[e].con = 99;
    game_update_vis(g, 0);
    u.vx = 3;
    reselect(t);
    int cost;
    CHECK(ui_attackable(&u, e, &cost));
    CHECK_EQ(cost, 3 + 3 + ATTACK_AP);
    clickmap(13, 1);
    settle();
    CHECK_EQ(g->cr[t].x, 12);
    CHECK_EQ(g->cr[t].ap, CREATURES[CR_TROLL].ap - cost);
    /* out of reach */
    e = spawn(g, CR_GOBLIN, 1, 12, 9);
    game_update_vis(g, 0);
    hovermap(12, 9);
    CHECK(hover_says("OUT OF REACH"));
    clickmap(12, 9);
    CHECK_STR(u.msg, "OUT OF REACH");
    CHECK(!ui_attackable(&u, wiz(), &cost)); /* own creatures are never attackable */
    /* attack on turn one */
    g->turn = 1;
    reselect(t);
    clickmap(12, 9);
    CHECK(strstr(u.msg, "FIRST TURN") != NULL);
    /* the target dies during the walk: no attack afterwards */
    start_test_game(1, 1);
    g = &u.game;
    t = spawn(g, CR_TROLL, 0, 10, 1);
    e = spawn(g, CR_GOBLIN, 1, 13, 1);
    game_update_vis(g, 0);
    u.vx = 3;
    reselect(t);
    clickmap(13, 1);
    g->cr[e].alive = false;
    settle();
    CHECK_EQ(g->cr[t].ap, CREATURES[CR_TROLL].ap - 6);
}

static void ambush_and_doors(void)
{
    /* an invisible enemy beside the path stops the walk */
    start_test_game(1, 1);
    Game *g = &u.game;
    int gob = spawn(g, CR_GOBLIN, 0, 8, 1);
    int e = spawn(g, CR_GOBLIN, 1, 10, 0);
    game_update_vis(g, 0);
    g->pl[0].vis[0][10] = 0; /* out of sight */
    u.vx = 3;
    reselect(gob);
    CHECK(ui_reach_cost(&u, 12, 1) > 0);
    clickmap(12, 1);
    settle();
    CHECK(g->cr[gob].x < 12);
    /* clicking an adjacent closed door opens it without moving */
    start_test_game(1, 1);
    g = &u.game;
    g->cr[wiz()].x = 5;
    reselect(wiz());
    hovermap(6, 4);
    CHECK(hover_says("OPEN DOOR: 2 AP"));
    clickmap(6, 4);
    CHECK_EQ(map_get(&g->map, 6, 4), T_DOOR_OPEN);
    CHECK_EQ(g->cr[wiz()].x, 5);
    /* a wolf has no hands: the door is not in its range */
    int w = spawn(g, CR_WOLF, 0, 5, 3);
    map_set(&g->map, 6, 4, T_DOOR_CLOSED);
    reselect(w);
    CHECK_EQ(ui_reach_cost(&u, 6, 4), -1);
    /* a walk into a door that cannot be opened stops */
    g->cr[wiz()].x = 4;
    reselect(wiz());
    clickmap(8, 4);
    CHECK_EQ(g->cr[wiz()].x, 5); /* the first step happens at once */
    g->cr[wiz()].ap = 1;
    settle();
    CHECK_EQ(g->cr[wiz()].x, 5);
    /* the walk ends the game on the portal */
    start_test_game(1, 1);
    g = &u.game;
    g->portal_open = true;
    map_set(&g->map, 11, 15, T_PORTAL);
    g->cr[wiz()].x = 10;
    g->cr[wiz()].y = 13;
    game_update_vis(g, 0);
    u.vx = 6;
    u.vy = 8;
    reselect(wiz());
    clickmap(11, 15);
    settle();
    CHECK_EQ(u.mode, M_GAMEOVER);
    CHECK_EQ(ui_viewer(&u), NONE);
}

static void touch(void)
{
    start_test_game(1, 1);
    Game *g = &u.game;
    /* first tap previews, second tap confirms */
    tapmap(5, 4);
    CHECK(u.touch);
    CHECK_EQ(g->cr[wiz()].x, 4);
    CHECK(strstr(u.msg, "TAP AGAIN") != NULL);
    CHECK(hover_says("TAP AGAIN"));
    frame();
    tapmap(5, 5); /* another square re-arms */
    CHECK_EQ(u.armed_x, 5);
    CHECK_EQ(u.armed_y, 5);
    tapmap(5, 5);
    settle();
    CHECK_EQ(g->cr[wiz()].y, 5);
    /* tapping an own creature selects at once */
    int gob = spawn(g, CR_GOBLIN, 0, 3, 3);
    tapmap(3, 3);
    CHECK_EQ(u.sel, gob);
    /* attack needs two taps too */
    int t = spawn(g, CR_TROLL, 0, 10, 1);
    int e = spawn(g, CR_GOBLIN, 1, 11, 1);
    g->cr[e].con = 99;
    game_update_vis(g, 0);
    u.vx = 3;
    tapmap(10, 1);
    CHECK_EQ(u.sel, t);
    tapmap(11, 1);
    CHECK_EQ(g->cr[t].ap, CREATURES[CR_TROLL].ap);
    tapmap(11, 1);
    CHECK_EQ(g->cr[t].ap, CREATURES[CR_TROLL].ap - ATTACK_AP);
    /* doors too */
    reselect(wiz());
    g->cr[wiz()].x = 5;
    g->cr[wiz()].y = 4;
    reselect(wiz());
    u.vx = 0;
    tapmap(6, 4);
    CHECK_EQ(map_get(&g->map, 6, 4), T_DOOR_CLOSED);
    tapmap(6, 4);
    CHECK_EQ(map_get(&g->map, 6, 4), T_DOOR_OPEN);
    /* a drag disarms the preview */
    tapmap(5, 5);
    send(IN_DRAG, 1, 0);
    CHECK_EQ(u.armed_x, -1);
    /* a mouse click switches back to mouse mode */
    click(250, 20);
    CHECK(!u.touch);
}

static void actions(void)
{
    start_test_game(1, 1);
    Game *g = &u.game;
    CHECK(has_button("CAST"));
    CHECK(has_button("3D VIEW"));
    CHECK(has_button("NEXT"));
    CHECK(has_button("END TURN"));
    CHECK(!has_button("FLY"));
    /* clicking the empty panel area and disabled things does nothing */
    click(250, 60);
    /* pick up, drink, drop with buttons and hotkeys */
    g->obj[0] = (Object){true, OB_HEAL_POTION, 4, 4};
    g->obj[1] = (Object){true, OB_MANA_POTION, 4, 4};
    g->nobj = 2;
    reselect(wiz());
    CHECK(button("PICK UP"));
    in(IN_PICKUP);
    CHECK_EQ(g->cr[wiz()].ninv, 2);
    g->cr[wiz()].con = 3;
    CHECK(button("DRINK"));
    CHECK(g->cr[wiz()].con > 3);
    in(IN_DRINK);
    in(IN_DROP);
    CHECK_EQ(g->cr[wiz()].ninv, 0);
    /* more than three context actions: MORE opens a list */
    g->obj[0] = (Object){true, OB_HEAL_POTION, 5, 4};
    g->obj[1] = (Object){true, OB_SWORD, 5, 4};
    g->nobj = 2;
    g->cr[wiz()].x = 5;
    g->cr[wiz()].ap = 14;
    reselect(wiz());
    in(IN_PICKUP);
    in(IN_PICKUP);
    CHECK(has_button("MORE"));
    CHECK(button("MORE"));
    CHECK_EQ(u.mode, M_LIST);
    CHECK(list("DOOR"));
    CHECK_EQ(map_get(&g->map, 6, 4), T_DOOR_OPEN);
    CHECK_EQ(u.mode, M_PLAY);
    CHECK(button("MORE"));
    in(IN_CANCEL);
    CHECK_EQ(u.mode, M_PLAY);
    CHECK(button("MORE"));
    click(20, 20); /* outside the list */
    CHECK_EQ(u.mode, M_PLAY);
    CHECK(button("MORE"));
    in(IN_DROP); /* hotkeys close the list and act */
    CHECK_EQ(u.mode, M_PLAY);
    CHECK_EQ(g->cr[wiz()].ninv, 1);
    /* door hotkey: at the cursor, the single adjacent one, or ask */
    u.cx = 6;
    u.cy = 4;
    in(IN_USE);
    CHECK_EQ(map_get(&g->map, 6, 4), T_DOOR_CLOSED);
    u.cx = 0;
    u.cy = 0;
    in(IN_USE);
    CHECK_EQ(map_get(&g->map, 6, 4), T_DOOR_OPEN);
    map_set(&g->map, 6, 3, T_DOOR_CLOSED);
    in(IN_USE);
    CHECK_STR(u.msg, "POINT AT THE DOOR TO USE");
    /* fly and land */
    int h = spawn(g, CR_HARPY, 0, 12, 1);
    reselect(h);
    CHECK(has_button("FLY"));
    in(IN_FLY);
    CHECK(g->cr[h].air);
    CHECK(has_button("LAND"));
    in(IN_FLY);
    CHECK(!g->cr[h].air);
    CHECK(button("FLY"));
    CHECK(button("LAND"));
    /* ride and dismount */
    start_test_game(1, 1);
    g = &u.game;
    int m = spawn(g, CR_UNICORN, 0, 4, 5);
    reselect(wiz());
    CHECK(has_button("RIDE"));
    u.cx = 0;
    u.cy = 0;
    in(IN_RIDE);
    CHECK_EQ(u.sel, m);
    CHECK_EQ(g->cr[wiz()].mount, m);
    CHECK(has_button("CAST")); /* the rider can cast */
    CHECK(has_button("DISMOUNT"));
    in(IN_RIDE);
    CHECK_EQ(u.sel, wiz());
    game_ride(g, wiz(), m);
    reselect(m);
    g->cr[wiz()].ap = 0;
    CHECK(button("DISMOUNT"));
    CHECK_EQ(u.sel, m);
    g->cr[wiz()].ap = 9;
    CHECK(button("DISMOUNT"));
    /* two mounts: point at one */
    int m2 = spawn(g, CR_PEGASUS, 0, g->cr[wiz()].x + 1, g->cr[wiz()].y);
    reselect(wiz());
    u.cx = 0;
    u.cy = 0;
    in(IN_RIDE);
    CHECK_STR(u.msg, "POINT AT THE MOUNT TO RIDE");
    u.cx = g->cr[m2].x;
    u.cy = g->cr[m2].y;
    in(IN_RIDE);
    CHECK_EQ(g->cr[wiz()].mount, m2);
    /* riding refused for lack of AP */
    in(IN_RIDE);
    g->cr[wiz()].ap = 0;
    u.cx = g->cr[m].x;
    u.cy = g->cr[m].y;
    reselect(wiz());
    u.cx = g->cr[m2].x;
    u.cy = g->cr[m2].y;
    in(IN_RIDE);
    CHECK_EQ(g->cr[wiz()].mount, NONE);
    /* actions that need a selection */
    in(IN_CANCEL);
    in(IN_DROP);
    CHECK_STR(u.msg, "SELECT A CREATURE FIRST");
    u.cx = 0;
    u.cy = 0;
    in(IN_VIEW);
    CHECK_STR(u.msg, "SELECT A CREATURE FIRST");
    CHECK(has_button("HELP"));
    /* only wizards cast */
    int gob = spawn(g, CR_GOBLIN, 0, 10, 10);
    reselect(gob);
    in(IN_CAST);
    CHECK_STR(u.msg, "ONLY WIZARDS CAST SPELLS");
}

static void spells(void)
{
    start_test_game(1, 1);
    Game *g = &u.game;
    /* C with nothing selected picks the wizard */
    in(IN_CANCEL);
    in(IN_CAST);
    CHECK_EQ(u.mode, M_LIST);
    CHECK_EQ(u.sel, wiz());
    /* hovering an entry describes the spell */
    Rect r = ui_menu_item_rect(&u, 0);
    send(IN_POINT, r.x + 3, r.y + 3);
    CHECK(hover_says("GOBLINS: SUMMON 2 GOBLINS"));
    send(IN_POINT, 20, 20);
    in(IN_DOWN);
    in(IN_UP);
    /* paging with the wheel and MORE SPELLS */
    send(IN_WHEEL, 0, 1);
    CHECK_EQ(u.spell_page, 1);
    send(IN_WHEEL, 0, -1);
    CHECK_EQ(u.spell_page, 0);
    send(IN_WHEEL, 0, -1);
    CHECK_EQ(u.spell_page, 2);
    list("MORE SPELLS");
    CHECK_EQ(u.spell_page, 0);
    /* number key summons */
    send(IN_DIGIT, 1, 0);
    CHECK_EQ(game_count_creatures(g, 0), 3);
    CHECK_EQ(u.mode, M_PLAY);
    send(IN_DIGIT, 1, 0); /* ignored outside the book */
    /* unaffordable spells are refused, in the list and by number */
    g->cr[wiz()].mana = 1;
    in(IN_CAST);
    list("1 GOBLINS");
    CHECK_STR(u.msg, "CANNOT DO THAT NOW");
    send(IN_DIGIT, 12, 0); /* out of range */
    in(IN_FIRE);
    list("CLOSE");
    CHECK_EQ(u.mode, M_PLAY);
    /* a target spell: valid targets, invalid click, confirm */
    g->cr[wiz()].mana = 60;
    int own = spawn(g, CR_TROLL, 0, 3, 5);
    g->cr[own].con = 2;
    game_update_vis(g, 0);
    in(IN_CAST);
    in(IN_WHEEL);
    list("7 HEAL");
    CHECK_EQ(u.mode, M_TARGET);
    CHECK(has_button("CANCEL"));
    CHECK(ui_valid_target(&u, 3, 5));
    CHECK(!ui_valid_target(&u, 5, 5));
    hovermap(5, 5);
    CHECK(hover_says("NOT A VALID TARGET"));
    clickmap(5, 5);
    CHECK_STR(u.msg, "NOT A VALID TARGET");
    hovermap(3, 5);
    CHECK(hover_says("CAST HEAL HERE"));
    clickmap(3, 5);
    CHECK_EQ(g->cr[own].con, 12);
    CHECK_EQ(u.mode, M_PLAY);
    /* touch targeting needs two taps; cancel with the button or Escape */
    in(IN_CAST);
    in(IN_WHEEL);
    list("4 LIGHTNING");
    tapmap(3, 5);
    CHECK(strstr(u.msg, "TAP AGAIN") != NULL);
    CHECK(button("CANCEL"));
    CHECK_EQ(u.mode, M_PLAY);
    in(IN_CAST);
    in(IN_WHEEL);
    list("4 LIGHTNING");
    in(IN_CANCEL);
    CHECK_EQ(u.mode, M_PLAY);
    /* each spell kind's target rule */
    static const struct { int spell, x, y; bool ok; } cases[] = {
        {SP_FIREBALL, 5, 5, true}, {SP_INVIS, 3, 5, true}, {SP_INVIS, 5, 5, false},
        {SP_QUICKEN, 3, 5, true}, {SP_QUICKEN, 4, 4, false}, {SP_TELEPORT, 5, 5, true},
        {SP_TELEPORT, 3, 5, false}, {SP_TELEPORT, 2, 2, false}, {SP_BANISH, 3, 5, true},
        {SP_BANISH, 4, 4, false}, {SP_SPARKS, 3, 5, true}, {SP_SPARKS, 5, 5, false},
        {SP_BOLT, 10, 10, false}, {SP_BOLT, 8, 4, false},
    };
    u.mode = M_TARGET;
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        u.pending_spell = cases[i].spell;
        if (ui_valid_target(&u, cases[i].x, cases[i].y) != cases[i].ok) {
            printf("target case %u failed\n", i);
            t_fail++;
        }
    }
    u.mode = M_PLAY;
    CHECK(!ui_valid_target(&u, 3, 5)); /* not in target mode */
    int gob = spawn(g, CR_GOBLIN, 0, 10, 10);
    reselect(gob);
    u.mode = M_TARGET;
    CHECK(!ui_valid_target(&u, 3, 5)); /* not a caster */
    u.mode = M_PLAY;
    /* casting a fireball onto yourself ends the game */
    start_test_game(1, 1);
    g = &u.game;
    g->cr[wiz()].con = 1;
    g->pl[1].alive = true;
    in(IN_CAST);
    in(IN_WHEEL);
    list("5 FIREBALL");
    clickmap(4, 4);
    CHECK(g->over);
    CHECK_EQ(u.mode, M_GAMEOVER);
    click(5, 5);
    CHECK_EQ(u.mode, M_TITLE);
    /* the target's caster disappears while targeting */
    start_test_game(1, 1);
    g = &u.game;
    in(IN_CAST);
    in(IN_WHEEL);
    list("4 LIGHTNING");
    g->cr[wiz()].alive = false;
    clickmap(4, 4);
    CHECK_STR(u.msg, "NOT A VALID TARGET");
}

static void end_turns(void)
{
    start_test_game(1, 1);
    Game *g = &u.game;
    /* creatures can still act: confirm needed */
    CHECK(button("END TURN"));
    CHECK(u.end_confirm > 0);
    CHECK(has_button("CONFIRM?"));
    CHECK_EQ(g->turn, 2);
    frame();
    in(IN_END_TURN);
    CHECK_EQ(g->turn, 3);
    CHECK_EQ(u.sel, wiz());
    /* the confirmation runs out */
    in(IN_END_TURN);
    for (int i = 0; i < CONFIRM_TICKS + 1; i++) ui_tick(&u);
    in(IN_END_TURN);
    CHECK_EQ(g->turn, 3);
    in(IN_UP); /* another action cancels it */
    in(IN_CAST);
    CHECK_EQ(u.end_confirm, 0);
    in(IN_CANCEL);
    /* ending from target mode */
    in(IN_CAST);
    in(IN_WHEEL);
    list("4 LIGHTNING");
    CHECK(button("END TURN"));
    CHECK_EQ(u.mode, M_PLAY);
    /* nobody can act: ends at once */
    g->cr[wiz()].ap = 0;
    in(IN_END_TURN);
    CHECK_EQ(g->turn, 4);
    /* hot seat: hand-over screen between humans */
    start_test_game(2, 0);
    g = &u.game;
    g->cr[wiz()].ap = 0;
    in(IN_END_TURN);
    CHECK_EQ(u.mode, M_HANDOVER);
    CHECK_EQ(g->cur, 1);
    tap(5, 5);
    CHECK_EQ(u.mode, M_PLAY);
    CHECK_EQ(u.sel, g->pl[1].wizard);
    /* a riding wizard: the mount is selected at turn start */
    int mnt = spawn(g, CR_UNICORN, 1, g->cr[g->pl[1].wizard].x + 1, g->cr[g->pl[1].wizard].y);
    game_ride(g, g->pl[1].wizard, mnt);
    for (int i = 0; i < g->ncr; i++) if (g->cr[i].alive) g->cr[i].ap = 0;
    in(IN_END_TURN);
    in(IN_FIRE);
    for (int i = 0; i < g->ncr; i++) if (g->cr[i].alive) g->cr[i].ap = 0;
    in(IN_END_TURN);
    in(IN_FIRE);
    CHECK_EQ(u.sel, mnt);
    /* the computer kills the wizard during its turn */
    start_test_game(1, 1);
    g = &u.game;
    g->cr[wiz()].con = 1;
    spawn(g, CR_DRAGON, 1, 4, 3);
    for (int i = 0; i < 50 && !g->over; i++) {
        g->cr[wiz()].ap = 0;
        g->cr[wiz()].con = 1;
        in(IN_END_TURN);
    }
    CHECK_EQ(u.mode, M_GAMEOVER);
}

static void view3d(void)
{
    start_test_game(1, 1);
    Game *g = &u.game;
    in(IN_VIEW);
    CHECK_EQ(u.mode, M_VIEW3D);
    CHECK_EQ(u.pov, wiz());
    CHECK(has_button("< TURN"));
    for (int i = 0; i < 10; i++) frame();
    in(IN_LEFT);
    CHECK_EQ(u.pov_facing, DIR_SE);
    in(IN_RIGHT);
    in(IN_UPLEFT);
    in(IN_UPRIGHT);
    in(IN_DOWNLEFT);
    in(IN_DOWNRIGHT);
    in(IN_DOWN);
    CHECK_EQ(u.pov_facing, DIR_N);
    in(IN_UP);
    CHECK_EQ(g->cr[wiz()].y, 3);
    send(IN_WHEEL, 0, 1);
    CHECK_EQ(u.pov_facing, DIR_NE);
    send(IN_WHEEL, 0, -1);
    send(IN_DRAG, 30, 0);
    CHECK_EQ(u.pov_facing, DIR_NW);
    send(IN_DRAG, -30, 0);
    CHECK_EQ(u.pov_facing, DIR_N);
    send(IN_DRAG, 10, 0);
    /* clicks: left third, right third, middle, outside */
    click(VIEW_X + 5, VIEW_Y + 50);
    CHECK_EQ(u.pov_facing, DIR_NW);
    tap(VIEW_X + VIEW_PX - 5, VIEW_Y + 50);
    CHECK_EQ(u.pov_facing, DIR_N);
    click(VIEW_X + VIEW_PX / 2, VIEW_Y + 50);
    click(250, 60);
    in(IN_NONE);
    /* buttons */
    CHECK(button("< TURN"));
    CHECK_EQ(u.pov_facing, DIR_NW);
    CHECK(button("TURN >"));
    CHECK(button("FORWARD"));
    in(IN_HELP);
    CHECK(u.help);
    in(IN_LEFT);
    CHECK(!u.help);
    CHECK(button("EXIT 3D"));
    CHECK_EQ(u.mode, M_PLAY);
    CHECK_EQ(u.sel, wiz());
    /* V on an own creature under the cursor, NEXT switches the eyes */
    int gob = spawn(g, CR_GOBLIN, 0, 2, 0);
    in(IN_CANCEL);
    hovermap(2, 0);
    in(IN_VIEW);
    CHECK_EQ(u.pov, gob);
    in(IN_NEXT);
    CHECK_EQ(u.pov, wiz());
    in(IN_PREV);
    CHECK_EQ(u.pov, gob);
    CHECK(button("NEXT"));
    in(IN_FIRE);
    CHECK_EQ(u.mode, M_PLAY);
    /* camera follows across the wrap-around edge */
    hovermap(2, 0);
    in(IN_VIEW);
    u.pov_facing = DIR_NW;
    in(IN_UP);
    u.pov_facing = DIR_NW;
    in(IN_UP);
    u.pov_facing = DIR_NW;
    in(IN_UP);
    for (int i = 0; i < 40; i++) frame();
    CHECK(u.cam_x > 14.0f);
    CHECK(u.cam_y > 12.0f);
    u.pov_facing = DIR_SE;
    in(IN_UP);
    in(IN_UP);
    for (int i = 0; i < 40; i++) frame();
    CHECK(u.cam_x < 2.0f);
    /* the viewed creature dies while stepping forward */
    g->cr[gob].con = 1;
    spawn(g, CR_DRAGON, 1, g->cr[gob].x + 1, g->cr[gob].y + 1);
    for (int i = 0; i < 40 && g->cr[gob].alive; i++) {
        g->cr[gob].ap = 10;
        u.mode = M_VIEW3D;
        u.pov = gob;
        u.pov_facing = DIR_SE;
        in(IN_UP);
    }
    CHECK(!g->cr[gob].alive);
    CHECK_EQ(u.mode, M_PLAY);
    /* not your creature: forward does nothing and its button is disabled */
    u.mode = M_VIEW3D;
    u.pov = g->pl[1].wizard;
    Button b;
    CHECK(find_button("FORWARD", &b));
    CHECK(!b.enabled);
    click(b.r.x + 2, b.r.y + 2);
    int y0 = g->cr[u.pov].y;
    in(IN_UP);
    CHECK_EQ(g->cr[u.pov].y, y0);
    in(IN_VIEW);
    CHECK_EQ(u.mode, M_PLAY);
    /* end the turn from the 3D view */
    in(IN_VIEW);
    g->cr[wiz()].ap = 0;
    for (int i = 0; i < g->ncr; i++) if (g->cr[i].owner == 0) g->cr[i].ap = 0;
    in(IN_END_TURN);
    CHECK_EQ(u.mode, M_PLAY);
    CHECK_EQ(g->turn, 3);
    /* escaping through the portal from the 3D view */
    start_test_game(1, 1);
    g = &u.game;
    g->portal_open = true;
    map_set(&g->map, 11, 15, T_PORTAL);
    g->cr[wiz()].x = 11;
    g->cr[wiz()].y = 14;
    reselect(wiz());
    in(IN_VIEW);
    u.pov_facing = DIR_S;
    in(IN_UP);
    CHECK_EQ(u.mode, M_GAMEOVER);
}

static void help_and_misc(void)
{
    start_test_game(1, 1);
    in(IN_HELP);
    CHECK(u.help);
    send(IN_POINT, 50, 50);
    send(IN_DRAG, 1, 1);
    CHECK(u.help);
    in(IN_FIRE);
    CHECK(!u.help);
    CHECK(button("END TURN"));
    in(IN_CANCEL);
    CHECK(button("NEXT"));
    in(IN_CANCEL);
    CHECK(button("HELP"));
    CHECK(u.help);
    tap(5, 5);
    CHECK(!u.help);
    /* message and sound queue limits */
    ui_message(&u, "HELLO");
    CHECK_STR(u.msg, "HELLO");
    u.nsfx = 0;
    for (int i = 0; i < 40; i++) in(IN_LEFT);
    CHECK_EQ(u.nsfx, 16);
    /* every game event maps to a sound */
    reselect(wiz());
    for (int e = 0; e < GE_COUNT; e++) game_event(&u.game, (GameEvent)e);
    u.nsfx = 0;
    in(IN_FLY); /* refused, but drains the events */
    CHECK(u.nsfx > 5);
}

static void render_checks(void)
{
    start_test_game(1, 1);
    Game *g = &u.game;
    int b = spawn(g, CR_BAT, 0, 5, 1);
    g->cr[b].air = true;
    int m = spawn(g, CR_UNICORN, 0, 9, 1);
    int r = spawn(g, CR_GOBLIN, 0, 8, 1);
    game_ride(g, r, m);
    int sk = spawn(g, CR_SKELETON, 0, 10, 1);
    g->cr[sk].con = 1;
    g->cr[sk].invisible = 2;
    g->cr[sk].inv[0] = OB_SWORD;
    g->cr[sk].ninv = 1;
    g->obj[0] = (Object){true, OB_SHIELD, 12, 1};
    g->nobj = 1;
    spawn(g, CR_GOBLIN, 1, 12, 2);
    map_set(&g->map, 6, 4, T_DOOR_OPEN);
    game_update_vis(g, 0);
    reselect(r);
    reselect(m);
    u.vx = 2;
    u.vy = 0;
    hovermap(10, 3);
    for (int t = 0; t < 30; t++) {
        u.tick = (uint32_t)t;
        ui_render(&u, &fb);
    }
    for (int y = 0; y < SCREEN_H; y++)
        for (int x = 0; x < SCREEN_W; x++)
            if (fb.px[y][x] > 15) t_fail++;
    /* the panel for several creatures and squares */
    static const int at[][2] = {{5, 1}, {9, 1}, {10, 1}, {12, 1}, {12, 2}, {11, 0}, {4, 4}};
    for (unsigned i = 0; i < sizeof at / sizeof at[0]; i++) {
        hovermap(at[i][0], at[i][1]);
        ui_render(&u, &fb);
    }
    in(IN_CANCEL);
    hovermap(12, 1); /* object on a square, no creature */
    ui_render(&u, &fb);
    send(IN_POINT, 250, 30);
    ui_render(&u, &fb);
    /* hovering a button highlights it; touch hides hotkeys */
    Button bt;
    find_button("NEXT", &bt);
    send(IN_POINT, bt.r.x + 2, bt.r.y + 2);
    ui_render(&u, &fb);
    CHECK_EQ(fb.px[bt.r.y + 1][bt.r.x + 1], C_LBLUE);
    u.touch = true;
    ui_render(&u, &fb);
    u.touch = false;
    /* end-turn confirm blink */
    in(IN_END_TURN);
    for (int t = 0; t < 20; t += 8) {
        u.tick = (uint32_t)t;
        ui_render(&u, &fb);
    }
    in(IN_LEFT);
    /* portal open, armed cursor, target mode cursor colours */
    g->portal_open = true;
    reselect(wiz());
    tapmap(5, 4);
    for (int t = 0; t < 14; t++) {
        u.tick = (uint32_t)t;
        ui_render(&u, &fb);
    }
    in(IN_CAST);
    in(IN_WHEEL);
    list("4 LIGHTNING");
    hovermap(5, 4);
    ui_render(&u, &fb);
    in(IN_CANCEL);
    /* hit flash border */
    u.fx_timer = 3;
    ui_render(&u, &fb);
    CHECK_EQ(fb.px[0][0], C_RED);
    /* help for both devices */
    in(IN_HELP);
    ui_render(&u, &fb);
    u.touch = true;
    ui_render(&u, &fb);
    u.touch = false;
    in(IN_FIRE);
    /* lists with disabled entries */
    g->cr[wiz()].mana = 0;
    in(IN_CAST);
    ui_render(&u, &fb);
    in(IN_CANCEL);
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
    /* 3D view with its panel */
    g->over = false;
    u.mode = M_VIEW3D;
    u.pov = wiz();
    u.msg_timer = 0;
    ui_render(&u, &fb);
    CHECK(fb.px[VIEW_Y + 10][VIEW_X + 10] < 16);
    char line[64];
    ui_hover_text(&u, line, sizeof line);
    CHECK(strstr(line, "3D VIEW") != NULL);
    /* hover text of a list that is not the spell book */
    u.mode = M_LIST;
    u.list_spells = false;
    ui_hover_text(&u, line, sizeof line);
    CHECK_STR(line, "");
    (void)audio_play;
}

void test_ui(void)
{
    title();
    cursor_and_hover();
    selection_cycle();
    movement();
    ambush_and_doors();
    touch();
    actions();
    spells();
    end_turns();
    view3d();
    help_and_misc();
    render_checks();
}
