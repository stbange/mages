#include "raycast.h"
#include "art.h"
#include "palette.h"
#include <math.h>

#define PI_F 3.14159265f
#define MAX_DEPTH 24
#define FOV_PLANE 0.66f
#define MAX_SPRITES 256

typedef struct {
    float x, y, dist;
    const char *const *art;
    int owner_colour;
    bool air;
    float scale;
} Billboard;

static const uint8_t OWNER_COLOURS[MAX_PLAYERS] = {C_YELLOW, C_CYAN, C_LGREEN, C_LRED};

/* Relative billboard size per creature type (percent). */
static const uint8_t CREATURE_SCALE[CR_COUNT] = {
    [CR_WIZARD] = 90, [CR_GOBLIN] = 75, [CR_DWARF] = 75, [CR_WOLF] = 80, [CR_SKELETON] = 90,
    [CR_BAT] = 70, [CR_HARPY] = 85, [CR_UNICORN] = 100, [CR_TROLL] = 115, [CR_GHOST] = 90,
    [CR_PEGASUS] = 100, [CR_DRAGON] = 140, [CR_OGRE] = 115, [CR_SPIDER] = 85, [CR_ZOMBIE] = 90,
};

float raycast_facing_angle(int facing) { return (float)facing * PI_F / 4.0f; }

static int fog(float dist)
{
    return dist > 10.0f ? 2 : (dist > 6.0f ? 1 : 0);
}

static int floor_pixel(const Map *m, float fx, float fy)
{
    int tx = (int)floorf(fx), ty = (int)floorf(fy);
    Terrain t = map_get(m, tx, ty);
    if (TERRAIN[t].flags & TF_SOLID3D) t = T_FLOOR;
    if (t == T_TREE) t = T_GRASS; /* trees are drawn as billboards */
    int u = (int)((fx - (float)tx) * SPR) & 15, v = (int)((fy - (float)ty) * SPR) & 15;
    int c = art_pixel(art_terrain(t)[v][u], C_WHITE);
    return c < 0 ? C_BLACK : c;
}

static void draw_sky_and_floor(Framebuffer *fb, const Game *g, float cx, float cy, float dx, float dy,
                               float plx, float ply, Rect r)
{
    int h = r.h, half = h / 2;
    for (int y = 0; y < h; y++) {
        int py = r.y + y;
        bool below = y > half;
        int p = below ? y - half : half - y;
        if (p == 0) p = 1;
        float row_dist = (0.5f * (float)h) / (float)p;
        float lx = cx + row_dist * (dx - plx), ly = cy + row_dist * (dy - ply);
        float sx = row_dist * 2.0f * plx / (float)r.w, sy = row_dist * 2.0f * ply / (float)r.w;
        for (int x = 0; x < r.w; x++) {
            float fx = lx + sx * (float)x, fy = ly + sy * (float)x;
            uint8_t c;
            if (below) {
                c = (uint8_t)floor_pixel(&g->map, fx, fy);
            } else if (map_has(&g->map, (int)floorf(fx), (int)floorf(fy), TF_ROOF)) {
                /* wooden ceiling with beams */
                float bx = fx - floorf(fx);
                c = (bx < 0.12f) ? C_BROWN : C_DGREY;
            } else {
                c = y < h / 6 ? C_BLUE : (y < h / 3 ? C_LBLUE : C_CYAN);
                if (y < h / 3 && (((unsigned)x * 73856093u) ^ ((unsigned)y * 19349663u)) % 89u == 0) c = C_WHITE;
                fb->px[py][r.x + x] = c;
                continue;
            }
            fb->px[py][r.x + x] = palette_darken(c, fog(row_dist));
        }
    }
}

