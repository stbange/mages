#include "ui.h"
#include "audio.h"
#include "raycast.h"
#include <stdio.h>
#include <string.h>

static const char *const MODE_NAMES[M_COUNT] = {
    "TITLE", "HANDOVER", "CURSOR", "SELECTED", "MENU", "SPELLS", "TARGET", "VIEW3D", "GAMEOVER",
};

const char *ui_mode_name(Mode m) { return MODE_NAMES[m]; }

static void sfx(Ui *u, int s)
{
    if (u->nsfx < (int)(sizeof u->sfx / sizeof u->sfx[0])) u->sfx[u->nsfx++] = s;
}

int ui_pop_sfx(Ui *u)
{
    if (u->nsfx == 0) return -1;
    int s = u->sfx[0];
    memmove(u->sfx, u->sfx + 1, sizeof(int) * (size_t)(u->nsfx - 1));
    u->nsfx--;
    return s;
}

void ui_message(Ui *u, const char *m)
{
    snprintf(u->msg, sizeof u->msg, "%s", m);
    u->msg_timer = 150;
}

static const uint8_t EVENT_SFX[GE_COUNT] = {
    [GE_STEP] = SFX_STEP, [GE_HIT] = SFX_HIT, [GE_MISS] = SFX_MISS, [GE_DEATH] = SFX_DEATH,
    [GE_CAST] = SFX_CAST, [GE_SUMMON] = SFX_SUMMON, [GE_DOOR] = SFX_DOOR, [GE_PORTAL] = SFX_PORTAL,
    [GE_TURN] = SFX_TURN, [GE_PICKUP] = SFX_PICKUP, [GE_DRINK] = SFX_PICKUP, [GE_FLY] = SFX_FLY,
    [GE_ERROR] = SFX_ERROR, [GE_WIN] = SFX_WIN,
};

/* Drain game events into sounds and show the game's message. */
static void sync_game(Ui *u)
{
    int e;
    bool any = false;
    while ((e = game_pop_event(&u->game)) >= 0) {
        sfx(u, EVENT_SFX[e]);
        if (e == GE_HIT) u->fx_timer = 6;
        any = true;
    }
    if (any) ui_message(u, u->game.msg);
}

int ui_viewer(const Ui *u)
{
    if (u->mode == M_GAMEOVER || u->game.over) return NONE;
    return u->game.cur;
}

/* ------------------------------------------------------------ view/cursor */

static void centre_view(Ui *u, int x, int y)
{
    u->vx = u->game.map.w ? map_wx(&u->game.map, x - VIEW_TILES / 2) : 0;
    u->vy = u->game.map.h ? map_wy(&u->game.map, y - VIEW_TILES / 2) : 0;
}

static void keep_cursor_visible(Ui *u)
{
    const Map *m = &u->game.map;
    int rx = map_wx(m, u->cx - u->vx), ry = map_wy(m, u->cy - u->vy);
    if (rx < 1 || rx > VIEW_TILES - 2) {
        u->vx = map_wx(m, rx > m->w / 2 || rx < 1 ? u->cx - 1 : u->cx - (VIEW_TILES - 2));
    }
    if (ry < 1 || ry > VIEW_TILES - 2) {
        u->vy = map_wy(m, ry > m->h / 2 || ry < 1 ? u->cy - 1 : u->cy - (VIEW_TILES - 2));
    }
}

static void set_cursor(Ui *u, int x, int y)
{
    u->cx = map_wx(&u->game.map, x);
    u->cy = map_wy(&u->game.map, y);
    keep_cursor_visible(u);
}

bool ui_map_to_screen(const Ui *u, int mx, int my, int *sx, int *sy)
{
    const Map *m = &u->game.map;
    int rx = map_wx(m, mx - u->vx), ry = map_wy(m, my - u->vy);
    if (rx >= VIEW_TILES || ry >= VIEW_TILES) return false;
    *sx = VIEW_X + rx * 16 + 8;
    *sy = VIEW_Y + ry * 16 + 8;
    return true;
}

