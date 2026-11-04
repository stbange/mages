/* Modern direct-manipulation UI: hover to inspect, click/tap to select, move,
   attack and cast, action bar buttons with hotkeys, touch preview-then-confirm. */
#include "ui.h"
#include "audio.h"
#include "raycast.h"
#include <stdio.h>
#include <string.h>

static const char *const MODE_NAMES[M_COUNT] = {
    "TITLE", "HANDOVER", "PLAY", "LIST", "TARGET", "VIEW3D", "GAMEOVER",
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

int ui_viewer(const Ui *u)
{
    if (u->mode == M_GAMEOVER || u->game.over) return NONE;
    return u->game.cur;
}

bool ui_walking(const Ui *u) { return u->path_pos < u->path_len; }

/* ------------------------------------------------------------ selection */

static bool sel_valid(const Ui *u)
{
    if (u->sel == NONE) return false;
    const Creature *c = &u->game.cr[u->sel];
    return c->alive && c->owner == u->game.cur && c->mount == NONE;
}

static bool visible_to_cur(const Game *g, int ci)
{
    return g->cur >= 0 && game_can_see_creature(g, g->cur, ci);
}

/* Visible enemy creature next to (x,y) on the given layer. */
static bool enemy_adjacent(const Game *g, int owner, int x, int y, bool air)
{
    for (int d = 0; d < DIR_COUNT; d++) {
        int o = game_creature_at(g, x + DIR_DX[d], y + DIR_DY[d], air);
        if (o != NONE && g->cr[o].owner != owner && visible_to_cur(g, o)) return true;
    }
    return false;
}

/* AP to enter (x,y) from a neighbour, or -1. Visible creatures block; hidden
   ones are ignored so that ambushes interrupt the real walk. */
static int step_cost(const Game *g, int ci, int x, int y)
{
    const Creature *c = &g->cr[ci];
    int o = game_creature_at(g, x, y, c->air);
    if (o != NONE && (g->cr[o].owner == c->owner || visible_to_cur(g, o))) return -1;
    int cost = game_terrain_cost(g, ci, x, y);
    if (cost >= 0) return cost;
    if (!c->air && (CREATURES[c->type].flags & CF_HANDS) && map_get(&g->map, x, y) == T_DOOR_CLOSED) {
        return 2 + TERRAIN[T_DOOR_OPEN].cost;
    }
    return -1;
}

static void compute_reach(Ui *u)
{
    const Game *g = &u->game;
    const Map *m = &g->map;
    for (int y = 0; y < MAP_MAX; y++)
        for (int x = 0; x < MAP_MAX; x++) u->reach[y][x] = -1;
    if (!sel_valid(u)) return;
    const Creature *c = &g->cr[u->sel];
    u->reach[c->y][c->x] = 0;
    if (g->turn == 1 || game_engaged(g, u->sel)) return;
    bool changed = true;
    while (changed) {
        changed = false;
        for (int y = 0; y < m->h; y++) {
            for (int x = 0; x < m->w; x++) {
                int here = u->reach[y][x];
                if (here < 0) continue;
                bool start = x == c->x && y == c->y;
                if (!start && enemy_adjacent(g, c->owner, x, y, c->air)) continue; /* must stop here */
                for (int d = 0; d < DIR_COUNT; d++) {
                    int nx = map_wx(m, x + DIR_DX[d]), ny = map_wy(m, y + DIR_DY[d]);
                    int cost = step_cost(g, u->sel, nx, ny);
                    if (cost < 0 || here + cost > c->ap) continue;
                    int16_t nc = (int16_t)(here + cost);
                    if (u->reach[ny][nx] < 0 || nc < u->reach[ny][nx]) {
                        u->reach[ny][nx] = nc;
                        u->reach_dir[ny][nx] = (int8_t)d;
                        changed = true;
                    }
                }
            }
        }
    }
}

int ui_reach_cost(const Ui *u, int x, int y)
{
    const Map *m = &u->game.map;
    return u->reach[map_wy(m, y)][map_wx(m, x)];
}

/* Fill path with directions from the selection to (x,y). Returns its length. */
static int build_path(const Ui *u, int x, int y, uint8_t *path)
{
    const Map *m = &u->game.map;
    const Creature *c = &u->game.cr[u->sel];
    uint8_t rev[MAX_PATH];
    int n = 0;
    x = map_wx(m, x);
    y = map_wy(m, y);
    while ((x != c->x || y != c->y) && n < MAX_PATH) {
        int d = u->reach_dir[y][x];
        rev[n++] = (uint8_t)d;
        x = map_wx(m, x - DIR_DX[d]);
        y = map_wy(m, y - DIR_DY[d]);
    }
    for (int i = 0; i < n; i++) path[i] = rev[n - 1 - i];
    return n;
}

/* Cheapest square next to creature ti from which the selection can still attack. */
static bool attack_plan(const Ui *u, int ti, int *dx, int *dy, int *cost)
{
    const Game *g = &u->game;
    if (!sel_valid(u)) return false;
    const Creature *c = &g->cr[u->sel], *t = &g->cr[ti];
    if (!game_on_map(t) || t->owner == c->owner || t->air != c->air || !visible_to_cur(g, ti)) return false;
    int best = -1;
    for (int d = 0; d < DIR_COUNT; d++) {
        int x = map_wx(&g->map, t->x + DIR_DX[d]), y = map_wy(&g->map, t->y + DIR_DY[d]);
        int r = u->reach[y][x];
        if (r < 0 || r + ATTACK_AP > c->ap) continue;
        if (best < 0 || r < best) {
            best = r;
            *dx = x;
            *dy = y;
        }
    }
    if (best < 0) return false;
    *cost = best + ATTACK_AP;
    return true;
}

bool ui_attackable(const Ui *u, int ci, int *cost)
{
    int x, y;
    return attack_plan(u, ci, &x, &y, cost);
}

bool ui_valid_target(const Ui *u, int x, int y)
{
    const Game *g = &u->game;
    if (u->mode != M_TARGET || !sel_valid(u)) return false;
    int wi = game_wizard_caster(g, u->sel);
    if (wi == NONE) return false;
    const Creature *w = &g->cr[wi];
    x = map_wx(&g->map, x);
    y = map_wy(&g->map, y);
    if (!g->pl[g->cur].seen[y][x]) return false;
    if (map_dist(&g->map, w->x, w->y, x, y) > SPELL_RANGE || !game_los(&g->map, w->x, w->y, x, y)) return false;
    int t = game_creature_at(g, x, y, false);
    if (t == NONE || !visible_to_cur(g, t)) t = game_creature_at(g, x, y, true);
    if (t != NONE && !visible_to_cur(g, t)) t = NONE;
    switch (SPELLS[u->pending_spell].kind) {
    case SK_FIREBALL: return true;
    case SK_HEAL: case SK_INVIS: return t != NONE && g->cr[t].owner == g->cur;
    case SK_QUICKEN: return t != NONE && g->cr[t].owner == g->cur && t != wi;
    case SK_TELEPORT:
        return t == NONE && !map_has(&g->map, x, y, TF_NOGROUND) && game_creature_at(g, x, y, false) == NONE;
    case SK_BANISH: return t != NONE && !game_is_wizard(&g->cr[t]);
    default: return t != NONE; /* bolt, sparks */
    }
}

/* ------------------------------------------------------------ view/cursor */

static void centre_view(Ui *u, int x, int y)
{
    u->vx = map_wx(&u->game.map, x - VIEW_TILES / 2);
    u->vy = map_wy(&u->game.map, y - VIEW_TILES / 2);
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

static bool in_map(int x, int y)
{
    return x >= VIEW_X && y >= VIEW_Y && x < VIEW_X + VIEW_PX && y < VIEW_Y + VIEW_PX;
}

static void screen_to_square(const Ui *u, int x, int y, int *mx, int *my)
{
    *mx = map_wx(&u->game.map, u->vx + (x - VIEW_X) / 16);
    *my = map_wy(&u->game.map, u->vy + (y - VIEW_Y) / 16);
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

/* ---------------------------------------------------------- game syncing */

/* After any game action: sounds, messages, game over, selection, reach. */
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
    if (u->game.over) {
        u->mode = M_GAMEOVER;
        u->path_len = u->path_pos = 0;
        u->saved_len = 0;
        u->save_request = SAVE_CLEAR;
        return;
    }
    if (u->sel != NONE && !sel_valid(u)) {
        u->sel = NONE;
        if (u->mode == M_TARGET || u->mode == M_LIST) u->mode = M_PLAY;
    }
    compute_reach(u);
}

static void select_creature(Ui *u, int ci)
{
    u->sel = ci;
    u->armed_x = -1;
    sfx(u, SFX_SELECT);
    compute_reach(u);
}

static void autosave(Ui *u)
{
    SaveMeta m = {u->humans, u->ais, u->seed};
    u->saved_len = save_write(&u->game, &m, u->saved, sizeof u->saved);
    u->save_request = u->saved_len > 0 ? SAVE_WRITE : SAVE_NONE;
}

static void begin_player_turn(Ui *u)
{
    const Game *g = &u->game;
    u->sel = NONE;
    u->end_confirm = 0;
    u->path_len = u->path_pos = 0;
    u->armed_x = -1;
    if (g->cur >= 0) {
        int w = g->pl[g->cur].wizard;
        const Creature *c = &g->cr[w];
        int on = c->mount != NONE ? c->mount : w;
        set_cursor(u, g->cr[on].x, g->cr[on].y);
        centre_view(u, g->cr[on].x, g->cr[on].y);
        select_creature(u, on);
    }
    u->mode = u->humans > 1 ? M_HANDOVER : M_PLAY;
    autosave(u);
}

void ui_start_game(Ui *u)
{
    game_init(&u->game, &SCENARIO_VALE, u->humans, u->ais, u->seed);
    u->pov = NONE;
    sync_game(u);
    if (u->game.over) return;
    begin_player_turn(u);
    ui_message(u, u->game.msg);
    sfx(u, SFX_TURN);
}

/* ------------------------------------------------------------------ walk */

static void walk_step(Ui *u)
{
    Game *g = &u->game;
    int ci = u->walk_ci;
    Creature *c = &g->cr[ci];
    int d = u->path[u->path_pos++];
    int nx = c->x + DIR_DX[d], ny = c->y + DIR_DY[d];
    int ox = c->x, oy = c->y;
    bool ok = true;
    if (map_get(&g->map, nx, ny) == T_DOOR_CLOSED) ok = game_use_door(g, ci, nx, ny) == R_OK;
    if (ok) ok = game_move(g, ci, d) == R_OK && c->alive && (c->x != ox || c->y != oy);
    if (ok && c->alive) set_cursor(u, c->x, c->y);
    if (!ok) u->path_len = u->path_pos; /* stop */
    if (u->path_pos >= u->path_len && ok && u->walk_attack != NONE && c->alive && !g->over) {
        int t = u->walk_attack;
        if (g->cr[t].alive) {
            game_attack(g, ci, t);
            if (g->cr[t].alive) set_cursor(u, g->cr[t].x, g->cr[t].y);
        }
    }
    if (u->path_pos >= u->path_len) u->walk_attack = NONE;
    sync_game(u);
}

static void finish_walk(Ui *u)
{
    while (ui_walking(u)) walk_step(u);
}

static void start_walk(Ui *u, int tx, int ty, int attack)
{
    u->walk_ci = u->sel;
    u->walk_attack = attack;
    u->path_len = build_path(u, tx, ty, u->path);
    u->path_pos = 0;
    u->armed_x = -1;
    if (u->path_len == 0) {
        if (attack != NONE) {
            game_attack(&u->game, u->sel, attack);
            u->walk_attack = NONE;
            sync_game(u);
        }
        return;
    }
    walk_step(u);
    u->walk_timer = WALK_TICKS;
}

/* ----------------------------------------------------------------- lists */

static void list_begin(Ui *u, const char *title, bool spells)
{
    memset(&u->menu, 0, sizeof u->menu);
    snprintf(u->menu.title, sizeof u->menu.title, "%s", title);
    u->list_spells = spells;
    u->mode = M_LIST;
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
        int first = u->menu.n > 3 ? 1 : 0; /* CONTINUE sits above the usual entries */
        Rect t = {190, 132 + (i - first) * MENU_ROW, 118, MENU_ROW};
        return t;
    }
    Rect r = {PANEL_X + 2, MENU_Y + i * MENU_ROW, SCREEN_W - PANEL_X - 6, MENU_ROW};
    return r;
}

static void title_menu(Ui *u)
{
    char a[22], b[22];
    int sel = u->menu.sel;
    memset(&u->menu, 0, sizeof u->menu);
    snprintf(a, sizeof a, "PLAYERS   %d", u->humans);
    snprintf(b, sizeof b, "COMPUTER  %d", u->ais);
    if (u->saved_len > 0) menu_add(u, "CONTINUE", A_CONTINUE, 0, false);
    menu_add(u, a, A_HUMANS, 0, false);
    menu_add(u, b, A_AIS, 0, false);
    menu_add(u, "START GAME", A_START, 0, false);
    u->menu.sel = sel;
}

static void open_spell_book(Ui *u)
{
    const Game *g = &u->game;
    int wi = game_wizard_caster(g, u->sel);
    char t[22];
    snprintf(t, sizeof t, "SPELLS  MANA %d", g->cr[wi].mana);
    list_begin(u, t, true);
    int first = u->spell_page * SPELLS_PER_PAGE;
    for (int s = first; s < SP_COUNT && s < first + SPELLS_PER_PAGE; s++) {
        char l[48];
        snprintf(l, sizeof l, "%d %-12.12s%3d", s - first + 1, SPELLS[s].name, (int)SPELLS[s].mana);
        menu_add(u, l, A_SPELL, s, !game_can_afford(g, wi, s));
    }
    menu_add(u, "MORE SPELLS", A_SPELL_PAGE, 0, false);
    menu_add(u, "CLOSE", A_CANCEL, 0, false);
}

/* ---------------------------------------------------------- action bar */

static int adjacent_doors(const Ui *u, int ci, int *fx, int *fy)
{
    const Game *g = &u->game;
    const Creature *c = &g->cr[ci];
    int n = 0;
    for (int d = 0; d < DIR_COUNT; d++) {
        int x = c->x + DIR_DX[d], y = c->y + DIR_DY[d];
        Terrain t = map_get(&g->map, x, y);
        if (t == T_DOOR_CLOSED || t == T_DOOR_OPEN) {
            n++;
            *fx = map_wx(&g->map, x);
            *fy = map_wy(&g->map, y);
        }
    }
    return n;
}

static bool is_free_mount(const Game *g, int m, int owner)
{
    return m != NONE && g->cr[m].owner == owner && (CREATURES[g->cr[m].type].flags & CF_MOUNT) &&
           g->cr[m].rider == NONE;
}

static int adjacent_mounts(const Ui *u, int ci, int *found)
{
    const Game *g = &u->game;
    const Creature *c = &g->cr[ci];
    int n = 0;
    for (int d = 0; d < DIR_COUNT; d++) {
        int m = game_creature_at(g, c->x + DIR_DX[d], c->y + DIR_DY[d], false);
        if (is_free_mount(g, m, c->owner)) {
            n++;
            *found = m;
        }
    }
    return n;
}

/* Context actions possible for the selection, in priority order. */
static int context_actions(const Ui *u, uint8_t *acts)
{
    const Game *g = &u->game;
    int n = 0, ci = u->sel, tmp, tx, ty;
    const Creature *c = &g->cr[ci];
    int fl = CREATURES[c->type].flags;
    if (game_wizard_caster(g, ci) != NONE) acts[n++] = A_CAST;
    if (c->air) acts[n++] = A_LAND;
    else if (fl & CF_FLY) acts[n++] = A_FLY;
    if ((fl & CF_HANDS) && !c->air && game_object_at(g, c->x, c->y) != NONE) acts[n++] = A_PICKUP;
    if ((fl & CF_HANDS) && (game_carries(g, ci, OB_HEAL_POTION) ||
                            (game_is_wizard(c) && game_carries(g, ci, OB_MANA_POTION)))) {
        acts[n++] = A_DRINK;
    }
    if (c->rider != NONE && !c->air) acts[n++] = A_DISMOUNT;
    else if ((fl & CF_RIDER) && !c->air && adjacent_mounts(u, ci, &tmp) > 0) acts[n++] = A_RIDE;
    if ((fl & CF_HANDS) && !c->air && adjacent_doors(u, ci, &tx, &ty) > 0) acts[n++] = A_DOOR;
    if ((fl & CF_HANDS) && c->ninv > 0) acts[n++] = A_DROP;
    return n;
}

static const struct { const char *label; const char *key; } ACTION_TEXT[A_COUNT] = {
    [A_CAST] = {"CAST", "C"}, [A_VIEW3D] = {"3D VIEW", "V"}, [A_FLY] = {"FLY", "F"},
    [A_LAND] = {"LAND", "F"}, [A_PICKUP] = {"PICK UP", "G"}, [A_DROP] = {"DROP", "X"},
    [A_DRINK] = {"DRINK", "Q"}, [A_RIDE] = {"RIDE", "R"}, [A_DISMOUNT] = {"DISMOUNT", "R"},
    [A_DOOR] = {"DOOR", "U"}, [A_MORE] = {"MORE", ""}, [A_NEXT] = {"NEXT", "TAB"},
    [A_END_TURN] = {"END TURN", "T"}, [A_HELP] = {"HELP", "H"}, [A_CANCEL] = {"CANCEL", "ESC"},
    [A_TURN_LEFT] = {"< TURN", "A"}, [A_TURN_RIGHT] = {"TURN >", "D"}, [A_FORWARD] = {"FORWARD", "W"},
    [A_EXIT3D] = {"EXIT 3D", "V"}, [A_MENU] = {"MENU", "ESC"},
};

static void put_button(Button *b, int slot, int act, bool enabled)
{
    b->r.x = PANEL_X + 2 + (slot % 2) * (BTN_W + 2);
    b->r.y = BTN_Y + (slot / 2) * (BTN_H + 2);
    b->r.w = BTN_W;
    b->r.h = BTN_H;
    b->act = (uint8_t)act;
    b->enabled = enabled;
    snprintf(b->label, sizeof b->label, "%s", ACTION_TEXT[act].label);
    snprintf(b->key, sizeof b->key, "%s", ACTION_TEXT[act].key);
}

int ui_buttons(const Ui *u, Button *out)
{
    int n = 0;
    if (u->mode == M_PLAY || u->mode == M_TARGET) {
        if (u->mode == M_TARGET) {
            put_button(&out[n++], 0, A_CANCEL, true);
            put_button(&out[n++], 1, A_HELP, true);
        } else if (sel_valid(u)) {
            uint8_t acts[12];
            int k = context_actions(u, acts);
            static const int slots[3] = {0, 2, 3};
            for (int i = 0; i < k && i < 3; i++) {
                bool more = i == 2 && k > 3;
                put_button(&out[n++], slots[i], more ? A_MORE : acts[i], true);
            }
            put_button(&out[n++], 1, A_VIEW3D, true);
        } else {
            put_button(&out[n++], 0, A_MENU, true);
            put_button(&out[n++], 1, A_HELP, true);
        }
    } else if (u->mode == M_VIEW3D) {
        put_button(&out[n++], 0, A_TURN_LEFT, true);
        put_button(&out[n++], 1, A_TURN_RIGHT, true);
        put_button(&out[n++], 2, A_FORWARD, u->game.cr[u->pov].owner == u->game.cur);
        put_button(&out[n++], 3, A_EXIT3D, true);
    } else {
        return 0;
    }
    put_button(&out[n++], 4, A_NEXT, true);
    put_button(&out[n], 5, A_END_TURN, true);
    if (u->end_confirm > 0) snprintf(out[n].label, sizeof out[n].label, "CONFIRM?");
    return n + 1;
}

/* ---------------------------------------------------------------- 3D view */

static void enter_view3d(Ui *u, int ci)
{
    const Creature *c = &u->game.cr[ci];
    u->pov = ci;
    u->pov_facing = c->facing;
    u->cam_x = (float)c->x + 0.5f;
    u->cam_y = (float)c->y + 0.5f;
    u->cam_ang = raycast_facing_angle(c->facing);
    u->view_drag = 0;
    u->mode = M_VIEW3D;
    sfx(u, SFX_SELECT);
}

static void exit_view3d(Ui *u)
{
    const Creature *c = &u->game.cr[u->pov];
    if (c->alive && c->mount == NONE) {
        set_cursor(u, c->x, c->y);
        if (c->owner == u->game.cur) select_creature(u, u->pov);
    }
    u->mode = M_PLAY;
    u->pov = NONE;
    compute_reach(u);
}

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
    game_move(g, ci, u->pov_facing);
    sync_game(u);
    if (u->mode == M_VIEW3D && !g->cr[ci].alive) exit_view3d(u);
}

