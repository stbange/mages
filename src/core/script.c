#include "script.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool err(Script *s, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(s->error, sizeof s->error, fmt, ap);
    va_end(ap);
    return false;
}

static const struct { const char *name; InputKind kind; } KEYS[] = {
    {"UP", IN_UP}, {"DOWN", IN_DOWN}, {"LEFT", IN_LEFT}, {"RIGHT", IN_RIGHT},
    {"UL", IN_UPLEFT}, {"UR", IN_UPRIGHT}, {"DL", IN_DOWNLEFT}, {"DR", IN_DOWNRIGHT},
    {"FIRE", IN_FIRE}, {"CANCEL", IN_CANCEL}, {"NEXT", IN_NEXT}, {"PREV", IN_PREV}, {"VIEW", IN_VIEW},
    {"CAST", IN_CAST}, {"FLY", IN_FLY}, {"PICKUP", IN_PICKUP}, {"DROP", IN_DROP}, {"DRINK", IN_DRINK},
    {"USE", IN_USE}, {"RIDE", IN_RIDE}, {"ENDTURN", IN_END_TURN}, {"HELP", IN_HELP},
};

static bool compare(int a, const char *op, int b)
{
    if (!strcmp(op, "==")) return a == b;
    if (!strcmp(op, "!=")) return a != b;
    if (!strcmp(op, ">=")) return a >= b;
    if (!strcmp(op, "<=")) return a <= b;
    if (!strcmp(op, ">")) return a > b;
    if (!strcmp(op, "<")) return a < b;
    return false;
}

static bool valid_op(const char *op)
{
    static const char *const ops[] = {"==", "!=", ">=", "<=", ">", "<"};
    for (size_t i = 0; i < sizeof ops / sizeof ops[0]; i++)
        if (!strcmp(op, ops[i])) return true;
    return false;
}

static void frame(Script *s)
{
    ui_tick(s->ui);
    ui_render(s->ui, s->fb);
}

static void input(Script *s, InputKind k, int x, int y)
{
    ui_input(s->ui, (Input){k, x, y});
    frame(s);
}

static int creature_type_by_name(const char *name)
{
    for (int i = 0; i < CR_COUNT; i++)
        if (!strcmp(CREATURES[i].name, name)) return i;
    return -1;
}

static bool expect_numeric(Script *s, const char *what, int value, const char *op, const char *num)
{
    if (!valid_op(op)) return err(s, "bad operator '%s'", op);
    int b = atoi(num);
    if (!compare(value, op, b)) return err(s, "expected %s %s %d but was %d", what, op, b, value);
    return true;
}

