#include "script.h"
#include "test.h"

static Ui u;
static Framebuffer fb;

static bool run(const char *text)
{
    ui_init(&u, 1);
    Script s = {&u, &fb, NULL, 0, "", 0};
    bool ok = script_run(&s, text);
    if (!ok) printf("script error line %d: %s\n", s.line, s.error);
    return ok;
}

static bool fails(const char *text)
{
    ui_init(&u, 1);
    Script s = {&u, &fb, NULL, 0, "", 0};
    return !script_run(&s, text) && s.error[0];
}

void test_script(void)
{
    CHECK(run(""));
    CHECK(run("# only a comment\n\n   \n"));
    CHECK(run("seed 11\nplayers 1 1\nexpect mode TITLE\nmenu START GAME\nexpect mode CURSOR\n"
              "expect turn == 1\nexpect player == 0\nexpect over 0\nexpect winner -1\n"
              "expect creatures 0 == 1\nexpect mana 0 >= 60\nexpect cursor 5 5\n"
              "expect wizard_at 0 5 5\nexpect terrain 2 2 WALL\nexpect palette_only\n"
              "expect colours >= 4\nexpect selected none\nkey FIRE\nmenu SELECT-G\n"
              "expect selected WIZARD\nexpect ap == 14\nexpect ap != 3\nexpect ap > 1\nexpect ap < 99\n"
              "expect ap <= 14\nkey RIGHT 2\nkey LEFT 2\ntapmap 5 5\nexpect mode MENU\n"
              "key CANCEL\ntap 10 10\ntick 3\nexpect pixel 0 0 6\nclearsound\nkey FIRE\n"
              "expect sound 1\nscreenshot ignored\n"));
    CHECK(run("menu START GAME\nexpect msg TURN 1\n"));
    CHECK(run("players 2 0\nmenu START GAME\nkey FIRE\nendturn\nexpect player == 1\nexpect mode CURSOR\n"));
    CHECK(run("menu START GAME\nkey FIRE\nmenu SELECT-G\nendturn\nexpect turn == 2\n"));
    CHECK(run("seed 3\nmenu START GAME\nrepeat 40 endturn\nexpect mode GAMEOVER\n"));
    CHECK(fails("menu START GAME\nkey FIRE\nendturn\n"));
    CHECK(fails("menu START GAME\nrepeat 2 bogus\n"));
    /* errors */
    CHECK(fails("bogus\n"));
    CHECK(fails("key SIDEWAYS\n"));
    CHECK(fails("menu START GAME\nmenu NOPE\n"));
    CHECK(fails("menu START GAME\nkey CANCEL\nmenu START\n"));
    CHECK(fails("menu\n"));
    CHECK(fails("expect\n"));
    CHECK(fails("expect mode CURSOR\n"));
    CHECK(fails("expect turn == 5\n"));
    CHECK(fails("expect turn ~ 5\n"));
    CHECK(fails("expect mana 7 == 1\n"));
    CHECK(fails("expect wizard_at 7 1 1\n"));
    CHECK(fails("menu START GAME\nexpect wizard_at 0 1 1\n"));
    CHECK(fails("menu START GAME\nexpect ap == 1\n"));
    CHECK(fails("menu START GAME\nexpect selected WIZARD\n"));
    CHECK(fails("menu START GAME\nexpect selected TROLL\n"));
    CHECK(fails("menu START GAME\nkey FIRE\nmenu SELECT-G\nexpect selected none\n"));
    CHECK(fails("expect cursor 1 1\n"));
    CHECK(fails("menu START GAME\nexpect terrain 2 2 GRASS\n"));
    CHECK(fails("expect msg NOTHING LIKE THIS\n"));
    CHECK(fails("expect pixel -1 0 0\n"));
    CHECK(fails("expect pixel 0 0 9\n"));
    CHECK(fails("expect sound 99\n"));
    CHECK(fails("expect something\n"));
    CHECK(fails("menu START GAME\ntapmap 20 20\n"));
    fb.px[0][0] = 77;
    ui_init(&u, 1);
    Script s = {&u, &fb, NULL, 0, "", 0};
    CHECK(script_exec_line(&s, "expect colours > 0"));
    fb.px[3][3] = 77;
    CHECK(!script_exec_line(&s, "expect palette_only"));
    /* screenshots */
    Script shot = {&u, &fb, ".", 0, "", 0};
    CHECK(script_exec_line(&shot, "screenshot unit_test_shot"));
    remove("unit_test_shot.ppm");
    Script bad = {&u, &fb, "/nonexistent-dir", 0, "", 0};
    CHECK(!script_exec_line(&bad, "screenshot x"));
    /* overly long lines are truncated, not overflowed */
    char longline[600];
    memset(longline, 'a', sizeof longline - 1);
    longline[sizeof longline - 1] = 0;
    CHECK(fails(longline));
}