/* --------------------------------------------------------------- actions */

static int cycle_creature(const Ui *u, int from, int step, bool need_ap)
{
    const Game *g = &u->game;
    for (int k = 1; k <= g->ncr; k++) {
        int i = ((from == NONE ? (step > 0 ? -1 : 0) : from) + k * step + g->ncr * 2) % g->ncr;
        const Creature *c = &g->cr[i];
        if (game_on_map(c) && c->owner == g->cur && (!need_ap || c->ap > 0)) return i;
    }
    return NONE;
}

static void next_creature(Ui *u, int step)
{
    int from = u->mode == M_VIEW3D ? u->pov : u->sel;
    int i = cycle_creature(u, from, step, u->mode != M_VIEW3D);
    if (i == NONE) i = cycle_creature(u, from, step, false);
    if (i == NONE) return;
    const Creature *c = &u->game.cr[i];
    if (u->mode == M_VIEW3D) {
        enter_view3d(u, i);
        return;
    }
    u->mode = M_PLAY;
    set_cursor(u, c->x, c->y);
    centre_view(u, c->x, c->y);
    select_creature(u, i);
}

static bool can_still_act(const Game *g)
{
    for (int i = 0; i < g->ncr; i++) {
        const Creature *c = &g->cr[i];
        if (c->alive && c->owner == g->cur && c->mount == NONE && c->ap >= 2) return true;
    }
    return false;
}

