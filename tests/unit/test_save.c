#include "save.h"
#include "test.h"
#include "ui.h"

static Game g, g2, g3;
static uint8_t buf[SAVE_MAX_BYTES], bad[SAVE_MAX_BYTES];

static void fix_checksum(uint8_t *b, size_t n)
{
    uint32_t sum = save_checksum(b + 10, n - 14);
    for (int i = 0; i < 4; i++) b[n - 4 + i] = (uint8_t)(sum >> (8 * i));
}

static void round_trip(void)
{
    SaveMeta m = {1, 2, 0x1234567890ABCDEFull}, m2 = {0, 0, 0};
    game_init(&g, &SCENARIO_VALE, 1, 2, 42);
    /* make the state interesting: play some turns, carry things, ride */
    for (int i = 0; i < 6; i++) game_end_turn(&g);
    int w = g.pl[0].wizard;
    g.cr[w].inv[0] = OB_SWORD;
    g.cr[w].ninv = 1;
    g.cr[w].invisible = 2;
    map_set(&g.map, 8, 5, T_DOOR_OPEN);
    snprintf(g.msg, sizeof g.msg, "HELLO");
    size_t n = save_write(&g, &m, buf, sizeof buf);
    CHECK(n > 100);
    CHECK(n < SAVE_MAX_BYTES);
    CHECK(save_read(&g2, &m2, buf, n));
    CHECK_EQ(m2.humans, 1);
    CHECK_EQ(m2.ais, 2);
    CHECK(m2.seed == m.seed);
    CHECK_EQ(game_hash(&g2), game_hash(&g));
    CHECK_EQ(g2.rng.s, g.rng.s);
    CHECK_EQ(memcmp(&g2.map, &g.map, sizeof g.map), 0);
    CHECK_EQ(g2.cr[w].inv[0], OB_SWORD);
    CHECK_EQ(g2.cr[w].invisible, 2);
    CHECK_EQ(g2.nobj, g.nobj);
    CHECK_EQ(memcmp(g2.obj, g.obj, sizeof g.obj[0] * (size_t)g.nobj), 0);
    CHECK_STR(g2.msg, "HELLO");
    CHECK_STR(g2.pl[1].name, g.pl[1].name);
    CHECK_EQ(memcmp(g2.pl[0].seen, g.pl[0].seen, sizeof g.pl[0].seen), 0);
    CHECK_EQ(memcmp(g2.pl[0].vis, g.pl[0].vis, sizeof g.pl[0].vis), 0);
    CHECK_EQ(g2.portal_turn, g.portal_turn);
    /* the resumed game continues exactly like the original */
    for (int i = 0; i < 40 && !g.over; i++) {
        game_end_turn(&g);
        game_end_turn(&g2);
        CHECK_EQ(game_hash(&g2), game_hash(&g));
    }
    /* riders survive the trip */
    game_init(&g, &TEST_SCENARIO, 2, 0, 1);
    g.turn = 2;
    int u = game_spawn(&g, CR_UNICORN, 0, 4, 5);
    game_ride(&g, g.pl[0].wizard, u);
    n = save_write(&g, &m, buf, sizeof buf);
    CHECK(save_read(&g2, &m2, buf, n));
    CHECK_EQ(g2.cr[u].rider, g.pl[0].wizard);
    CHECK_EQ(g2.cr[g.pl[0].wizard].mount, u);
    /* too little room */
    CHECK_EQ(save_write(&g, &m, buf, 20), 0);
    CHECK_EQ(save_write(&g, &m, buf, n - 1), 0);
}