static int dir_of_input(InputKind k)
{
    switch (k) {
    case IN_UP: return DIR_N;
    case IN_UPRIGHT: return DIR_NE;
    case IN_RIGHT: return DIR_E;
    case IN_DOWNRIGHT: return DIR_SE;
    case IN_DOWN: return DIR_S;
    case IN_DOWNLEFT: return DIR_SW;
    case IN_LEFT: return DIR_W;
    case IN_UPLEFT: return DIR_NW;
    default: return -1;
    }
}

/* The creature to act with for the current selection (riders act via mount). */
static int sel_valid(Ui *u)
{
    if (u->sel == NONE) return 0;
    const Creature *c = &u->game.cr[u->sel];
    if (!c->alive || c->owner != u->game.cur || c->mount != NONE) {
        u->sel = NONE;
        return 0;
    }
    return 1;
}

static int own_creature_at_cursor(const Ui *u, bool air)
{
    int c = game_creature_at(&u->game, u->cx, u->cy, air);
    return (c != NONE && u->game.cr[c].owner == u->game.cur) ? c : NONE;
}

static void focus_player(Ui *u)
{
    const Game *g = &u->game;
    if (g->cur < 0) return;
    const Creature *w = &g->cr[g->pl[g->cur].wizard];
    int x = w->x, y = w->y;
    u->cx = x;
    u->cy = y;
    centre_view(u, x, y);
}

/* ------------------------------------------------------------------ menus */

static void menu_begin(Ui *u, const char *title, Mode from)
{
    memset(&u->menu, 0, sizeof u->menu);
    snprintf(u->menu.title, sizeof u->menu.title, "%s", title);
    u->menu_from = from;
}

static void menu_add(Ui *u, const char *label, Action a, int arg, bool disabled)
{
    Menu *m = &u->menu;
    if (m->n >= MENU_MAX) return;
    snprintf(m->label[m->n], sizeof m->label[0], "%s", label);
    m->act[m->n] = (uint8_t)a;
    m->arg[m->n] = (uint8_t)arg;
    m->disabled[m->n] = disabled;
    m->n++;
}

Rect ui_menu_item_rect(const Ui *u, int i)
{
    if (u->mode == M_TITLE) {
        Rect t = {190, 132 + i * MENU_ROW, 118, MENU_ROW};
        return t;
    }
    Rect r = {PANEL_X + 2, MENU_Y + i * MENU_ROW, SCREEN_W - PANEL_X - 6, MENU_ROW};
    return r;
}

static void title_menu(Ui *u)
{
    char a[22], b[22];
    menu_begin(u, "", M_TITLE);
    snprintf(a, sizeof a, "PLAYERS   %d", u->humans);
    snprintf(b, sizeof b, "COMPUTER  %d", u->ais);
    menu_add(u, a, A_HUMANS, 0, false);
    menu_add(u, b, A_AIS, 0, false);
    menu_add(u, "START GAME", A_START, 0, false);
    u->menu.sel = 2;
}

static bool has_adjacent(const Ui *u, int ci, int what)
{
    const Game *g = &u->game;
    const Creature *c = &g->cr[ci];
    for (int d = 0; d < DIR_COUNT; d++) {
        int x = c->x + DIR_DX[d], y = c->y + DIR_DY[d];
        if (what == A_USE) {
            Terrain t = map_get(&g->map, x, y);
            if (t == T_DOOR_CLOSED || t == T_DOOR_OPEN) return true;
        } else {
            int m = game_creature_at(g, x, y, false);
            if (m != NONE && g->cr[m].owner == c->owner && (CREATURES[g->cr[m].type].flags & CF_MOUNT) &&
                g->cr[m].rider == NONE) {
                return true;
            }
        }
    }
    return false;
}

static void open_cursor_menu(Ui *u)
{
    menu_begin(u, "COMMANDS", M_CURSOR);
    int g = own_creature_at_cursor(u, false), a = own_creature_at_cursor(u, true);
    if (g != NONE) menu_add(u, "SELECT-G", A_SELECT_G, 0, false);
    if (a != NONE) menu_add(u, "SELECT-A", A_SELECT_A, 0, false);
    if (g != NONE || a != NONE) menu_add(u, "VIEW 3D", A_VIEW3D, 0, false);
    menu_add(u, "NEXT", A_NEXT, 0, false);
    menu_add(u, "END TURN", A_END_TURN, 0, false);
    menu_add(u, "CANCEL", A_CANCEL, 0, false);
    u->mode = M_MENU;
    sfx(u, SFX_SELECT);
}

