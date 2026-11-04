/* Draws every screen of the game from the UI state. */
#include "art.h"
#include "palette.h"
#include "raycast.h"
#include "ui.h"
#include <stdio.h>
#include <string.h>

static const uint8_t OWNER_COLOURS[MAX_PLAYERS] = {C_YELLOW, C_CYAN, C_LGREEN, C_LRED};

static int owner_colour(int owner) { return owner >= 0 ? OWNER_COLOURS[owner] : C_LGREY; }

static const char *const FACING_NAMES[DIR_COUNT] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};

/* ------------------------------------------------------------ frame/panel */

static void draw_frames(Framebuffer *fb, int border)
{
    fb_clear(fb, C_BLACK);
    /* outer border like an 8-bit screen border */
    fb_frame(fb, 0, 0, SCREEN_W, SCREEN_H, (uint8_t)border);
    fb_frame(fb, 1, 1, SCREEN_W - 2, SCREEN_H - 2, (uint8_t)border);
    /* map window frame */
    fb_frame(fb, VIEW_X - 3, VIEW_Y - 3, VIEW_PX + 6, VIEW_PX + 6, C_LBLUE);
    fb_frame(fb, VIEW_X - 2, VIEW_Y - 2, VIEW_PX + 4, VIEW_PX + 4, C_BLUE);
    fb_frame(fb, VIEW_X - 1, VIEW_Y - 1, VIEW_PX + 2, VIEW_PX + 2, C_PURPLE);
}

static void draw_icon(Framebuffer *fb, int icon, int x, int y, uint8_t c)
{
    const char *const *ic = art_icon(icon);
    for (int j = 0; j < ICON; j++)
        for (int i = 0; i < ICON; i++)
            if (ic[j][i] != '.') fb_pixel(fb, x + i, y + j, c);
}

static void draw_bar(Framebuffer *fb, int x, int value, int max, uint8_t colour, int icon)
{
    const int top = 66, h = 40;
    fb_frame(fb, x - 1, top - 1, 12, h + 2, C_DGREY);
    int fill = max > 0 ? clampi(value * h / max, 0, h) : 0;
    fb_rect(fb, x, top + h - fill, 10, fill, colour);
    fb_rect(fb, x - 1, top + h + 2, 12, 10, colour);
    draw_icon(fb, icon, x + 1, top + h + 3, C_BLACK);
}

static void draw_creature_panel(Framebuffer *fb, const Game *g, int ci)
{
    const Creature *c = &g->cr[ci];
    const CreatureInfo *info = &CREATURES[c->type];
    int x0 = PANEL_X + 4;
    bool st[5] = {
        (info->flags & CF_UNDEAD) != 0, (info->flags & CF_FLY) != 0, (info->flags & CF_MOUNT) != 0,
        game_wounded(g, ci), c->invisible > 0,
    };
    static const uint8_t on_col[5] = {C_LRED, C_LBLUE, C_YELLOW, C_RED, C_PURPLE};
    for (int i = 0; i < 5; i++) draw_icon(fb, IC_UNDEAD + i, x0 + 4 + i * 24, 4, st[i] ? on_col[i] : C_DGREY);
    fb_text(fb, x0, 14, info->name, (uint8_t)owner_colour(c->owner));
    fb_text(fb, x0, 23, c->owner >= 0 ? g->pl[c->owner].name : "INDEPENDENT", C_GREY);
    if (c->air) fb_text(fb, x0 + 98, 23, "AIR", C_LBLUE);
    for (int i = 0; i < MAX_CARRY; i++) {
        int bx = x0 + i * 30;
        fb_frame(fb, bx, 32, 20, 20, C_PURPLE);
        fb_frame(fb, bx + 1, 33, 18, 18, C_BLUE);
        if (i < c->ninv) fb_sprite(fb, art_object(c->inv[i]), SPR, bx + 2, 34, 1, C_WHITE, 0, NULL);
    }
    char line[32];
    snprintf(line, sizeof line, "AP %d/%d", c->ap, game_max_ap(g, ci));
    fb_text(fb, x0, 55, line, C_LGREEN);
    if (game_is_wizard(c)) {
        snprintf(line, sizeof line, "MANA %d", c->mana);
        fb_text(fb, x0 + 66, 55, line, C_PURPLE);
    } else if (c->rider != NONE) {
        fb_text(fb, x0 + 66, 55, "+RIDER", C_GREY);
    }
    int bx = PANEL_X + 10;
    draw_bar(fb, bx, c->ap, imax(1, game_max_ap(g, ci)), C_LGREEN, IC_AP);
    draw_bar(fb, bx + 22, c->stamina, info->stamina, C_YELLOW, IC_STAMINA);
    draw_bar(fb, bx + 44, c->con, info->con, C_LRED, IC_CON);
    draw_bar(fb, bx + 66, game_combat(g, ci), 15, C_CYAN, IC_COMBAT);
    draw_bar(fb, bx + 88, game_defence(g, ci), 15, C_WHITE, IC_DEFENCE);
}