static void rejection(void)
{
    SaveMeta m = {1, 1, 7}, m2;
    game_init(&g, &TEST_SCENARIO, 1, 1, 3);
    size_t n = save_write(&g, &m, buf, sizeof buf);
    g3 = g2;
    /* every single flipped byte is detected (header, body or checksum) */
    int accepted = 0;
    for (size_t i = 0; i < n; i++) {
        memcpy(bad, buf, n);
        bad[i] ^= 0x5A;
        accepted += save_read(&g2, &m2, bad, n);
    }
    CHECK_EQ(accepted, 0);
    CHECK_EQ(memcmp(&g2, &g3, sizeof g2), 0); /* untouched on failure */
    /* truncation and padding */
    CHECK(!save_read(&g2, &m2, buf, n - 1));
    CHECK(!save_read(&g2, &m2, buf, 5));
    memcpy(bad, buf, n);
    bad[n] = 0;
    CHECK(!save_read(&g2, &m2, bad, n + 1));
    /* other versions */
    memcpy(bad, buf, n);
    bad[4] = SAVE_VERSION + 1;
    CHECK(!save_read(&g2, &m2, bad, n));
    /* a valid checksum does not excuse impossible values */
    static const struct { int what; } cases[] = {{0}, {1}, {2}, {3}, {4}, {5}, {6}};
    for (unsigned k = 0; k < sizeof cases / sizeof cases[0]; k++) {
        game_init(&g, &TEST_SCENARIO, 1, 1, 3);
        switch (cases[k].what) {
        case 0: g.cr[1].type = 99; break;
        case 1: g.cr[1].x = 40; break;
        case 2: g.map.t[0][0] = 99; break;
        case 3: g.cr[1].ninv = 9; break;
        case 4: g.obj[0] = (Object){true, 9, 0, 0}; g.nobj = 1; break;
        case 5: g.pl[1].wizard = 150; break;
        default: g.cr[0].rider = 120; break;
        }
        size_t len = save_write(&g, &m, bad, sizeof bad);
        if (save_read(&g2, &m2, bad, len)) {
            printf("bad value case %d accepted\n", cases[k].what);
            t_fail++;
        }
    }
    /* a structurally broken body with a valid checksum (extra trailing byte) */
    game_init(&g, &TEST_SCENARIO, 1, 1, 3);
    n = save_write(&g, &m, bad, sizeof bad);
    memmove(bad + n - 3, bad + n - 4, 4); /* insert one byte before the checksum */
    bad[n - 4] = 0;
    size_t len = n + 1;
    uint32_t body = (uint32_t)(len - 14);
    for (int i = 0; i < 4; i++) bad[6 + i] = (uint8_t)(body >> (8 * i));
    fix_checksum(bad, len);
    CHECK(!save_read(&g2, &m2, bad, len));
    /* a body that ends early with a valid checksum */
    n = save_write(&g, &m, bad, sizeof bad);
    len = 40;
    memmove(bad + len - 4, bad + n - 4, 4);
    body = (uint32_t)(len - 14);
    for (int i = 0; i < 4; i++) bad[6 + i] = (uint8_t)(body >> (8 * i));
    fix_checksum(bad, len);
    CHECK(!save_read(&g2, &m2, bad, len));
    /* a huge map size is refused before reading the map */
    n = save_write(&g, &m, bad, sizeof bad);
    bad[20] = 200;
    fix_checksum(bad, n);
    CHECK(!save_read(&g2, &m2, bad, n));
}