static void end_turn(Ui *u)
{
    if (u->end_confirm == 0 && can_still_act(&u->game)) {
        u->end_confirm = CONFIRM_TICKS;
        ui_message(u, "CREATURES CAN STILL ACT - END TURN AGAIN");
        sfx(u, SFX_ERROR);
        return;
    }
    if (u->mode == M_VIEW3D) exit_view3d(u);
    u->end_confirm = 0;
    game_end_turn(&u->game);
    sync_game(u);
    if (u->mode == M_GAMEOVER) return;
    begin_player_turn(u);
}

static void need_selection(Ui *u)
{
    ui_message(u, "SELECT A CREATURE FIRST");
    sfx(u, SFX_ERROR);
}

static void do_action(Ui *u, int act, int arg);

static void ride_or_door(Ui *u, int act)
{
    Game *g = &u->game;
    int ci = u->sel;
    if (act == A_DOOR) {
        int fx = -1, fy = -1;
        Terrain t = map_get(&g->map, u->cx, u->cy);
        bool at_cursor = (t == T_DOOR_CLOSED || t == T_DOOR_OPEN) &&
                         map_dist(&g->map, g->cr[ci].x, g->cr[ci].y, u->cx, u->cy) == 1;
        if (at_cursor) { fx = u->cx; fy = u->cy; }
        else if (adjacent_doors(u, ci, &fx, &fy) != 1) fx = -1;
        if (fx < 0) {
            ui_message(u, "POINT AT THE DOOR TO USE");
            sfx(u, SFX_ERROR);
            return;
        }
        game_use_door(g, ci, fx, fy);
    } else {
        int m = game_creature_at(g, u->cx, u->cy, false);
        bool at_cursor = is_free_mount(g, m, g->cur) &&
                         map_dist(&g->map, g->cr[ci].x, g->cr[ci].y, u->cx, u->cy) == 1;
        if (!at_cursor && adjacent_mounts(u, ci, &m) != 1) m = NONE;
        if (m == NONE) {
            ui_message(u, "POINT AT THE MOUNT TO RIDE");
            sfx(u, SFX_ERROR);
            return;
        }
        if (game_ride(g, ci, m) == R_OK) {
            sync_game(u);
            select_creature(u, m);
            return;
        }
    }
    sync_game(u);
}