static void open_selected_menu(Ui *u)
{
    const Game *g = &u->game;
    int ci = u->sel;
    const Creature *c = &g->cr[ci];
    int fl = CREATURES[c->type].flags;
    menu_begin(u, CREATURES[c->type].name, M_SELECTED);
    if (game_wizard_caster(g, ci) != NONE) menu_add(u, "CAST", A_CAST, 0, false);
    if ((fl & CF_FLY) && !c->air) menu_add(u, "FLY", A_FLY, 0, false);
    if (c->air) menu_add(u, "LAND", A_LAND, 0, false);
    if ((fl & CF_HANDS) && !c->air && game_object_at(g, c->x, c->y) != NONE) menu_add(u, "PICK UP", A_PICKUP, 0, false);
    if ((fl & CF_HANDS) && c->ninv > 0) menu_add(u, "DROP", A_DROP, 0, false);
    if ((fl & CF_HANDS) && (game_carries(g, ci, OB_HEAL_POTION) ||
                            (game_is_wizard(c) && game_carries(g, ci, OB_MANA_POTION)))) {
        menu_add(u, "DRINK", A_DRINK, 0, false);
    }
    if ((fl & CF_HANDS) && !c->air && has_adjacent(u, ci, A_USE)) menu_add(u, "USE", A_USE, 0, false);
    if ((fl & CF_RIDER) && !c->air && has_adjacent(u, ci, A_RIDE)) menu_add(u, "RIDE", A_RIDE, 0, false);
    if (c->rider != NONE && !c->air) menu_add(u, "DISMOUNT", A_DISMOUNT, 0, false);
    menu_add(u, "VIEW 3D", A_VIEW3D, 0, false);
    menu_add(u, "END MOVE", A_END_MOVE, 0, false);
    menu_add(u, "END TURN", A_END_TURN, 0, false);
    menu_add(u, "CANCEL", A_CANCEL, 0, false);
    u->mode = M_MENU;
    sfx(u, SFX_SELECT);
}

static void open_spell_menu(Ui *u)
{
    const Game *g = &u->game;
    int wi = game_wizard_caster(g, u->sel);
    char t[22];
    snprintf(t, sizeof t, "MANA %d", g->cr[wi].mana);
    menu_begin(u, t, M_SELECTED);
    int first = u->spell_page * SPELLS_PER_PAGE;
    for (int s = first; s < SP_COUNT && s < first + SPELLS_PER_PAGE; s++) {
        char l[22];
        snprintf(l, sizeof l, "%-12s%2d %d", SPELLS[s].name, SPELLS[s].mana, SPELLS[s].level);
        menu_add(u, l, A_SPELL, s, !game_can_afford(g, wi, s));
    }
    menu_add(u, "MORE SPELLS", A_MORE, 0, false);
    menu_add(u, "CANCEL", A_CANCEL, 0, false);
    u->mode = M_SPELLS;
}

static void open_end_confirm(Ui *u)
{
    menu_begin(u, "END TURN?", M_CURSOR);
    menu_add(u, "CONTINUE", A_CONTINUE, 0, false);
    menu_add(u, "END TURN", A_CONFIRM_END, 0, false);
    u->mode = M_MENU;
}

/* ----------------------------------------------------------- game flow */

void ui_start_game(Ui *u)
{
    game_init(&u->game, &SCENARIO_VALE, u->humans, u->ais, u->seed);
    u->sel = NONE;
    u->pov = NONE;
    sync_game(u);
    if (u->game.over) {
        u->mode = M_GAMEOVER;
        return;
    }
    focus_player(u);
    u->mode = u->humans > 1 ? M_HANDOVER : M_CURSOR;
    ui_message(u, u->game.msg);
    sfx(u, SFX_TURN);
}

static void after_turn(Ui *u)
{
    u->sel = NONE;
    sync_game(u);
    if (u->game.over) {
        u->mode = M_GAMEOVER;
        return;
    }
    focus_player(u);
    u->mode = u->humans > 1 ? M_HANDOVER : M_CURSOR;
}

