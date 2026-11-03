#include "map.h"
#include <string.h>

const TerrainInfo TERRAIN[T_COUNT] = {
    [T_GRASS] = {"GRASS", '.', 3, 0, 5},
    [T_PATH] = {"PATH", ':', 2, 0, 7},
    [T_FLOOR] = {"FLOOR", ',', 2, TF_ROOF, 2},
    [T_WALL] = {"WALL", '#', 0, TF_NOGROUND | TF_OPAQUE | TF_ROOF | TF_SOLID3D, 10},
    [T_DOOR_CLOSED] = {"DOOR", '+', 0, TF_NOGROUND | TF_OPAQUE | TF_ROOF | TF_SOLID3D, 8},
    [T_DOOR_OPEN] = {"OPEN DOOR", '/', 2, TF_ROOF, 8},
    [T_TREE] = {"TREES", 'T', 4, TF_OPAQUE, 5},
    [T_WATER] = {"WATER", '~', 0, TF_NOGROUND, 14},
    [T_SWAMP] = {"SWAMP", '%', 5, 0, 9},
    [T_MOUNTAIN] = {"MOUNTAIN", '^', 0, TF_NOGROUND | TF_OPAQUE | TF_SOLID3D, 12},
    [T_BRIDGE] = {"BRIDGE", '=', 2, 0, 8},
    [T_PENTAGRAM] = {"PENTAGRAM", '*', 2, TF_ROOF, 2},
    [T_STONES] = {"STONE CIRCLE", 'o', 2, 0, 12},
    [T_PORTAL] = {"PORTAL", 'O', 2, 0, 4},
};

static int wrapn(int v, int n)
{
    v %= n;
    return v < 0 ? v + n : v;
}

int map_wx(const Map *m, int x) { return wrapn(x, m->w); }
int map_wy(const Map *m, int y) { return wrapn(y, m->h); }

Terrain map_get(const Map *m, int x, int y)
{
    return (Terrain)m->t[map_wy(m, y)][map_wx(m, x)];
}

void map_set(Map *m, int x, int y, Terrain t)
{
    m->t[map_wy(m, y)][map_wx(m, x)] = (uint8_t)t;
}

bool map_has(const Map *m, int x, int y, int flag)
{
    return (TERRAIN[map_get(m, x, y)].flags & flag) != 0;
}

int wrap_delta(int a, int b, int n)
{
    int d = wrapn(b - a, n);
    return d > n / 2 ? d - n : d;
}

int map_dist(const Map *m, int x0, int y0, int x1, int y1)
{
    return imax(iabs(wrap_delta(x0, x1, m->w)), iabs(wrap_delta(y0, y1, m->h)));
}

Terrain terrain_from_glyph(char c)
{
    for (int i = 0; i < T_COUNT; i++) {
        if (TERRAIN[i].glyph == c) return (Terrain)i;
    }
    return T_COUNT;
}

bool map_load(Map *m, const char *const *rows, int h)
{
    if (h <= 0 || h > MAP_MAX) return false;
    int w = (int)strlen(rows[0]);
    if (w <= 0 || w > MAP_MAX) return false;
    memset(m, 0, sizeof(*m));
    m->w = w;
    m->h = h;
    for (int y = 0; y < h; y++) {
        if ((int)strlen(rows[y]) != w) return false;
        for (int x = 0; x < w; x++) {
            Terrain t = terrain_from_glyph(rows[y][x]);
            if (t == T_COUNT) return false;
            m->t[y][x] = (uint8_t)t;
        }
    }
    return true;
}