static void open_more(Ui *u)
{
    uint8_t acts[12];
    int k = context_actions(u, acts);
    list_begin(u, "MORE ACTIONS", false);
    for (int i = 2; i < k; i++) menu_add(u, ACTION_TEXT[acts[i]].label, (Action)acts[i], 0, false);
    menu_add(u, "CLOSE", A_CANCEL, 0, false);
}

static void choose_spell(Ui *u, int spell)
{
    Game *g = &u->game;
    if (!game_can_afford(g, game_wizard_caster(g, u->sel), spell)) {
        ui_message(u, "NOT ENOUGH MANA OR ACTION POINTS");
        sfx(u, SFX_ERROR);
        return;
    }
    if (game_spell_needs_target(spell)) {
        u->pending_spell = spell;
        u->mode = M_TARGET;
        u->armed_x = -1;
        char m[64];
        snprintf(m, sizeof m, "%s: CHOOSE A TARGET", SPELLS[spell].name);
        ui_message(u, m);
        return;
    }
    u->mode = M_PLAY;
    game_cast(g, game_wizard_caster(g, u->sel), spell, 0, 0);
    sync_game(u);
}

bool ui_offer_save(Ui *u, const uint8_t *buf, size_t len)
{
    static Game probe;
    SaveMeta m;
    if (len > sizeof u->saved || !save_read(&probe, &m, buf, len) || probe.over) return false;
    memcpy(u->saved, buf, len);
    u->saved_len = len;
    if (u->mode == M_TITLE) {
        title_menu(u);
        u->menu.sel = 0;
    }
    return true;
}

