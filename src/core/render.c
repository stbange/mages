/* Draws every screen of the game from the UI state. */
#include "art.h"
#include "palette.h"
#include "raycast.h"
#include "ui.h"
#include <stdio.h>

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

static void draw_bar(Framebuffer *fb, int x, int value, int max, uint8_t colour, int icon)
{
    const int top = 92, h = 72;
    fb_frame(fb, x - 1, top - 1, 12, h + 2, C_DGREY);
    int fill = max > 0 ? clampi(value * h / max, 0, h) : 0;
    fb_rect(fb, x, top + h - fill, 10, fill, colour);
    fb_rect(fb, x - 1, top + h + 3, 12, 10, colour);
    fb_sprite(fb, art_icon(icon), ICON, x + 1, top + h + 4, 1, C_BLACK, 0, NULL);
    /* icon pixels are white; draw them black on the coloured tile */
    for (int j = 0; j < ICON; j++)
        for (int i = 0; i < ICON; i++)
            if (art_icon(icon)[j][i] != '.') fb->px[top + h + 4 + j][x + 1 + i] = C_BLACK;
}

static void draw_creature_panel(Framebuffer *fb, const Game *g, int ci)
{
    const Creature *c = &g->cr[ci];
    const CreatureInfo *info = &CREATURES[c->type];
    int x0 = PANEL_X + 4;
    /* status icons */
    bool st[5] = {
        (info->flags & CF_UNDEAD) != 0, (info->flags & CF_FLY) != 0, (info->flags & CF_MOUNT) != 0,
        game_wounded(g, ci), c->invisible > 0,
    };
    static const uint8_t on_col[5] = {C_LRED, C_LBLUE, C_YELLOW, C_RED, C_PURPLE};
    for (int i = 0; i < 5; i++) {
        const char *const *ic = art_icon(IC_UNDEAD + i);
        for (int j = 0; j < ICON; j++)
            for (int k = 0; k < ICON; k++)
                if (ic[j][k] != '.') fb_pixel(fb, x0 + 4 + i * 24 + k, 4 + j, st[i] ? on_col[i] : C_DGREY);
    }
    fb_text(fb, x0, 16, info->name, (uint8_t)owner_colour(c->owner));
    fb_text(fb, x0, 25, c->owner >= 0 ? g->pl[c->owner].name : "INDEPENDENT", C_GREY);
    if (c->air) fb_text(fb, x0 + 84, 25, "AIR", C_LBLUE);
    /* carried objects */
    for (int i = 0; i < MAX_CARRY; i++) {
        int bx = x0 + i * 30;
        fb_frame(fb, bx, 35, 20, 20, C_PURPLE);
        fb_frame(fb, bx + 1, 36, 18, 18, C_BLUE);
        if (i < c->ninv) fb_sprite(fb, art_object(c->inv[i]), SPR, bx + 2, 37, 1, C_WHITE, 0, NULL);
    }
    char line[32];
    if (game_is_wizard(c)) {
        snprintf(line, sizeof line, "MANA %d", c->mana);
        fb_text(fb, x0, 62, line, C_PURPLE);
    }
    if (c->rider != NONE) {
        snprintf(line, sizeof line, "RIDER: %s", CREATURES[g->cr[c->rider].type].name);
        fb_text(fb, x0, 72, line, C_GREY);
    }
    snprintf(line, sizeof line, "AP%d ST%d CN%d", c->ap, c->stamina, c->con);
    fb_text(fb, x0, 81, line, C_DGREY);
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
        fb_text(fb, x0, 30 + p * 9, line, g->pl[p].alive ? (uint8_t)owner_colour(p) : C_DGREY);
    }
    bool seen = viewer < 0 || g->pl[viewer].seen[u->cy][u->cx];
    fb_text(fb, x0, 72, seen ? TERRAIN[map_get(&g->map, u->cx, u->cy)].name : "UNKNOWN", C_GREY);
    /* objects on the square (the six blue boxes of the classic panel) */
    int n = 0;
    bool vis = viewer < 0 || g->pl[viewer].vis[u->cy][u->cx];
    for (int i = 0; i < 6; i++) {
        int bx = x0 + (i % 3) * 38, by = 84 + (i / 3) * 24;
        fb_frame(fb, bx, by, 20, 20, C_BLUE);
    }
    for (int k = 0; k < g->nobj && vis && n < 6; k++) {
        const Object *o = &g->obj[k];
        if (!o->used || o->x != u->cx || o->y != u->cy) continue;
        fb_sprite(fb, art_object(o->type), SPR, x0 + (n % 3) * 38 + 2, 86 + (n / 3) * 24, 1, C_WHITE, 0, NULL);
        n++;
    }
    if (g->portal_open) fb_text(fb, x0, 140, "THE PORTAL IS OPEN", C_PURPLE);
    else {
        snprintf(line, sizeof line, "PORTAL IN %d TURNS", g->portal_turn - g->turn);
        fb_text(fb, x0, 140, line, C_DGREY);
    }
    fb_text(fb, x0, 160, "TAP HERE FOR MENU", C_DGREY);
}