static void ui_flow(void)
{
    static Ui u;
    static Framebuffer fb;
    ui_init(&u, 4);
    CHECK_EQ(u.saved_len, 0);
    CHECK_EQ(u.menu.n, 3);
    /* starting a game writes a save at the first human turn */
    u.menu.sel = 2;
    ui_input(&u, (Input){IN_FIRE, 0, 0});
    CHECK(u.saved_len > 0);
    CHECK_EQ(ui_take_save_request(&u), SAVE_WRITE);
    CHECK_EQ(ui_take_save_request(&u), SAVE_NONE);
    size_t n = u.saved_len;
    memcpy(buf, u.saved, n);
    uint32_t hash = game_hash(&u.game);
    /* invalid and finished games are not offered */
    ui_init(&u, 4);
    CHECK(!ui_offer_save(&u, buf, 3));
    CHECK(!ui_offer_save(&u, buf, SAVE_MAX_BYTES + 1));
    game_init(&g, &TEST_SCENARIO, 1, 1, 1);
    g.over = true;
    SaveMeta m = {1, 1, 1};
    size_t over_n = save_write(&g, &m, bad, sizeof bad);
    CHECK(!ui_offer_save(&u, bad, over_n));
    CHECK_EQ(u.menu.n, 3);
    /* a valid save adds CONTINUE as the highlighted first entry */
    CHECK(ui_offer_save(&u, buf, n));
    CHECK_EQ(u.menu.n, 4);
    CHECK_STR(u.menu.label[0], "CONTINUE");
    CHECK_EQ(u.menu.sel, 0);
    ui_render(&u, &fb);
    Rect r = ui_menu_item_rect(&u, 0);
    CHECK_EQ(r.y, 132 - MENU_ROW);
    ui_input(&u, (Input){IN_FIRE, 0, 0});
    CHECK_EQ(u.mode, M_PLAY);
    CHECK_EQ(game_hash(&u.game), hash);
    CHECK_STR(u.msg, "WELCOME BACK");
    /* a corrupted copy in memory cannot be continued */
    ui_init(&u, 4);
    CHECK(ui_offer_save(&u, buf, n));
    u.saved[20] ^= 1;
    ui_input(&u, (Input){IN_FIRE, 0, 0});
    CHECK_EQ(u.mode, M_TITLE);
    /* offering while not on the title keeps the menu */
    ui_init(&u, 4);
    u.mode = M_PLAY;
    CHECK(ui_offer_save(&u, buf, n));
    CHECK_EQ(u.menu.n, 3);
    /* the game menu: Escape with nothing selected, RESUME, HELP, QUIT TO TITLE */
    ui_init(&u, 4);
    CHECK(ui_offer_save(&u, buf, n));
    ui_input(&u, (Input){IN_FIRE, 0, 0});
    ui_input(&u, (Input){IN_CANCEL, 0, 0}); /* deselect */
    ui_input(&u, (Input){IN_CANCEL, 0, 0}); /* menu */
    CHECK_EQ(u.mode, M_LIST);
    CHECK_STR(u.menu.title, "GAME MENU");
    ui_input(&u, (Input){IN_FIRE, 0, 0}); /* RESUME */
    CHECK_EQ(u.mode, M_PLAY);
    Button b[MAX_BUTTONS];
    int nb = ui_buttons(&u, b);
    CHECK_STR(b[0].label, "MENU");
    CHECK_STR(b[1].label, "HELP");
    (void)nb;
    ui_input(&u, (Input){IN_CLICK, b[0].r.x + 2, b[0].r.y + 2});
    CHECK_EQ(u.mode, M_LIST);
    ui_input(&u, (Input){IN_DOWN, 0, 0});
    ui_input(&u, (Input){IN_FIRE, 0, 0}); /* HELP */
    CHECK(u.help);
    ui_input(&u, (Input){IN_FIRE, 0, 0});
    ui_input(&u, (Input){IN_CANCEL, 0, 0});
    ui_input(&u, (Input){IN_UP, 0, 0}); /* QUIT TO TITLE (wraps) */
    ui_input(&u, (Input){IN_FIRE, 0, 0});
    CHECK_EQ(u.mode, M_TITLE);
    CHECK_STR(u.menu.label[0], "CONTINUE");
    /* game over clears the save */
    ui_input(&u, (Input){IN_FIRE, 0, 0});
    game_kill(&u.game, u.game.pl[0].wizard, 1);
    ui_input(&u, (Input){IN_END_TURN, 0, 0});
    CHECK_EQ(u.mode, M_GAMEOVER);
    CHECK_EQ(u.saved_len, 0);
    CHECK_EQ(ui_take_save_request(&u), SAVE_CLEAR);
    /* a hot-seat game resumes behind the hand-over screen */
    ui_init(&u, 4);
    u.humans = 2;
    u.ais = 0;
    ui_start_game(&u);
    n = u.saved_len;
    memcpy(buf, u.saved, n);
    ui_init(&u, 9);
    CHECK(ui_offer_save(&u, buf, n));
    ui_input(&u, (Input){IN_FIRE, 0, 0});
    CHECK_EQ(u.mode, M_HANDOVER);
    CHECK_EQ(u.humans, 2);
    CHECK_EQ(u.ais, 0);
}

void test_save(void)
{
    round_trip();
    rejection();
    ui_flow();
}