SaveRequest ui_take_save_request(Ui *u)
{
    SaveRequest r = u->save_request;
    u->save_request = SAVE_NONE;
    return r;
}

static void continue_game(Ui *u)
{
    SaveMeta m;
    if (!save_read(&u->game, &m, u->saved, u->saved_len)) return;
    u->humans = m.humans;
    u->ais = m.ais;
    u->seed = m.seed;
    u->pov = NONE;
    u->help = false;
    begin_player_turn(u);
    ui_message(u, "WELCOME BACK");
    sfx(u, SFX_TURN);
}

static void quit_to_title(Ui *u)
{
    static uint8_t keep[SAVE_MAX_BYTES];
    size_t n = u->saved_len;
    memcpy(keep, u->saved, n);
    uint64_t seed = u->seed + 1;
    ui_init(u, seed);
    ui_offer_save(u, keep, n);
}

static void open_game_menu(Ui *u)
{
    list_begin(u, "GAME MENU", false);
    menu_add(u, "RESUME", A_CANCEL, 0, false);
    menu_add(u, "HELP", A_HELP, 0, false);
    menu_add(u, "QUIT TO TITLE", A_QUIT, 0, false);
}

static void do_action(Ui *u, int act, int arg)
{
    Game *g = &u->game;
    if (act != A_END_TURN) u->end_confirm = 0;
    bool needs_sel = act == A_CAST || act == A_FLY || act == A_LAND || act == A_PICKUP || act == A_DROP ||
                     act == A_DRINK || act == A_RIDE || act == A_DISMOUNT || act == A_DOOR || act == A_MORE;
    if (needs_sel && !sel_valid(u)) {
        /* convenience: C with nothing selected picks your wizard */
        if (act == A_CAST && g->cur >= 0) {
            int w = g->pl[g->cur].wizard;
            int on = g->cr[w].mount != NONE ? g->cr[w].mount : w;
            select_creature(u, on);
        } else {
            need_selection(u);
            return;
        }
    }
    if (u->mode == M_LIST && act != A_SPELL && act != A_SPELL_PAGE) u->mode = M_PLAY;
    switch (act) {
    case A_CAST:
        if (game_wizard_caster(g, u->sel) == NONE) {
            ui_message(u, "ONLY WIZARDS CAST SPELLS");
            sfx(u, SFX_ERROR);
            break;
        }
        u->spell_page = 0;
        open_spell_book(u);
        sfx(u, SFX_SELECT);
        break;
    case A_SPELL: choose_spell(u, arg); break;
    case A_SPELL_PAGE: {
        int pages = (SP_COUNT + SPELLS_PER_PAGE - 1) / SPELLS_PER_PAGE;
        u->spell_page = (u->spell_page + (arg ? pages - 1 : 1)) % pages;
        open_spell_book(u);
        break;
    }
    case A_VIEW3D: {
        int ci = sel_valid(u) ? u->sel : NONE;
        int under = game_creature_at(g, u->cx, u->cy, false);
        if (under == NONE) under = game_creature_at(g, u->cx, u->cy, true);
        if (under != NONE && g->cr[under].owner == g->cur && u->cursor_shown) ci = under;
        if (ci == NONE) {
            need_selection(u);
            break;
        }
        enter_view3d(u, ci);
        break;
    }
    case A_FLY: game_fly(g, u->sel); sync_game(u); break;
    case A_LAND: game_land(g, u->sel); sync_game(u); break;
    case A_PICKUP: game_pickup(g, u->sel); sync_game(u); break;
    case A_DROP: game_drop(g, u->sel); sync_game(u); break;
    case A_DRINK: game_drink(g, u->sel); sync_game(u); break;
    case A_RIDE: ride_or_door(u, A_RIDE); break;
    case A_DOOR: ride_or_door(u, A_DOOR); break;
    case A_DISMOUNT: {
        int r = g->cr[u->sel].rider;
        if (game_dismount(g, u->sel) == R_OK) {
            sync_game(u);
            select_creature(u, r);
        } else {
            sync_game(u);
        }
        break;
    }
    case A_MORE: open_more(u); break;
    case A_NEXT: next_creature(u, 1); break;
    case A_PREV: next_creature(u, -1); break;
    case A_END_TURN:
        if (u->mode == M_TARGET) u->mode = M_PLAY;
        end_turn(u);
        break;
    case A_HELP: u->help = true; break;
    case A_TURN_LEFT: view_turn(u, -1); break;
    case A_TURN_RIGHT: view_turn(u, 1); break;
    case A_FORWARD: view_forward(u); break;
    case A_EXIT3D: exit_view3d(u); break;
    case A_HUMANS:
        u->humans = u->humans % 4 + 1;
        if (u->humans + u->ais > 4) u->ais = 4 - u->humans;
        if (u->humans + u->ais < 2) u->ais = 1;
        title_menu(u);
        break;
    case A_AIS: {
        int lo = u->humans == 1 ? 1 : 0, hi = 4 - u->humans;
        u->ais = u->ais + 1 > hi ? lo : u->ais + 1;
        title_menu(u);
        break;
    }
    case A_START: ui_start_game(u); break;
    case A_CONTINUE: continue_game(u); break;
    case A_MENU: open_game_menu(u); break;
    case A_QUIT: quit_to_title(u); break;
    default: /* A_CANCEL */
        if (u->mode == M_VIEW3D) exit_view3d(u);
        else if (u->mode == M_TARGET) u->mode = M_PLAY;
        else if (u->sel != NONE) {
            u->sel = NONE;
            compute_reach(u);
        } else if (u->mode == M_PLAY && arg) {
            open_game_menu(u);
        }
        break;
    }
}