static void draw_square_panel(Framebuffer *fb, const Ui *u, int viewer)
{
    const Game *g = &u->game;
    int x0 = PANEL_X + 4;
    char line[32];
    snprintf(line, sizeof line, "GAME TURN %d", g->turn);
    fb_text(fb, x0, 6, line, C_WHITE);
    if (g->cur >= 0) fb_text(fb, x0, 16, g->pl[g->cur].name, (uint8_t)owner_colour(g->cur));
    for (int p = 0; p < g->npl; p++) {
        snprintf(line, sizeof line, "%-9s %3d VP", g->pl[p].name, g->pl[p].vp);
        fb_text(fb, x0, 28 + p * 9, line, g->pl[p].alive ? (uint8_t)owner_colour(p) : C_DGREY);
    }
    bool seen = viewer < 0 || g->pl[viewer].seen[u->cy][u->cx];
    bool vis = viewer < 0 || g->pl[viewer].vis[u->cy][u->cx];
    if (u->cursor_shown) fb_text(fb, x0, 68, seen ? TERRAIN[map_get(&g->map, u->cx, u->cy)].name : "UNKNOWN", C_GREY);
    int n = 0;
    for (int k = 0; k < g->nobj && vis && u->cursor_shown && n < 4; k++) {
        const Object *o = &g->obj[k];
        if (!o->used || o->x != u->cx || o->y != u->cy) continue;
        fb_sprite(fb, art_object(o->type), SPR, x0 + n * 22, 78, 1, C_WHITE, 0, NULL);
        n++;
    }
    if (g->portal_open) {
        fb_text(fb, x0, 100, "THE PORTAL IS OPEN", C_PURPLE);
    } else {
        snprintf(line, sizeof line, "PORTAL IN %d TURNS", g->portal_turn - g->turn);
        fb_text(fb, x0, 100, line, C_DGREY);
    }
}

static void draw_buttons(Framebuffer *fb, const Ui *u)
{
    Button b[MAX_BUTTONS];
    int n = ui_buttons(u, b);
    for (int i = 0; i < n; i++) {
        bool hover = !u->touch && rect_has(&b[i].r, u->px, u->py);
        uint8_t bg = !b[i].enabled ? C_DGREY : (hover ? C_LBLUE : C_BLUE);
        if (b[i].act == A_END_TURN && u->end_confirm > 0) bg = (u->tick / 8) % 2 ? C_RED : C_LRED;
        fb_rect(fb, b[i].r.x, b[i].r.y, b[i].r.w, b[i].r.h, bg);
        fb_frame(fb, b[i].r.x, b[i].r.y, b[i].r.w, b[i].r.h, hover ? C_WHITE : C_LBLUE);
        bool keys = !u->touch && b[i].key[0];
        /* with a keyboard the hotkey sits in the top-right corner, the label below it */
        fb_text(fb, b[i].r.x + 3, b[i].r.y + (keys ? 11 : 7), b[i].label,
                b[i].enabled ? (hover ? C_BLACK : C_WHITE) : C_GREY);
        if (keys) {
            int kx = b[i].r.x + b[i].r.w - 2 - fb_text_width(b[i].key);
            fb_text(fb, kx, b[i].r.y + 2, b[i].key, hover ? C_BLUE : C_LBLUE);
        }
    }
}