static void draw_menu(Framebuffer *fb, const Ui *u)
{
    const Menu *m = &u->menu;
    fb_rect(fb, PANEL_X, 3, SCREEN_W - PANEL_X - 3, 183, C_BLACK);
    fb_frame(fb, PANEL_X, 3, SCREEN_W - PANEL_X - 3, 183, C_PURPLE);
    fb_text(fb, PANEL_X + 4, 6, m->title, C_WHITE);
    for (int i = 0; i < m->n; i++) {
        Rect r = ui_menu_item_rect(u, i);
        bool hi = i == m->sel;
        if (hi) fb_rect(fb, r.x, r.y, r.w, r.h, C_LBLUE);
        uint8_t col = m->disabled[i] ? C_RED : (u->mode == M_SPELLS ? C_YELLOW : C_WHITE);
        if (m->act[i] == A_CANCEL) col = C_GREY;
        if (hi) col = m->disabled[i] ? C_RED : C_BLACK;
        fb_text(fb, r.x + 2, r.y + 4, m->label[i], col);
    }
}

/* -------------------------------------------------------------- map view */

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
            if (t == T_PORTAL && (u->tick / 8) % 2) {
                fb_sprite(fb, art_terrain(t), SPR, sx, sy, 1, C_WHITE, 0, &clip);
                fb_rect(fb, sx + 7, sy + 7, 2, 2, C_WHITE);
            }
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
        }
    }
    /* selection marker */
    if (u->sel != NONE && (u->mode == M_SELECTED || u->mode == M_TARGET || u->mode == M_MENU || u->mode == M_SPELLS)) {
        int sx, sy;
        const Creature *c = &g->cr[u->sel];
        if (c->alive && ui_map_to_screen(u, c->x, c->y, &sx, &sy)) {
            fb_frame(fb, sx - 8, sy - 8, 16, 16, C_PURPLE);
        }
    }
    /* cursor */
    int sx, sy;
    if (ui_map_to_screen(u, u->cx, u->cy, &sx, &sy) && (u->tick / 10) % 3 != 0) {
        uint8_t col = C_WHITE;
        if (u->mode == M_SELECTED) col = (u->sel != NONE && g->cr[u->sel].air) ? C_LBLUE : C_LGREEN;
        if (u->mode == M_TARGET) col = C_YELLOW;
        fb_frame(fb, sx - 8, sy - 8, 16, 16, col);
        fb_frame(fb, sx - 7, sy - 7, 14, 14, col);
        if (u->mode == M_SELECTED) fb_rect(fb, sx - 2, sy - 2, 4, 4, col);
    }
}

static void describe_cursor(const Ui *u, int viewer, char *out, size_t n)
{
    const Game *g = &u->game;
    for (int layer = 1; layer >= 0; layer--) {
        int ci = game_creature_at(g, u->cx, u->cy, layer == 1);
        if (ci != NONE && (viewer < 0 || game_can_see_creature(g, viewer, ci))) {
            snprintf(out, n, "%s%s", CREATURES[g->cr[ci].type].name, layer ? " (FLYING)" : "");
            return;
        }
    }
    bool vis = viewer < 0 || g->pl[viewer].vis[u->cy][u->cx];
    bool seen = viewer < 0 || g->pl[viewer].seen[u->cy][u->cx];
    int oi = vis ? game_object_at(g, u->cx, u->cy) : NONE;
    if (oi != NONE) snprintf(out, n, "%s", OBJECTS[g->obj[oi].type].name);
    else snprintf(out, n, "%s", seen ? TERRAIN[map_get(&g->map, u->cx, u->cy)].name : "");
}

static int panel_creature(const Ui *u, int viewer)
{
    const Game *g = &u->game;
    if (u->mode == M_VIEW3D) return u->pov;
    if (u->mode == M_SELECTED && u->sel != NONE && g->cr[u->sel].alive &&
        g->cr[u->sel].x == u->cx && g->cr[u->sel].y == u->cy) {
        return u->sel;
    }
    for (int layer = 1; layer >= 0; layer--) {
        int ci = game_creature_at(g, u->cx, u->cy, layer == 1);
        if (ci != NONE && (viewer < 0 || game_can_see_creature(g, viewer, ci))) return ci;
    }
    if ((u->mode == M_SELECTED || u->mode == M_TARGET) && u->sel != NONE && g->cr[u->sel].alive) return u->sel;
    return NONE;
}

/* ---------------------------------------------------------------- screens */