/* ------------------------------------------------------- act on a square */

static bool confirm(Ui *u, bool touch, const char *what)
{
    if (!touch || (u->armed_x == u->cx && u->armed_y == u->cy)) {
        u->armed_x = -1;
        return true;
    }
    u->armed_x = u->cx;
    u->armed_y = u->cy;
    char m[64];
    snprintf(m, sizeof m, "TAP AGAIN: %s", what);
    ui_message(u, m);
    sfx(u, SFX_CURSOR);
    return false;
}

static int own_at(const Ui *u, int x, int y)
{
    const Game *g = &u->game;
    int gr = game_creature_at(g, x, y, false), air = game_creature_at(g, x, y, true);
    bool own_g = gr != NONE && g->cr[gr].owner == g->cur, own_a = air != NONE && g->cr[air].owner == g->cur;
    if (own_g && own_a) return u->sel == gr ? air : gr; /* clicking again toggles the layer */
    return own_g ? gr : (own_a ? air : NONE);
}

static void act_at_cursor(Ui *u, bool touch)
{
    Game *g = &u->game;
    int x = u->cx, y = u->cy;
    char what[64];
    if (u->mode == M_TARGET) {
        if (!ui_valid_target(u, x, y)) {
            ui_message(u, "NOT A VALID TARGET");
            sfx(u, SFX_ERROR);
            return;
        }
        snprintf(what, sizeof what, "CAST %s", SPELLS[u->pending_spell].name);
        if (!confirm(u, touch, what)) return;
        u->mode = M_PLAY;
        game_cast(g, game_wizard_caster(g, u->sel), u->pending_spell, x, y);
        sync_game(u);
        return;
    }
    int own = own_at(u, x, y);
    if (own != NONE) {
        if (own != u->sel) select_creature(u, own);
        return;
    }
    if (!sel_valid(u)) return;
    const Creature *c = &g->cr[u->sel];
    /* attack a visible enemy, walking into reach first */
    int e = game_creature_at(g, x, y, c->air);
    if (e != NONE && visible_to_cur(g, e) && g->cr[e].owner != c->owner) {
        int dx, dy, cost;
        if (!attack_plan(u, e, &dx, &dy, &cost)) {
            ui_message(u, g->turn == 1 ? "NO MOVEMENT ON THE FIRST TURN" : "OUT OF REACH");
            sfx(u, SFX_ERROR);
            return;
        }
        snprintf(what, sizeof what, "ATTACK %s", CREATURES[g->cr[e].type].name);
        if (!confirm(u, touch, what)) return;
        start_walk(u, dx, dy, e);
        return;
    }
    /* open an adjacent closed door */
    if (map_get(&g->map, x, y) == T_DOOR_CLOSED && map_dist(&g->map, c->x, c->y, x, y) == 1 && !c->air &&
        (CREATURES[c->type].flags & CF_HANDS)) {
        if (!confirm(u, touch, "OPEN DOOR")) return;
        game_use_door(g, u->sel, x, y);
        sync_game(u);
        return;
    }
    int cost = ui_reach_cost(u, x, y);
    if (cost > 0) {
        snprintf(what, sizeof what, "MOVE %d AP", cost);
        if (!confirm(u, touch, what)) return;
        start_walk(u, x, y, NONE);
        return;
    }
    if (cost < 0) {
        ui_message(u, g->turn == 1 ? "NO MOVEMENT ON THE FIRST TURN" : "CANNOT GO THERE");
        sfx(u, SFX_ERROR);
    }
}

/* ------------------------------------------------------------ hover text */