static bool cmd_expect(Script *s, int argc, char **argv)
{
    Ui *u = s->ui;
    Game *g = &u->game;
    if (argc < 2) return err(s, "expect needs arguments");
    const char *w = argv[1];
    if (!strcmp(w, "mode") && argc == 3) {
        if (strcmp(ui_mode_name(u->mode), argv[2]) != 0)
            return err(s, "expected mode %s but was %s", argv[2], ui_mode_name(u->mode));
        return true;
    }
    if (!strcmp(w, "turn") && argc == 4) return expect_numeric(s, "turn", g->turn, argv[2], argv[3]);
    if (!strcmp(w, "player") && argc == 4) return expect_numeric(s, "player", g->cur, argv[2], argv[3]);
    if (!strcmp(w, "at") && argc == 4) {
        if (u->sel == NONE) return err(s, "nothing selected");
        const Creature *c = &g->cr[u->sel];
        if (c->x != atoi(argv[2]) || c->y != atoi(argv[3]))
            return err(s, "expected selection at %s,%s but was %d,%d", argv[2], argv[3], c->x, c->y);
        return true;
    }
    if (!strcmp(w, "reach") && argc == 6)
        return expect_numeric(s, "reach", ui_reach_cost(u, atoi(argv[2]), atoi(argv[3])), argv[4], argv[5]);
    if (!strcmp(w, "help") && argc == 3) return expect_numeric(s, "help", u->help, "==", argv[2]);
    if (!strcmp(w, "confirm") && argc == 3) return expect_numeric(s, "confirm", u->end_confirm > 0, "==", argv[2]);
    if (!strcmp(w, "view") && argc == 4) {
        if (u->vx != atoi(argv[2]) || u->vy != atoi(argv[3]))
            return err(s, "expected view %s,%s but was %d,%d", argv[2], argv[3], u->vx, u->vy);
        return true;
    }
    if (!strcmp(w, "over") && argc == 3) return expect_numeric(s, "over", g->over, "==", argv[2]);
    if (!strcmp(w, "winner") && argc == 3) return expect_numeric(s, "winner", g->winner, "==", argv[2]);
    if (!strcmp(w, "creatures") && argc == 5)
        return expect_numeric(s, "creatures", game_count_creatures(g, atoi(argv[2])), argv[3], argv[4]);
    if (!strcmp(w, "mana") && argc == 5) {
        int p = atoi(argv[2]);
        if (p < 0 || p >= g->npl) return err(s, "no player %d", p);
        return expect_numeric(s, "mana", g->cr[g->pl[p].wizard].mana, argv[3], argv[4]);
    }
    if (!strcmp(w, "ap") && argc == 4) {
        if (u->sel == NONE) return err(s, "nothing selected");
        return expect_numeric(s, "ap", g->cr[u->sel].ap, argv[2], argv[3]);
    }
    if (!strcmp(w, "selected") && argc == 3) {
        if (!strcmp(argv[2], "none")) {
            if (u->sel != NONE) return err(s, "expected no selection");
            return true;
        }
        int t = creature_type_by_name(argv[2]);
        if (u->sel == NONE || g->cr[u->sel].type != t) return err(s, "expected %s selected", argv[2]);
        return true;
    }
    if (!strcmp(w, "cursor") && argc == 4) {
        if (u->cx != atoi(argv[2]) || u->cy != atoi(argv[3]))
            return err(s, "expected cursor %s,%s but was %d,%d", argv[2], argv[3], u->cx, u->cy);
        return true;
    }
    if (!strcmp(w, "wizard_at") && argc == 5) {
        int p = atoi(argv[2]);
        if (p < 0 || p >= g->npl) return err(s, "no player %d", p);
        const Creature *c = &g->cr[g->pl[p].wizard];
        if (c->x != atoi(argv[3]) || c->y != atoi(argv[4]))
            return err(s, "expected wizard %d at %s,%s but was %d,%d", p, argv[3], argv[4], c->x, c->y);
        return true;
    }
    if (!strcmp(w, "terrain") && argc >= 5) {
        char want[32] = "";
        for (int i = 4; i < argc; i++) {
            if (i > 4) strncat(want, " ", sizeof want - strlen(want) - 1);
            strncat(want, argv[i], sizeof want - strlen(want) - 1);
        }
        Terrain t = map_get(&g->map, atoi(argv[2]), atoi(argv[3]));
        if (strcmp(TERRAIN[t].name, want) != 0)
            return err(s, "expected terrain %s but was %s", want, TERRAIN[t].name);
        return true;
    }
    if (!strcmp(w, "msg") && argc >= 3) {
        char want[64] = "";
        for (int i = 2; i < argc; i++) {
            if (i > 2) strncat(want, " ", sizeof want - strlen(want) - 1);
            strncat(want, argv[i], sizeof want - strlen(want) - 1);
        }
        if (!strstr(u->msg, want)) return err(s, "expected message '%s' but was '%s'", want, u->msg);
        return true;
    }
    if (!strcmp(w, "pixel") && argc == 5) {
        int x = atoi(argv[2]), y = atoi(argv[3]);
        if (x < 0 || y < 0 || x >= SCREEN_W || y >= SCREEN_H) return err(s, "pixel out of screen");
        return expect_numeric(s, "pixel", s->fb->px[y][x], "==", argv[4]);
    }
    if (!strcmp(w, "palette_only") && argc == 2) {
        for (int y = 0; y < SCREEN_H; y++)
            for (int x = 0; x < SCREEN_W; x++)
                if (s->fb->px[y][x] > 15) return err(s, "pixel %d,%d not in palette", x, y);
        return true;
    }
    if (!strcmp(w, "colours") && argc == 4) {
        int seen[16] = {0}, n = 0;
        for (int y = 0; y < SCREEN_H; y++)
            for (int x = 0; x < SCREEN_W; x++)
                if (!seen[s->fb->px[y][x] & 15]++) n++;
        return expect_numeric(s, "colours", n, argv[2], argv[3]);
    }
    if (!strcmp(w, "sound") && argc == 3) {
        int want = atoi(argv[2]);
        for (int i = 0; i < u->nsfx; i++)
            if (u->sfx[i] == want) return true;
        return err(s, "sound %d not queued", want);
    }
    return err(s, "unknown expectation '%s'", w);
}