static void draw_list(Framebuffer *fb, const Ui *u)
{
    const Menu *m = &u->menu;
    fb_rect(fb, PANEL_X, 3, SCREEN_W - PANEL_X - 3, 183, C_BLACK);
    fb_frame(fb, PANEL_X, 3, SCREEN_W - PANEL_X - 3, 183, C_PURPLE);
    fb_text(fb, PANEL_X + 4, 6, m->title, C_WHITE);
    for (int i = 0; i < m->n; i++) {
        Rect r = ui_menu_item_rect(u, i);
        bool hi = i == m->sel;
        if (hi) fb_rect(fb, r.x, r.y, r.w, r.h, C_LBLUE);
        uint8_t col = m->disabled[i] ? C_RED : (u->list_spells ? C_YELLOW : C_WHITE);
        if (m->act[i] == A_CANCEL) col = C_GREY;
        if (hi && !m->disabled[i]) col = C_BLACK;
        fb_text(fb, r.x + 2, r.y + 4, m->label[i], col);
    }
}

/* -------------------------------------------------------------- map view */

static void dotted_frame(Framebuffer *fb, int x, int y, uint8_t c, int phase)
{
    for (int i = 1; i < 15; i++) {
        if ((i + phase) % 2) continue;
        fb_pixel(fb, x + i, y + 1, c);
        fb_pixel(fb, x + i, y + 14, c);
        fb_pixel(fb, x + 1, y + i, c);
        fb_pixel(fb, x + 14, y + i, c);
    }
}

static void draw_overlays(Framebuffer *fb, const Ui *u, int mx, int my, int sx, int sy)
{
    const Game *g = &u->game;
    if (u->mode == M_TARGET) {
        if (ui_valid_target(u, mx, my)) dotted_frame(fb, sx, sy, C_YELLOW, 0);
        return;
    }
    if (u->mode != M_PLAY || ui_walking(u) || u->sel == NONE || g->cur < 0) return;
    const Creature *s = &g->cr[u->sel];
    if (!s->alive || s->owner != g->cur || s->mount != NONE) return;
    int e = game_creature_at(g, mx, my, s->air), cost;
    if (e != NONE && ui_attackable(u, e, &cost)) {
        dotted_frame(fb, sx, sy, C_LRED, 0);
        dotted_frame(fb, sx, sy, C_LRED, 1);
    } else if (ui_reach_cost(u, mx, my) > 0) {
        dotted_frame(fb, sx, sy, C_LGREEN, 0);
    }
}

static void draw_path(Framebuffer *fb, const Ui *u)
{
    const Game *g = &u->game;
    if (u->mode != M_PLAY || ui_walking(u) || !u->cursor_shown || u->sel == NONE) return;
    if (ui_reach_cost(u, u->cx, u->cy) <= 0) return;
    int x = u->cx, y = u->cy;
    for (int guard = 0; guard < MAX_PATH; guard++) {
        const Creature *c = &g->cr[u->sel];
        if (x == c->x && y == c->y) break;
        int sx, sy;
        if (ui_map_to_screen(u, x, y, &sx, &sy)) fb_rect(fb, sx - 1, sy - 1, 3, 3, C_YELLOW);
        int d = u->reach_dir[y][x];
        x = map_wx(&g->map, x - DIR_DX[d]);
        y = map_wy(&g->map, y - DIR_DY[d]);
    }
}