void ui_hover_text(const Ui *u, char *out, size_t n)
{
    const Game *g = &u->game;
    int viewer = ui_viewer(u);
    out[0] = 0;
    if (u->mode == M_LIST) {
        int i = u->menu.sel;
        if (u->list_spells && i < u->menu.n && u->menu.act[i] == A_SPELL) {
            const SpellInfo *s = &SPELLS[u->menu.arg[i]];
            snprintf(out, n, "%s: %s", s->name, s->desc);
        }
        return;
    }
    if (u->mode == M_VIEW3D) {
        static const char *const F[DIR_COUNT] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
        snprintf(out, n, "3D VIEW - %s FACING %s", CREATURES[g->cr[u->pov].type].name, F[u->pov_facing]);
        return;
    }
    int x = u->cx, y = u->cy;
    const char *prefix = u->touch && u->armed_x == x && u->armed_y == y ? "TAP AGAIN: " : "";
    if (u->mode == M_TARGET) {
        if (ui_valid_target(u, x, y)) snprintf(out, n, "%sCAST %s HERE", prefix, SPELLS[u->pending_spell].name);
        else snprintf(out, n, "%s: NOT A VALID TARGET", SPELLS[u->pending_spell].name);
        return;
    }
    if (!u->cursor_shown) return;
    if (sel_valid(u)) {
        const Creature *c = &g->cr[u->sel];
        int own = own_at(u, x, y);
        if (own != NONE && own != u->sel) {
            snprintf(out, n, "SELECT %s", CREATURES[g->cr[own].type].name);
            return;
        }
        int e = game_creature_at(g, x, y, c->air);
        int cost;
        if (e != NONE && visible_to_cur(g, e) && g->cr[e].owner != c->owner) {
            if (ui_attackable(u, e, &cost)) snprintf(out, n, "%sATTACK %s: %d AP", prefix, CREATURES[g->cr[e].type].name, cost);
            else snprintf(out, n, "%s (OUT OF REACH)", CREATURES[g->cr[e].type].name);
            return;
        }
        if (map_get(&g->map, x, y) == T_DOOR_CLOSED && map_dist(&g->map, c->x, c->y, x, y) == 1 && !c->air &&
            (CREATURES[c->type].flags & CF_HANDS)) {
            snprintf(out, n, "%sOPEN DOOR: 2 AP", prefix);
            return;
        }
        cost = ui_reach_cost(u, x, y);
        if (cost > 0) {
            snprintf(out, n, "%sMOVE: %d AP", prefix, cost);
            return;
        }
    }
    /* plain description of the square */
    for (int layer = 1; layer >= 0; layer--) {
        int ci = game_creature_at(g, x, y, layer == 1);
        if (ci != NONE && (viewer < 0 || game_can_see_creature(g, viewer, ci))) {
            snprintf(out, n, "%s%s", CREATURES[g->cr[ci].type].name, layer ? " (FLYING)" : "");
            return;
        }
    }
    bool vis = viewer < 0 || g->pl[viewer].vis[y][x];
    bool seen = viewer < 0 || g->pl[viewer].seen[y][x];
    int oi = vis ? game_object_at(g, x, y) : NONE;
    if (oi != NONE) snprintf(out, n, "%s", OBJECTS[g->obj[oi].type].name);
    else if (seen) snprintf(out, n, "%s", TERRAIN[map_get(&g->map, x, y)].name);
}

int ui_panel_creature(const Ui *u)
{
    const Game *g = &u->game;
    int viewer = ui_viewer(u);
    if (u->mode == M_VIEW3D) return u->pov;
    if (u->cursor_shown) {
        for (int layer = 1; layer >= 0; layer--) {
            int ci = game_creature_at(g, u->cx, u->cy, layer == 1);
            if (ci != NONE && (viewer < 0 || game_can_see_creature(g, viewer, ci))) return ci;
        }
    }
    return sel_valid(u) ? u->sel : NONE;
}

/* ------------------------------------------------------------------ input */

static void drag_map(Ui *u, int dx, int dy)
{
    const Map *m = &u->game.map;
    u->drag_x += dx;
    u->drag_y += dy;
    while (u->drag_x >= 16) { u->vx = map_wx(m, u->vx - 1); u->drag_x -= 16; }
    while (u->drag_x <= -16) { u->vx = map_wx(m, u->vx + 1); u->drag_x += 16; }
    while (u->drag_y >= 16) { u->vy = map_wy(m, u->vy - 1); u->drag_y -= 16; }
    while (u->drag_y <= -16) { u->vy = map_wy(m, u->vy + 1); u->drag_y += 16; }
    u->armed_x = -1;
}

static bool press_button(Ui *u, int x, int y)
{
    Button b[MAX_BUTTONS];
    int n = ui_buttons(u, b);
    for (int i = 0; i < n; i++) {
        if (rect_has(&b[i].r, x, y)) {
            if (b[i].enabled) {
                sfx(u, SFX_SELECT);
                do_action(u, b[i].act, 0);
            }
            return true;
        }
    }
    return false;
}

static void list_choose(Ui *u, int i)
{
    Menu *m = &u->menu;
    if (i < 0 || i >= m->n) return;
    m->sel = i;
    if (m->disabled[i]) {
        sfx(u, SFX_ERROR);
        ui_message(u, "CANNOT DO THAT NOW");
        return;
    }
    sfx(u, SFX_SELECT);
    do_action(u, m->act[i], m->arg[i]);
}

static int list_hit(const Ui *u, int x, int y)
{
    for (int i = 0; i < u->menu.n; i++) {
        Rect r = ui_menu_item_rect(u, i);
        if (rect_has(&r, x, y)) return i;
    }
    return -1;
}

static void list_input(Ui *u, Input in)
{
    Menu *m = &u->menu;
    int hit;
    switch (in.kind) {
    case IN_UP: m->sel = (m->sel + m->n - 1) % m->n; sfx(u, SFX_CURSOR); break;
    case IN_DOWN: m->sel = (m->sel + 1) % m->n; sfx(u, SFX_CURSOR); break;
    case IN_FIRE: list_choose(u, m->sel); break;
    case IN_DIGIT:
        if (u->mode == M_LIST && u->list_spells && in.x >= 1 && in.x <= m->n - 2) list_choose(u, in.x - 1);
        break;
    case IN_WHEEL:
        if (u->mode == M_LIST && u->list_spells) do_action(u, A_SPELL_PAGE, in.y < 0);
        break;
    case IN_POINT:
        hit = list_hit(u, in.x, in.y);
        if (hit >= 0) m->sel = hit;
        break;
    case IN_CLICK:
    case IN_TAP:
        hit = list_hit(u, in.x, in.y);
        if (hit >= 0) list_choose(u, hit);
        else if (u->mode == M_LIST) u->mode = M_PLAY;
        break;
    case IN_CANCEL:
        if (u->mode == M_LIST) u->mode = M_PLAY;
        break;
    case IN_CAST: case IN_VIEW: case IN_FLY: case IN_PICKUP: case IN_DROP: case IN_DRINK:
    case IN_USE: case IN_RIDE: case IN_END_TURN: case IN_NEXT: case IN_PREV: case IN_HELP:
        if (u->mode == M_LIST) {
            u->mode = M_PLAY;
            ui_input(u, in);
        }
        break;
    default: break;
    }
}