void raycast_render(Framebuffer *fb, const Game *g, int viewer, int pov, float cx, float cy,
                    float ang, Rect r, uint32_t tick)
{
    float zbuf[SCREEN_W];
    float dx = sinf(ang), dy = -cosf(ang);
    float plx = -dy * FOV_PLANE, ply = dx * FOV_PLANE;
    const Map *m = &g->map;
    (void)tick;

    draw_sky_and_floor(fb, g, cx, cy, dx, dy, plx, ply, r);

    /* walls */
    for (int x = 0; x < r.w; x++) {
        float camx = 2.0f * (float)x / (float)r.w - 1.0f;
        float rx = dx + plx * camx, ry = dy + ply * camx;
        int mx = (int)floorf(cx), my = (int)floorf(cy);
        float ddx = rx == 0 ? 1e30f : fabsf(1.0f / rx), ddy = ry == 0 ? 1e30f : fabsf(1.0f / ry);
        int stx = rx < 0 ? -1 : 1, sty = ry < 0 ? -1 : 1;
        float sdx = rx < 0 ? (cx - (float)mx) * ddx : ((float)mx + 1.0f - cx) * ddx;
        float sdy = ry < 0 ? (cy - (float)my) * ddy : ((float)my + 1.0f - cy) * ddy;
        int side = 0, hit = 0;
        for (int i = 0; i < MAX_DEPTH * 2 && !hit; i++) {
            if (sdx < sdy) { sdx += ddx; mx += stx; side = 0; }
            else { sdy += ddy; my += sty; side = 1; }
            if (map_has(m, mx, my, TF_SOLID3D)) hit = 1;
        }
        zbuf[x] = 1e30f;
        if (!hit) continue;
        float dist = side == 0 ? sdx - ddx : sdy - ddy;
        if (dist < 0.05f) dist = 0.05f;
        zbuf[x] = dist;
        int lh = (int)((float)r.h / dist);
        int y0 = r.h / 2 - lh / 2;
        float wx = side == 0 ? cy + dist * ry : cx + dist * rx;
        wx -= floorf(wx);
        int u = (int)(wx * SPR) & 15;
        const char *const *tex = art_terrain(map_get(m, mx, my));
        int shade = fog(dist) + side;
        int ys = y0 < 0 ? 0 : y0, ye = y0 + lh > r.h ? r.h : y0 + lh;
        for (int y = ys; y < ye; y++) {
            int v = (int)((float)(y - y0) * SPR / (float)lh) & 15;
            int c = art_pixel(tex[v][u], C_WHITE);
            if (c < 0) c = C_BLACK;
            fb->px[r.y + y][r.x + x] = palette_darken((uint8_t)c, shade);
        }
    }

    /* billboards: trees, objects, creatures */
    static Billboard bb[MAX_SPRITES];
    int n = 0;
    int icx = (int)floorf(cx), icy = (int)floorf(cy);
    for (int oy = -MAX_DEPTH / 2; oy <= MAX_DEPTH / 2; oy++) {
        for (int ox = -MAX_DEPTH / 2; ox <= MAX_DEPTH / 2; ox++) {
            int tx = icx + ox, ty = icy + oy;
            if (n < MAX_SPRITES && map_get(m, tx, ty) == T_TREE) {
                bb[n++] = (Billboard){(float)tx + 0.5f, (float)ty + 0.5f, 0, art_terrain(T_TREE), C_WHITE, false, 1.0f};
            }
            int wx = map_wx(m, tx), wy = map_wy(m, ty);
            bool vis = viewer < 0 || g->pl[viewer].vis[wy][wx];
            if (!vis) continue;
            for (int k = 0; k < g->nobj && n < MAX_SPRITES; k++) {
                const Object *o = &g->obj[k];
                if (o->used && o->x == wx && o->y == wy) {
                    bb[n++] = (Billboard){(float)tx + 0.5f, (float)ty + 0.5f, 0, art_object(o->type), C_WHITE, false, 0.6f};
                }
            }
            for (int k = 0; k < g->ncr && n < MAX_SPRITES; k++) {
                const Creature *c = &g->cr[k];
                if (k == pov || !game_on_map(c) || c->x != wx || c->y != wy) continue;
                if (viewer >= 0 && !game_can_see_creature(g, viewer, k)) continue;
                if (pov >= 0 && g->cr[pov].mount == k) continue;
                int oc = c->owner >= 0 ? OWNER_COLOURS[c->owner] : C_LGREY;
                bb[n++] = (Billboard){(float)tx + 0.5f, (float)ty + 0.5f, 0, art_creature(c->type), oc, c->air,
                                      (float)CREATURE_SCALE[c->type] / 100.0f};
            }
        }
    }
    for (int i = 0; i < n; i++) {
        float sx = bb[i].x - cx, sy = bb[i].y - cy;
        bb[i].dist = sx * sx + sy * sy;
    }
    /* insertion sort, far to near */
    for (int i = 1; i < n; i++) {
        Billboard t = bb[i];
        int j = i - 1;
        while (j >= 0 && bb[j].dist < t.dist) { bb[j + 1] = bb[j]; j--; }
        bb[j + 1] = t;
    }
    float inv = 1.0f / (plx * dy - dx * ply);
    for (int i = 0; i < n; i++) {
        float sx = bb[i].x - cx, sy = bb[i].y - cy;
        float tx = inv * (dy * sx - dx * sy);
        float ty = inv * (-ply * sx + plx * sy);
        if (ty <= 0.2f) continue;
        int scx = (int)((float)r.w / 2.0f * (1.0f + tx / ty));
        int size = (int)((float)r.h / ty * bb[i].scale);
        if (size < 2) continue;
        int base = r.h / 2 + (int)((float)r.h / ty / 2.0f); /* floor line */
        int top = base - size - (bb[i].air ? (int)((float)r.h / ty * 0.6f) : 0);
        int left = scx - size / 2;
        int shade = fog(ty);
        for (int x = imax(0, left); x < imin(r.w, left + size); x++) {
            if (zbuf[x] <= ty) continue;
            int u = (x - left) * SPR / size;
            for (int y = imax(0, top); y < imin(r.h, top + size); y++) {
                int v = (y - top) * SPR / size;
                int c = art_pixel(bb[i].art[v][u], bb[i].owner_colour);
                if (c < 0) continue;
                fb->px[r.y + y][r.x + x] = palette_darken((uint8_t)c, shade);
            }
        }
    }
}