static void draw_map(Framebuffer *fb, const Ui *u, int viewer)
{
    const Game *g = &u->game;
    const Map *m = &g->map;
    Rect clip = {VIEW_X, VIEW_Y, VIEW_PX, VIEW_PX};
    for (int j = 0; j < VIEW_TILES; j++) {
        for (int i = 0; i < VIEW_TILES; i++) {
            int mx = map_wx(m, u->vx + i), my = map_wy(m, u->vy + j);
            int sx = VIEW_X + i * 16, sy = VIEW_Y + j * 16;
            bool seen = viewer < 0 || g->pl[viewer].seen[my][mx];
            bool vis = viewer < 0 || g->pl[viewer].vis[my][mx];
            if (!seen) continue;
            Terrain t = map_get(m, mx, my);
            fb_sprite(fb, art_terrain(t), SPR, sx, sy, 1, C_WHITE, vis ? 0 : 1, &clip);
            if (t == T_PORTAL && (u->tick / 8) % 2) fb_rect(fb, sx + 7, sy + 7, 2, 2, C_WHITE);
            if (!vis) continue;
            int oi = game_object_at(g, mx, my);
            if (oi != NONE) fb_sprite(fb, art_object(g->obj[oi].type), SPR, sx, sy, 1, C_WHITE, 0, &clip);
            for (int layer = 0; layer < 2; layer++) {
                int ci = game_creature_at(g, mx, my, layer == 1);
                if (ci == NONE || (viewer >= 0 && !game_can_see_creature(g, viewer, ci))) continue;
                const Creature *c = &g->cr[ci];
                int oy = layer ? -4 : 0;
                if (layer) fb_rect(fb, sx + 4, sy + 13, 8, 2, C_DGREY); /* shadow */
                bool blink = c->invisible > 0 && (u->tick / 6) % 2;
                if (!blink) fb_sprite(fb, art_creature(c->type), SPR, sx, sy + oy, 1, owner_colour(c->owner), 0, &clip);
                if (c->owner >= 0) fb_rect(fb, sx + 13, sy + 13, 3, 3, (uint8_t)owner_colour(c->owner));
                if (c->rider != NONE) fb_rect(fb, sx, sy + 13, 3, 3, C_WHITE);
            }
            draw_overlays(fb, u, mx, my, sx, sy);
        }
    }
    draw_path(fb, u);
    int sx, sy;
    /* selection marker */
    if (u->sel != NONE && g->cr[u->sel].alive && ui_map_to_screen(u, g->cr[u->sel].x, g->cr[u->sel].y, &sx, &sy)) {
        fb_frame(fb, sx - 8, sy - 8, 16, 16, C_LGREEN);
        fb_frame(fb, sx - 7, sy - 7, 14, 14, C_GREEN);
    }
    /* cursor */
    if ((u->cursor_shown || u->mode == M_TARGET) && ui_map_to_screen(u, u->cx, u->cy, &sx, &sy)) {
        uint8_t col = C_WHITE;
        if (u->mode == M_TARGET) col = ui_valid_target(u, u->cx, u->cy) ? C_YELLOW : C_RED;
        bool armed = u->armed_x == u->cx && u->armed_y == u->cy;
        if (armed) col = (u->tick / 6) % 2 ? C_YELLOW : C_WHITE;
        fb_frame(fb, sx - 8, sy - 8, 16, 16, col);
        if (armed || (u->tick / 10) % 3 != 0) fb_frame(fb, sx - 7, sy - 7, 14, 14, col);
    }
}

/* -------------------------------------------------------------- overlays */

static const char *const HELP_DESKTOP[] = {
    "MOUSE",
    "CLICK CREATURE  SELECT",
    "CLICK SQUARE    MOVE/ATTACK",
    "RIGHT CLICK     CANCEL",
    "DRAG            SCROLL MAP",
    "WHEEL           NEXT ONE",
    "",
    "KEYBOARD",
    "ARROWS/WASD CURSOR",
    "ENTER ACT   ESC CANCEL",
    "TAB NEXT    C CAST",
    "V 3D VIEW   F FLY/LAND",
    "G GET  X DROP  Q DRINK",
    "U DOOR  R RIDE  T END TURN",
    "1-9 SPELLS  H HELP",
};