static void next_creature(Ui *u)
{
    const Game *g = &u->game;
    int start = game_creature_at(g, u->cx, u->cy, false);
    if (start == NONE) start = game_creature_at(g, u->cx, u->cy, true);
    for (int k = 1; k <= g->ncr; k++) {
        int i = ((start == NONE ? -1 : start) + k + g->ncr) % g->ncr;
        const Creature *c = &g->cr[i];
        if (game_on_map(c) && c->owner == g->cur) {
            set_cursor(u, c->x, c->y);
            sfx(u, SFX_CURSOR);
            return;
        }
    }
}

static void enter_view3d(Ui *u, int ci, Mode from)
{
    const Creature *c = &u->game.cr[ci];
    u->pov = ci;
    u->pov_from = from;
    u->pov_facing = c->facing;
    u->cam_x = (float)c->x + 0.5f;
    u->cam_y = (float)c->y + 0.5f;
    u->cam_ang = raycast_facing_angle(c->facing);
    u->mode = M_VIEW3D;
    sfx(u, SFX_SELECT);
}

static void exit_view3d(Ui *u)
{
    if (u->pov != NONE && u->game.cr[u->pov].alive) set_cursor(u, u->game.cr[u->pov].x, u->game.cr[u->pov].y);
    u->mode = (u->pov_from == M_SELECTED && sel_valid(u)) ? M_SELECTED : M_CURSOR;
    u->pov = NONE;
}

static void select_creature(Ui *u, int ci)
{
    u->sel = ci;
    u->mode = M_SELECTED;
    set_cursor(u, u->game.cr[ci].x, u->game.cr[ci].y);
    sfx(u, SFX_SELECT);
}

static void begin_target(Ui *u, int what, bool spell)
{
    u->pending = what;
    u->pending_is_spell = spell;
    u->mode = M_TARGET;
    ui_message(u, spell ? "CHOOSE TARGET" : "CHOOSE SQUARE");
}

/* Perform the single adjacent USE or RIDE target directly, otherwise ask. */
static void use_or_target(Ui *u, int what)
{
    const Game *g = &u->game;
    const Creature *c = &g->cr[u->sel];
    int found = 0, fx = 0, fy = 0;
    for (int d = 0; d < DIR_COUNT; d++) {
        int x = c->x + DIR_DX[d], y = c->y + DIR_DY[d];
        bool ok;
        if (what == A_USE) {
            Terrain t = map_get(&g->map, x, y);
            ok = t == T_DOOR_CLOSED || t == T_DOOR_OPEN;
        } else {
            int m = game_creature_at(g, x, y, false);
            ok = m != NONE && g->cr[m].owner == c->owner && (CREATURES[g->cr[m].type].flags & CF_MOUNT) &&
                 g->cr[m].rider == NONE;
        }
        if (ok) { found++; fx = x; fy = y; }
    }
    if (found == 1) {
        u->pending = what;
        u->pending_is_spell = false;
        u->cx = map_wx(&g->map, fx);
        u->cy = map_wy(&g->map, fy);
        u->mode = M_TARGET;
        ui_input(u, (Input){IN_FIRE, 0, 0});
    } else {
        begin_target(u, what, false);
    }
}