static void draw_title(Framebuffer *fb, const Ui *u)
{
    fb_clear(fb, C_BLACK);
    /* starfield */
    for (int i = 0; i < 90; i++) {
        int x = (i * 97 + 13) % SCREEN_W, y = (i * 53 + 7) % 120;
        fb_pixel(fb, x, y, (i + (int)(u->tick / 20)) % 7 == 0 ? C_WHITE : C_GREY);
    }
    /* logo with drop shadow */
    fb_text_scaled(fb, 24 + 3, 18 + 3, "MAGES", C_BLUE, 6);
    fb_text_scaled(fb, 24, 18, "MAGES", C_PURPLE, 6);
    fb_text(fb, 24, 70, "A TURN-BASED WIZARD BATTLE", C_LGREY);
    /* a hill with the four wizards */
    fb_rect(fb, 0, 120, SCREEN_W, 80, C_GREEN);
    for (int x = 0; x < SCREEN_W; x += 4) fb_rect(fb, x, 118 + ((x / 4) % 3), 4, 3, C_GREEN);
    for (int p = 0; p < MAX_PLAYERS; p++) {
        fb_sprite(fb, art_creature(CR_WIZARD), SPR, 14 + p * 40, 92 + (p % 2) * 6, 2, OWNER_COLOURS[p], 0, NULL);
    }
    fb_sprite(fb, art_creature(CR_DRAGON), SPR, 236, 40 + (int)((u->tick / 30) % 4), 4, C_WHITE, 0, NULL);
    /* settings menu in a box at the bottom */
    fb_rect(fb, 186, 128, 126, 50, C_BLACK);
    fb_frame(fb, 186, 128, 126, 50, C_LBLUE);
    fb_frame(fb, 187, 129, 124, 48, C_BLUE);
    for (int i = 0; i < u->menu.n; i++) {
        Rect r = ui_menu_item_rect(u, i);
        if (i == u->menu.sel) fb_rect(fb, r.x, r.y, r.w, r.h, C_LBLUE);
        fb_text(fb, r.x + 4, r.y + 4, u->menu.label[i], i == u->menu.sel ? C_BLACK : C_WHITE);
    }
    fb_text(fb, 8, 182, "ORIGINAL GAME - FIRE OR TAP TO CHOOSE", C_YELLOW);
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
    if ((u->tick / 20) % 2) fb_text_centre(fb, SCREEN_W / 2, 154, "PRESS FIRE OR TAP", C_YELLOW);
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
    if ((u->tick / 20) % 2) fb_text_centre(fb, SCREEN_W / 2, 166, "PRESS FIRE", C_LGREY);
}

static void draw_view3d(Framebuffer *fb, const Ui *u, int viewer)
{
    const Game *g = &u->game;
    Rect r = {VIEW_X, VIEW_Y, VIEW_PX, VIEW_PX};
    raycast_render(fb, g, viewer, u->pov, u->cam_x, u->cam_y, u->cam_ang, r, u->tick);
    /* touch hints */
    fb_rect(fb, VIEW_X, VIEW_Y + VIEW_PX - 12, VIEW_PX, 12, C_BLACK);
    fb_text(fb, VIEW_X + 4, VIEW_Y + VIEW_PX - 10, "<", C_GREY);
    fb_text(fb, VIEW_X + VIEW_PX - 10, VIEW_Y + VIEW_PX - 10, ">", C_GREY);
    fb_text_centre(fb, VIEW_X + VIEW_PX / 2, VIEW_Y + VIEW_PX - 10, "EXIT 3D", C_LGREY);
    fb_text_centre(fb, VIEW_X + VIEW_PX / 2, VIEW_Y + 3, FACING_NAMES[u->pov_facing], C_WHITE);
}

void ui_render(const Ui *u, Framebuffer *fb)
{
    if (u->mode == M_TITLE) { draw_title(fb, u); return; }
    if (u->mode == M_HANDOVER) { draw_handover(fb, u); return; }
    if (u->mode == M_GAMEOVER) { draw_gameover(fb, u); return; }
    const Game *g = &u->game;
    int viewer = ui_viewer(u);
    draw_frames(fb, u->fx_timer > 0 ? C_RED : C_BLUE);
    if (u->mode == M_VIEW3D) draw_view3d(fb, u, viewer);
    else draw_map(fb, u, viewer);
    if (u->mode == M_MENU || u->mode == M_SPELLS) {
        draw_menu(fb, u);
    } else {
        int ci = panel_creature(u, viewer);
        if (ci != NONE) draw_creature_panel(fb, g, ci);
        else draw_square_panel(fb, u, viewer);
    }
    char line[64];
    if (u->msg_timer > 0) {
        snprintf(line, sizeof line, "%s", u->msg);
    } else if (u->mode == M_VIEW3D) {
        snprintf(line, sizeof line, "3D VIEW - %s FACING %s", CREATURES[g->cr[u->pov].type].name,
                 FACING_NAMES[u->pov_facing]);
    } else {
        describe_cursor(u, viewer, line, sizeof line);
    }
    fb_text(fb, 6, MSG_Y, line, C_YELLOW);
}