static const char *const HELP_TOUCH[] = {
    "TOUCH",
    "TAP CREATURE  SELECT",
    "TAP SQUARE    SHOW MOVE",
    "TAP AGAIN     DO IT",
    "DRAG          SCROLL MAP",
    "BUTTONS       ACTIONS",
    "",
    "3D VIEW",
    "SWIPE         TURN",
    "TAP SIDES     TURN",
    "TAP MIDDLE    STEP",
    "",
    "GREEN: CAN MOVE THERE",
    "RED: CAN ATTACK",
    "TAP TO CLOSE",
};

static void draw_help(Framebuffer *fb, const Ui *u)
{
    const char *const *lines = u->touch ? HELP_TOUCH : HELP_DESKTOP;
    int n = 15;
    fb_rect(fb, VIEW_X + 2, VIEW_Y + 2, VIEW_PX - 4, VIEW_PX - 4, C_BLACK);
    fb_frame(fb, VIEW_X + 2, VIEW_Y + 2, VIEW_PX - 4, VIEW_PX - 4, C_YELLOW);
    for (int i = 0; i < n; i++) {
        bool head = i == 0 || (i > 0 && lines[i - 1][0] == 0);
        fb_text(fb, VIEW_X + 6, VIEW_Y + 6 + i * 11, lines[i], head ? C_YELLOW : C_LGREY);
    }
}

/* ---------------------------------------------------------------- screens */

static void draw_title(Framebuffer *fb, const Ui *u)
{
    fb_clear(fb, C_BLACK);
    for (int i = 0; i < 90; i++) {
        int x = (i * 97 + 13) % SCREEN_W, y = (i * 53 + 7) % 120;
        fb_pixel(fb, x, y, (i + (int)(u->tick / 20)) % 7 == 0 ? C_WHITE : C_GREY);
    }
    fb_text_scaled(fb, 24 + 3, 18 + 3, "MAGES", C_BLUE, 6);
    fb_text_scaled(fb, 24, 18, "MAGES", C_PURPLE, 6);
    fb_text(fb, 24, 70, "A TURN-BASED WIZARD BATTLE", C_LGREY);
    fb_rect(fb, 0, 120, SCREEN_W, 80, C_GREEN);
    for (int x = 0; x < SCREEN_W; x += 4) fb_rect(fb, x, 118 + ((x / 4) % 3), 4, 3, C_GREEN);
    for (int p = 0; p < MAX_PLAYERS; p++) {
        fb_sprite(fb, art_creature(CR_WIZARD), SPR, 14 + p * 40, 92 + (p % 2) * 6, 2, OWNER_COLOURS[p], 0, NULL);
    }
    fb_sprite(fb, art_creature(CR_DRAGON), SPR, 236, 40 + (int)((u->tick / 30) % 4), 4, C_WHITE, 0, NULL);
    fb_rect(fb, 186, 128, 126, 50, C_BLACK);
    fb_frame(fb, 186, 128, 126, 50, C_LBLUE);
    fb_frame(fb, 187, 129, 124, 48, C_BLUE);
    for (int i = 0; i < u->menu.n; i++) {
        Rect r = ui_menu_item_rect(u, i);
        if (i == u->menu.sel) fb_rect(fb, r.x, r.y, r.w, r.h, C_LBLUE);
        fb_text(fb, r.x + 4, r.y + 4, u->menu.label[i], i == u->menu.sel ? C_BLACK : C_WHITE);
    }
    fb_text(fb, 8, 182, "ORIGINAL GAME - CLICK OR TAP TO CHOOSE", C_YELLOW);
}

static void draw_scores(Framebuffer *fb, const Game *g, int y)
{
    char line[40];
    for (int p = 0; p < g->npl; p++) {
        snprintf(line, sizeof line, "%-10s %4d VP %s", g->pl[p].name, g->pl[p].vp,
                 g->pl[p].alive ? "" : "(FALLEN)");
        fb_text(fb, 80, y + p * 12, line, g->pl[p].alive ? (uint8_t)owner_colour(p) : C_DGREY);
    }
}