static void do_action(Ui *u, int act, int arg)
{
    Game *g = &u->game;
    Mode back = u->menu_from;
    switch (act) {
    case A_CANCEL:
    case A_CONTINUE:
        u->mode = back == M_SELECTED && sel_valid(u) ? M_SELECTED : (back == M_TITLE ? M_TITLE : M_CURSOR);
        break;
    case A_HUMANS:
        u->humans = u->humans % 4 + 1;
        if (u->humans + u->ais > 4) u->ais = 4 - u->humans;
        if (u->humans + u->ais < 2) u->ais = 1;
        title_menu(u);
        u->menu.sel = 0;
        break;
    case A_AIS: {
        int lo = u->humans == 1 ? 1 : 0, hi = 4 - u->humans;
        u->ais = u->ais + 1 > hi ? lo : u->ais + 1;
        title_menu(u);
        u->menu.sel = 1;
        break;
    }
    case A_START: ui_start_game(u); break;
    case A_SELECT_G:
    case A_SELECT_A: {
        int ci = own_creature_at_cursor(u, act == A_SELECT_A);
        if (ci != NONE) select_creature(u, ci);
        else u->mode = M_CURSOR;
        break;
    }
    case A_NEXT:
        u->mode = M_CURSOR;
        next_creature(u);
        break;
    case A_VIEW3D: {
        int ci = back == M_SELECTED ? u->sel : own_creature_at_cursor(u, false);
        if (ci == NONE) ci = own_creature_at_cursor(u, true);
        if (ci != NONE) enter_view3d(u, ci, back);
        else u->mode = M_CURSOR;
        break;
    }
    case A_END_TURN: open_end_confirm(u); break;
    case A_CONFIRM_END:
        game_end_turn(g);
        after_turn(u);
        break;
    case A_CAST:
        u->spell_page = 0;
        open_spell_menu(u);
        break;
    case A_MORE:
        u->spell_page = (u->spell_page + 1) % ((SP_COUNT + SPELLS_PER_PAGE - 1) / SPELLS_PER_PAGE);
        open_spell_menu(u);
        u->menu.sel = u->menu.n - 2;
        break;
    case A_SPELL:
        if (game_spell_needs_target(arg)) {
            u->menu_from = M_SELECTED;
            begin_target(u, arg, true);
            set_cursor(u, g->cr[u->sel].x, g->cr[u->sel].y);
        } else {
            game_cast(g, game_wizard_caster(g, u->sel), arg, 0, 0);
            sync_game(u);
            u->mode = sel_valid(u) ? M_SELECTED : M_CURSOR;
        }
        break;
    case A_FLY: game_fly(g, u->sel); sync_game(u); u->mode = M_SELECTED; break;
    case A_LAND: game_land(g, u->sel); sync_game(u); u->mode = u->game.over ? M_GAMEOVER : M_SELECTED; break;
    case A_PICKUP: game_pickup(g, u->sel); sync_game(u); u->mode = M_SELECTED; break;
    case A_DROP: game_drop(g, u->sel); sync_game(u); u->mode = M_SELECTED; break;
    case A_DRINK: game_drink(g, u->sel); sync_game(u); u->mode = M_SELECTED; break;
    case A_USE:
    case A_RIDE: use_or_target(u, act); break;
    case A_DISMOUNT: {
        int r = g->cr[u->sel].rider;
        if (game_dismount(g, u->sel) == R_OK) {
            sync_game(u);
            select_creature(u, r);
        } else {
            sync_game(u);
            u->mode = M_SELECTED;
        }
        break;
    }
    default: /* A_END_MOVE */
        u->sel = NONE;
        u->mode = M_CURSOR;
        break;
    }
}

static void menu_choose(Ui *u, int i)
{
    Menu *m = &u->menu;
    if (m->disabled[i]) {
        sfx(u, SFX_ERROR);
        ui_message(u, "CANNOT DO THAT NOW");
        return;
    }
    sfx(u, SFX_SELECT);
    do_action(u, m->act[i], m->arg[i]);
}

static void menu_input(Ui *u, Input in)
{
    Menu *m = &u->menu;
    switch (in.kind) {
    case IN_UP: m->sel = (m->sel + m->n - 1) % m->n; sfx(u, SFX_CURSOR); break;
    case IN_DOWN: m->sel = (m->sel + 1) % m->n; sfx(u, SFX_CURSOR); break;
    case IN_FIRE: menu_choose(u, m->sel); break;
    case IN_CANCEL:
        if (u->mode == M_TITLE) break;
        do_action(u, A_CANCEL, 0);
        break;
    case IN_TAP:
        for (int i = 0; i < m->n; i++) {
            Rect r = ui_menu_item_rect(u, i);
            if (rect_has(&r, in.x, in.y)) {
                m->sel = i;
                menu_choose(u, i);
                return;
            }
        }
        if (u->mode != M_TITLE) do_action(u, A_CANCEL, 0);
        break;
    default: break;
    }
}

/* -------------------------------------------------------------- map modes */