static const uint8_t HOTKEY_ACTION[] = {
    [IN_NEXT] = A_NEXT, [IN_PREV] = A_PREV, [IN_VIEW] = A_VIEW3D, [IN_CAST] = A_CAST, [IN_FLY] = A_FLY,
    [IN_PICKUP] = A_PICKUP, [IN_DROP] = A_DROP, [IN_DRINK] = A_DRINK, [IN_USE] = A_DOOR,
    [IN_RIDE] = A_RIDE, [IN_END_TURN] = A_END_TURN, [IN_HELP] = A_HELP,
};

static void play_input(Ui *u, Input in)
{
    int d = dir_of_input(in.kind);
    if (d >= 0) {
        set_cursor(u, u->cx + DIR_DX[d], u->cy + DIR_DY[d]);
        u->cursor_shown = true;
        u->armed_x = -1;
        sfx(u, SFX_CURSOR);
        return;
    }
    switch (in.kind) {
    case IN_FIRE:
        u->cursor_shown = true;
        act_at_cursor(u, false);
        break;
    case IN_CANCEL: do_action(u, A_CANCEL, 1); break; /* with nothing selected: game menu */
    case IN_FLY: {
        bool air = sel_valid(u) && u->game.cr[u->sel].air;
        do_action(u, air ? A_LAND : A_FLY, 0);
        break;
    }
    case IN_RIDE:
        do_action(u, sel_valid(u) && u->game.cr[u->sel].rider != NONE ? A_DISMOUNT : A_RIDE, 0);
        break;
    case IN_NEXT: case IN_PREV: case IN_VIEW: case IN_CAST: case IN_PICKUP: case IN_DROP:
    case IN_DRINK: case IN_USE: case IN_END_TURN: case IN_HELP:
        do_action(u, HOTKEY_ACTION[in.kind], 0);
        break;
    case IN_WHEEL: do_action(u, in.y > 0 ? A_NEXT : A_PREV, 0); break;
    case IN_DRAG: drag_map(u, in.x, in.y); break;
    case IN_POINT:
        if (in_map(in.x, in.y)) {
            int mx, my;
            screen_to_square(u, in.x, in.y, &mx, &my);
            /* a new square under the pointer: let its description replace old news soon */
            if ((mx != u->cx || my != u->cy) && u->msg_timer > 30) u->msg_timer = 30;
            u->cx = mx;
            u->cy = my;
            u->cursor_shown = true;
        } else {
            u->cursor_shown = false;
        }
        break;
    case IN_CLICK:
    case IN_TAP:
        if (press_button(u, in.x, in.y)) break;
        if (in_map(in.x, in.y)) {
            int mx, my;
            screen_to_square(u, in.x, in.y, &mx, &my);
            u->cx = mx;
            u->cy = my;
            u->cursor_shown = true;
            act_at_cursor(u, in.kind == IN_TAP);
        }
        break;
    default: break;
    }
}

static void view_input(Ui *u, Input in)
{
    switch (in.kind) {
    case IN_LEFT: case IN_UPLEFT: case IN_DOWNLEFT: view_turn(u, -1); break;
    case IN_RIGHT: case IN_UPRIGHT: case IN_DOWNRIGHT: view_turn(u, 1); break;
    case IN_DOWN: view_turn(u, 4); break;
    case IN_UP: view_forward(u); break;
    case IN_WHEEL: view_turn(u, in.y > 0 ? 1 : -1); break;
    case IN_NEXT: next_creature(u, 1); break;
    case IN_PREV: next_creature(u, -1); break;
    case IN_END_TURN: do_action(u, A_END_TURN, 0); break;
    case IN_HELP: u->help = true; break;
    case IN_DRAG:
        u->view_drag += in.x;
        if (u->view_drag >= 24) { view_turn(u, -1); u->view_drag = 0; }
        if (u->view_drag <= -24) { view_turn(u, 1); u->view_drag = 0; }
        break;
    case IN_CLICK:
    case IN_TAP: {
        if (press_button(u, in.x, in.y)) break;
        int x = in.x - VIEW_X, y = in.y - VIEW_Y;
        if (x < 0 || y < 0 || x >= VIEW_PX || y >= VIEW_PX) break;
        if (x < VIEW_PX / 3) view_turn(u, -1);
        else if (x >= VIEW_PX * 2 / 3) view_turn(u, 1);
        else view_forward(u);
        break;
    }
    case IN_FIRE: case IN_CANCEL: case IN_VIEW: exit_view3d(u); break;
    default: break;
    }
}

void ui_init(Ui *u, uint64_t seed)
{
    memset(u, 0, sizeof *u);
    u->seed = seed;
    u->humans = 1;
    u->ais = 1;
    u->sel = NONE;
    u->pov = NONE;
    u->armed_x = -1;
    u->walk_attack = NONE;
    u->mode = M_TITLE;
    u->menu.sel = 2;
    title_menu(u);
}

void ui_input(Ui *u, Input in)
{
    if (in.kind == IN_TAP) u->touch = true;
    if (in.kind == IN_CLICK || (in.kind >= IN_UP && in.kind <= IN_DIGIT)) u->touch = false;
    if (in.kind == IN_CLICK || in.kind == IN_TAP || in.kind == IN_POINT) {
        u->px = in.x;
        u->py = in.y;
    }
    if (ui_walking(u) && in.kind != IN_POINT) finish_walk(u);
    if (u->help) {
        if (in.kind != IN_POINT && in.kind != IN_DRAG) u->help = false;
        return;
    }
    switch (u->mode) {
    case M_TITLE: list_input(u, in); break;
    case M_LIST: list_input(u, in); break;
    case M_HANDOVER:
        if (in.kind == IN_FIRE || in.kind == IN_TAP || in.kind == IN_CLICK) {
            u->mode = M_PLAY;
            sfx(u, SFX_SELECT);
        }
        break;
    case M_GAMEOVER:
        if (in.kind == IN_FIRE || in.kind == IN_TAP || in.kind == IN_CLICK) ui_init(u, u->seed + 1);
        break;
    case M_VIEW3D: view_input(u, in); break;
    default: play_input(u, in); break;
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
    if (u->end_confirm > 0) u->end_confirm--;
    if (ui_walking(u) && --u->walk_timer <= 0) {
        walk_step(u);
        u->walk_timer = WALK_TICKS;
    }
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