static void draw_handover(Framebuffer *fb, const Ui *u)
{
    const Game *g = &u->game;
    fb_clear(fb, C_BLACK);
    fb_frame(fb, 40, 30, 240, 140, C_PURPLE);
    fb_frame(fb, 42, 32, 236, 136, C_BLUE);
    char line[40];
    snprintf(line, sizeof line, "GAME TURN %d", g->turn);
    fb_text_centre(fb, SCREEN_W / 2, 44, line, C_WHITE);
    fb_text_scaled(fb, SCREEN_W / 2 - fb_text_width(g->pl[g->cur].name), 60, g->pl[g->cur].name,
                   (uint8_t)owner_colour(g->cur), 2);
    fb_text_centre(fb, SCREEN_W / 2, 82, "IT IS YOUR TURN", C_LGREY);
    draw_scores(fb, g, 100);
    if ((u->tick / 20) % 2) fb_text_centre(fb, SCREEN_W / 2, 154, "CLICK, TAP OR PRESS ENTER", C_YELLOW);
}

static void draw_gameover(Framebuffer *fb, const Ui *u)
{
    const Game *g = &u->game;
    fb_clear(fb, C_BLACK);
    fb_frame(fb, 20, 20, 280, 160, C_YELLOW);
    if (g->winner != NONE) {
        fb_text_scaled(fb, SCREEN_W / 2 - fb_text_width("VICTORY") * 3 / 2, 34, "VICTORY", C_YELLOW, 3);
        fb_sprite(fb, art_creature(CR_WIZARD), SPR, SCREEN_W / 2 - 16, 62, 2, owner_colour(g->winner), 0, NULL);
    } else {
        fb_text_scaled(fb, SCREEN_W / 2 - fb_text_width("CHAOS") * 3 / 2, 34, "CHAOS", C_LRED, 3);
    }
    fb_text_centre(fb, SCREEN_W / 2, 100, g->msg, C_WHITE);
    draw_scores(fb, g, 116);
    if ((u->tick / 20) % 2) fb_text_centre(fb, SCREEN_W / 2, 166, "CLICK OR TAP FOR A NEW GAME", C_LGREY);
}

static void draw_view3d(Framebuffer *fb, const Ui *u, int viewer)
{
    Rect r = {VIEW_X, VIEW_Y, VIEW_PX, VIEW_PX};
    raycast_render(fb, &u->game, viewer, u->pov, u->cam_x, u->cam_y, u->cam_ang, r, u->tick);
    fb_text(fb, VIEW_X + 3, VIEW_Y + VIEW_PX / 2 - 3, "<", C_WHITE);
    fb_text(fb, VIEW_X + VIEW_PX - 8, VIEW_Y + VIEW_PX / 2 - 3, ">", C_WHITE);
    fb_text_centre(fb, VIEW_X + VIEW_PX / 2, VIEW_Y + 3, FACING_NAMES[u->pov_facing], C_WHITE);
}

void ui_render(const Ui *u, Framebuffer *fb)
{
    if (u->mode == M_TITLE) { draw_title(fb, u); return; }
    if (u->mode == M_HANDOVER) { draw_handover(fb, u); return; }
    if (u->mode == M_GAMEOVER) { draw_gameover(fb, u); return; }
    int viewer = ui_viewer(u);
    draw_frames(fb, u->fx_timer > 0 ? C_RED : C_BLUE);
    if (u->mode == M_VIEW3D) draw_view3d(fb, u, viewer);
    else draw_map(fb, u, viewer);
    if (u->help) draw_help(fb, u);
    if (u->mode == M_LIST) {
        draw_list(fb, u);
    } else {
        int ci = ui_panel_creature(u);
        if (ci != NONE) draw_creature_panel(fb, &u->game, ci);
        else draw_square_panel(fb, u, viewer);
        draw_buttons(fb, u);
    }
    char line[64];
    if (u->msg_timer > 0) snprintf(line, sizeof line, "%s", u->msg);
    else ui_hover_text(u, line, sizeof line);
    fb_text(fb, 6, MSG_Y, line, C_YELLOW);
}