static void map_fire(Ui *u)
{
    Game *g = &u->game;
    if (u->mode == M_CURSOR) {
        open_cursor_menu(u);
        return;
    }
    if (u->mode == M_TARGET) {
        int s = sel_valid(u) ? u->sel : NONE;
        if (s == NONE) { u->mode = M_CURSOR; return; }
        if (u->pending_is_spell) {
            game_cast(g, game_wizard_caster(g, s), u->pending, u->cx, u->cy);
        } else if (u->pending == A_USE) {
            game_use_door(g, s, u->cx, u->cy);
        } else {
            int m = game_creature_at(g, u->cx, u->cy, false);
            if (m != NONE && game_ride(g, s, m) == R_OK) {
                sync_game(u);
                select_creature(u, m);
                return;
            }
            if (m == NONE) game_ride(g, s, s); /* produces the refusal message */
        }
        sync_game(u);
        if (g->over) { u->mode = M_GAMEOVER; return; }
        u->mode = sel_valid(u) ? M_SELECTED : M_CURSOR;
        if (u->mode == M_SELECTED) set_cursor(u, g->cr[u->sel].x, g->cr[u->sel].y);
        return;
    }
    /* M_SELECTED */
    if (!sel_valid(u)) { u->mode = M_CURSOR; return; }
    Creature *c = &g->cr[u->sel];
    int ddx = wrap_delta(c->x, u->cx, g->map.w), ddy = wrap_delta(c->y, u->cy, g->map.h);
    if (iabs(ddx) <= 1 && iabs(ddy) <= 1 && (ddx || ddy)) {
        int d = dir_from_delta(ddx, ddy);
        int ox = c->x, oy = c->y;
        game_move(g, u->sel, d);
        sync_game(u);
        if (g->over) { u->mode = M_GAMEOVER; return; }
        if (!sel_valid(u)) { u->mode = M_CURSOR; return; }
        if (c->x != ox || c->y != oy) set_cursor(u, u->cx + DIR_DX[d], u->cy + DIR_DY[d]);
        return;
    }
    open_selected_menu(u);
}

static void map_tap(Ui *u, int x, int y)
{
    if (x >= VIEW_X && y >= VIEW_Y && x < VIEW_X + VIEW_PX && y < VIEW_Y + VIEW_PX) {
        int mx = map_wx(&u->game.map, u->vx + (x - VIEW_X) / 16);
        int my = map_wy(&u->game.map, u->vy + (y - VIEW_Y) / 16);
        if (mx == u->cx && my == u->cy) {
            map_fire(u);
        } else {
            u->cx = mx;
            u->cy = my;
            keep_cursor_visible(u);
            sfx(u, SFX_CURSOR);
        }
    } else if (x >= PANEL_X) {
        if (u->mode == M_SELECTED && sel_valid(u)) open_selected_menu(u);
        else if (u->mode == M_TARGET) u->mode = sel_valid(u) ? M_SELECTED : M_CURSOR;
        else open_cursor_menu(u);
    }
}

static void map_input(Ui *u, Input in)
{
    int d = dir_of_input(in.kind);
    if (d >= 0) {
        set_cursor(u, u->cx + DIR_DX[d], u->cy + DIR_DY[d]);
        sfx(u, SFX_CURSOR);
        return;
    }
    switch (in.kind) {
    case IN_FIRE: map_fire(u); break;
    case IN_TAP: map_tap(u, in.x, in.y); break;
    case IN_NEXT: next_creature(u); break;
    case IN_VIEW: {
        int ci = u->mode == M_SELECTED && sel_valid(u) ? u->sel : own_creature_at_cursor(u, false);
        if (ci == NONE) ci = own_creature_at_cursor(u, true);
        if (ci != NONE) enter_view3d(u, ci, u->mode);
        break;
    }
    case IN_CANCEL:
        if (u->mode == M_TARGET) {
            u->mode = sel_valid(u) ? M_SELECTED : M_CURSOR;
        } else if (u->mode == M_SELECTED) {
            u->sel = NONE;
            u->mode = M_CURSOR;
        }
        break;
    default: break;
    }
}

/* ---------------------------------------------------------------- 3D view */