static bool cmd_menu(Script *s, int argc, char **argv)
{
    Ui *u = s->ui;
    if (argc < 2) return err(s, "menu needs a label");
    char want[32] = "";
    for (int i = 1; i < argc; i++) {
        if (i > 1) strncat(want, " ", sizeof want - strlen(want) - 1);
        strncat(want, argv[i], sizeof want - strlen(want) - 1);
    }
    if (u->mode != M_LIST && u->mode != M_TITLE) return err(s, "no list open");
    for (int i = 0; i < u->menu.n; i++) {
        if (strncmp(u->menu.label[i], want, strlen(want)) == 0) {
            Rect r = ui_menu_item_rect(u, i);
            input(s, IN_TAP, r.x + r.w / 2, r.y + r.h / 2);
            return true;
        }
    }
    return err(s, "menu has no entry '%s'", want);
}

static void settle(Script *s)
{
    for (int i = 0; i < 1000 && ui_walking(s->ui); i++) frame(s);
}

static bool cmd_button(Script *s, int argc, char **argv)
{
    if (argc < 2) return err(s, "button needs a label");
    char want[32] = "";
    for (int i = 1; i < argc; i++) {
        if (i > 1) strncat(want, " ", sizeof want - strlen(want) - 1);
        strncat(want, argv[i], sizeof want - strlen(want) - 1);
    }
    Button b[MAX_BUTTONS];
    int n = ui_buttons(s->ui, b);
    for (int i = 0; i < n; i++) {
        if (strncmp(b[i].label, want, strlen(want)) == 0) {
            input(s, strcmp(argv[0], "tapbutton") ? IN_CLICK : IN_TAP, b[i].r.x + b[i].r.w / 2, b[i].r.y + b[i].r.h / 2);
            return true;
        }
    }
    return err(s, "no button '%s'", want);
}

