/* World map: a wrap-around grid of terrain squares. */
#ifndef MAGES_MAP_H
#define MAGES_MAP_H
#include "defs.h"

#define MAP_MAX 64

typedef enum {
    T_GRASS, T_PATH, T_FLOOR, T_WALL, T_DOOR_CLOSED, T_DOOR_OPEN, T_TREE,
    T_WATER, T_SWAMP, T_MOUNTAIN, T_BRIDGE, T_PENTAGRAM, T_STONES, T_PORTAL,
    T_COUNT
} Terrain;

enum {
    TF_NOGROUND = 1, /* ground creatures cannot enter */
    TF_OPAQUE = 2,   /* blocks line of sight */
    TF_ROOF = 4,     /* indoors: flyers cannot enter or take off */
    TF_SOLID3D = 8,  /* drawn as a solid block in the 3D view */
};

typedef struct {
    const char *name;
    char glyph;
    uint8_t cost;        /* AP cost for ground creatures */
    uint8_t flags;
    uint8_t floor_color; /* main colour (floor in 3D, mini map) */
} TerrainInfo;

extern const TerrainInfo TERRAIN[T_COUNT];

typedef struct {
    int w, h;
    uint8_t t[MAP_MAX][MAP_MAX];
} Map;

int map_wx(const Map *m, int x);
int map_wy(const Map *m, int y);
Terrain map_get(const Map *m, int x, int y);
void map_set(Map *m, int x, int y, Terrain t);
bool map_has(const Map *m, int x, int y, int flag);
/* Signed shortest delta from a to b on a wrapping axis of length n. */
int wrap_delta(int a, int b, int n);
/* Chebyshev distance with wrap-around. */
int map_dist(const Map *m, int x0, int y0, int x1, int y1);
/* Load from rows of glyphs. Returns false on bad size or unknown glyph. */
bool map_load(Map *m, const char *const *rows, int h);
Terrain terrain_from_glyph(char c);

#endif