static void view_turn(Ui *u, int steps)
{
    u->pov_facing = (u->pov_facing + steps + DIR_COUNT) % DIR_COUNT;
    sfx(u, SFX_CURSOR);
}

static void view_forward(Ui *u)
{
    Game *g = &u->game;
    int ci = u->pov;
    if (g->cr[ci].owner != g->cur) return;
    Result r = game_move(g, ci, u->pov_facing);
    sync_game(u);
    if (g->over) { u->mode = M_GAMEOVER; return; }
    if (r == R_OK && !g->cr[ci].alive) exit_view3d(u);
    else if (!g->cr[ci].alive) exit_view3d(u);
}

static void view_input(Ui *u, Input in)
{
    switch (in.kind) {
    case IN_LEFT: case IN_UPLEFT: view_turn(u, -1); break;
    case IN_RIGHT: case IN_UPRIGHT: view_turn(u, 1); break;
    case IN_DOWN: view_turn(u, 4); break;
    case IN_UP: view_forward(u); break;
    case IN_TAP: {
        int x = in.x - VIEW_X, y = in.y - VIEW_Y;
        if (x < 0 || y < 0 || x >= VIEW_PX || y >= VIEW_PX || y >= VIEW_PX - 14) exit_view3d(u);
        else if (x < VIEW_PX / 3) view_turn(u, -1);
        else if (x >= VIEW_PX * 2 / 3) view_turn(u, 1);
        else view_forward(u);
        break;
    }
    case IN_FIRE: case IN_CANCEL: case IN_VIEW: exit_view3d(u); break;
    default: break;
    }
}

/* ------------------------------------------------------------------ input */

void ui_init(Ui *u, uint64_t seed)
{
    memset(u, 0, sizeof *u);
    u->seed = seed;
    u->humans = 1;
    u->ais = 1;
    u->sel = NONE;
    u->pov = NONE;
    u->mode = M_TITLE;
    title_menu(u);
}

void ui_input(Ui *u, Input in)
{
    switch (u->mode) {
    case M_TITLE:
    case M_MENU:
    case M_SPELLS: menu_input(u, in); break;
    case M_HANDOVER:
        if (in.kind == IN_FIRE || in.kind == IN_TAP) {
            u->mode = M_CURSOR;
            sfx(u, SFX_SELECT);
        }
        break;
    case M_GAMEOVER:
        if (in.kind == IN_FIRE || in.kind == IN_TAP) {
            ui_init(u, u->seed + 1);
        }
        break;
    case M_VIEW3D: view_input(u, in); break;
    default: map_input(u, in); break;
    }
}

static float approach(float v, float target, float k)
{
    return v + (target - v) * k;
}

void ui_tick(Ui *u)
{
    u->tick++;
    if (u->msg_timer > 0) u->msg_timer--;
    if (u->fx_timer > 0) u->fx_timer--;
    if (u->mode == M_VIEW3D && u->pov != NONE) {
        const Creature *c = &u->game.cr[u->pov];
        const Map *m = &u->game.map;
        float tx = (float)c->x + 0.5f, ty = (float)c->y + 0.5f;
        /* move along the shortest wrapped path */
        float ddx = (float)wrap_delta((int)u->cam_x, c->x, m->w) + (tx - (float)c->x) - (u->cam_x - (float)(int)u->cam_x);
        float ddy = (float)wrap_delta((int)u->cam_y, c->y, m->h) + (ty - (float)c->y) - (u->cam_y - (float)(int)u->cam_y);
        u->cam_x = approach(u->cam_x, u->cam_x + ddx, 0.35f);
        u->cam_y = approach(u->cam_y, u->cam_y + ddy, 0.35f);
        if (u->cam_x < 0) u->cam_x += (float)m->w;
        if (u->cam_y < 0) u->cam_y += (float)m->h;
        if (u->cam_x >= (float)m->w) u->cam_x -= (float)m->w;
        if (u->cam_y >= (float)m->h) u->cam_y -= (float)m->h;
        float target = raycast_facing_angle(u->pov_facing);
        float da = target - u->cam_ang;
        while (da > 3.14159265f) da -= 6.2831853f;
        while (da < -3.14159265f) da += 6.2831853f;
        u->cam_ang += da * 0.35f;
    }
}