bool script_exec_line(Script *s, const char *line)
{
    char buf[256];
    snprintf(buf, sizeof buf, "%s", line);
    char *hash = strchr(buf, '#');
    if (hash) *hash = 0;
    char *argv[16];
    int argc = 0;
    for (char *tok = strtok(buf, " \t\r\n"); tok && argc < 16; tok = strtok(NULL, " \t\r\n")) argv[argc++] = tok;
    if (argc == 0) return true;
    s->commands++;
    Ui *u = s->ui;
    const char *c = argv[0];
    if (!strcmp(c, "seed") && argc == 2) {
        u->seed = strtoull(argv[1], NULL, 10);
        return true;
    }
    if (!strcmp(c, "players") && argc == 3) {
        u->humans = atoi(argv[1]);
        u->ais = atoi(argv[2]);
        return true;
    }
    if (!strcmp(c, "key") && (argc == 2 || argc == 3)) {
        int n = argc == 3 ? atoi(argv[2]) : 1;
        for (size_t k = 0; k < sizeof KEYS / sizeof KEYS[0]; k++) {
            if (!strcmp(KEYS[k].name, argv[1])) {
                for (int i = 0; i < n; i++) input(s, KEYS[k].kind, 0, 0);
                return true;
            }
        }
        return err(s, "unknown key '%s'", argv[1]);
    }
    if ((!strcmp(c, "click") || !strcmp(c, "hover")) && argc == 3) {
        input(s, c[0] == 'c' ? IN_CLICK : IN_POINT, atoi(argv[1]), atoi(argv[2]));
        return true;
    }
    if ((!strcmp(c, "clickmap") || !strcmp(c, "hovermap")) && argc == 3) {
        int sx, sy;
        if (!ui_map_to_screen(u, atoi(argv[1]), atoi(argv[2]), &sx, &sy)) return err(s, "square not in view");
        input(s, c[0] == 'c' ? IN_CLICK : IN_POINT, sx, sy);
        return true;
    }
    if (!strcmp(c, "drag") && argc == 3) {
        /* deliver the drag in small steps like a real pointer */
        int dx = atoi(argv[1]), dy = atoi(argv[2]);
        for (int i = 0; i < 4; i++) input(s, IN_DRAG, dx / 4, dy / 4);
        return true;
    }
    if (!strcmp(c, "wheel") && argc == 2) {
        input(s, IN_WHEEL, 0, atoi(argv[1]));
        return true;
    }
    if (!strcmp(c, "digit") && argc == 2) {
        input(s, IN_DIGIT, atoi(argv[1]), 0);
        return true;
    }
    if (!strcmp(c, "button") || !strcmp(c, "tapbutton")) return cmd_button(s, argc, argv);
    if (!strcmp(c, "settle") && argc == 1) {
        settle(s);
        return true;
    }
    if (!strcmp(c, "tap") && argc == 3) {
        input(s, IN_TAP, atoi(argv[1]), atoi(argv[2]));
        return true;
    }
    if (!strcmp(c, "tapmap") && argc == 3) {
        int sx, sy;
        if (!ui_map_to_screen(u, atoi(argv[1]), atoi(argv[2]), &sx, &sy)) return err(s, "square not in view");
        input(s, IN_TAP, sx, sy);
        return true;
    }
    if (!strcmp(c, "tick") && argc == 2) {
        for (int i = atoi(argv[1]); i > 0; i--) frame(s);
        return true;
    }
    if (!strcmp(c, "clearsound") && argc == 1) {
        u->nsfx = 0;
        return true;
    }
    if (!strcmp(c, "repeat") && argc >= 3) {
        /* repeat N command...: stops early once the game is over */
        char rest[256] = "";
        for (int i = 2; i < argc; i++) {
            if (i > 2) strncat(rest, " ", sizeof rest - strlen(rest) - 1);
            strncat(rest, argv[i], sizeof rest - strlen(rest) - 1);
        }
        for (int i = atoi(argv[1]); i > 0 && u->mode != M_GAMEOVER; i--) {
            if (!script_exec_line(s, rest)) return false;
        }
        return true;
    }
    if (!strcmp(c, "endturn") && argc == 1) {
        /* like a player: press END TURN, confirm if asked, pass the hand-over screen */
        if (u->mode == M_LIST || u->mode == M_TARGET) input(s, IN_CANCEL, 0, 0);
        if (u->mode != M_PLAY && u->mode != M_VIEW3D)
            return err(s, "endturn needs the map (mode %s)", ui_mode_name(u->mode));
        int turn = u->game.turn, cur = u->game.cur;
        char *a1[] = {"button", "END"}, *a2[] = {"button", "CONFIRM"};
        if (!cmd_button(s, 2, a1)) return false;
        if (u->end_confirm > 0 && !cmd_button(s, 2, a2)) return false;
        if (u->mode == M_HANDOVER) input(s, IN_FIRE, 0, 0);
        if (u->mode != M_GAMEOVER && u->game.turn == turn && u->game.cur == cur) return err(s, "turn did not end");
        return true;
    }
    if (!strcmp(c, "menu")) return cmd_menu(s, argc, argv);
    if (!strcmp(c, "expect")) {
        settle(s);
        return cmd_expect(s, argc, argv);
    }
    if (!strcmp(c, "screenshot") && argc == 2) {
        if (!s->shot_dir) return true;
        char path[512];
        snprintf(path, sizeof path, "%s/%s.ppm", s->shot_dir, argv[1]);
        if (!fb_write_ppm(s->fb, path)) return err(s, "cannot write %s", path);
        return true;
    }
    return err(s, "unknown command '%s'", c);
}

bool script_run(Script *s, const char *text)
{
    s->line = 0;
    s->error[0] = 0;
    s->commands = 0;
    frame(s);
    const char *p = text;
    while (*p) {
        const char *e = strchr(p, '\n');
        size_t n = e ? (size_t)(e - p) : strlen(p);
        char line[256];
        if (n >= sizeof line) n = sizeof line - 1;
        memcpy(line, p, n);
        line[n] = 0;
        s->line++;
        if (!script_exec_line(s, line)) return false;
        p = e ? e + 1 : p + n;
    }
    return true;
}
