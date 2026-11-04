#include "save.h"
#include <string.h>

/* ------------------------------------------------------------- writing */

typedef struct {
    uint8_t *p;
    size_t n, cap;
} Out;

static void put(Out *o, uint64_t v, int bytes)
{
    for (int i = 0; i < bytes; i++) {
        if (o->n < o->cap) o->p[o->n] = (uint8_t)(v >> (8 * i));
        o->n++;
    }
}

static void put_bytes(Out *o, const uint8_t *b, size_t len)
{
    for (size_t i = 0; i < len; i++) put(o, b[i], 1);
}

uint32_t save_checksum(const uint8_t *p, size_t n)
{
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < n; i++) {
        h ^= p[i];
        h *= 16777619u;
    }
    return h;
}

#define HEADER 10 /* "MAGS", version u16, body length u32 */

size_t save_write(const Game *g, const SaveMeta *m, uint8_t *buf, size_t cap)
{
    Out o = {buf, 0, cap};
    put_bytes(&o, (const uint8_t *)"MAGS", 4);
    put(&o, SAVE_VERSION, 2);
    put(&o, 0, 4); /* body length, patched below */
    put(&o, (uint64_t)m->humans, 1);
    put(&o, (uint64_t)m->ais, 1);
    put(&o, m->seed, 8);
    put(&o, (uint64_t)g->map.w, 1);
    put(&o, (uint64_t)g->map.h, 1);
    for (int y = 0; y < g->map.h; y++) put_bytes(&o, g->map.t[y], (size_t)g->map.w);
    put(&o, g->rng.s, 8);
    put(&o, (uint64_t)g->npl, 1);
    put(&o, (uint64_t)g->humans, 1);
    put(&o, (uint64_t)g->turn, 2);
    put(&o, (uint64_t)(g->cur + 1), 1);
    put(&o, (uint64_t)g->portal_x, 1);
    put(&o, (uint64_t)g->portal_y, 1);
    put(&o, (uint64_t)g->portal_turn, 2);
    put(&o, g->portal_open, 1);
    put(&o, g->over, 1);
    put(&o, (uint64_t)(g->winner + 1), 1);
    put_bytes(&o, (const uint8_t *)g->msg, sizeof g->msg);
    for (int p = 0; p < g->npl; p++) {
        const Player *pl = &g->pl[p];
        put_bytes(&o, (const uint8_t *)pl->name, sizeof pl->name);
        put(&o, pl->ai, 1);
        put(&o, pl->alive, 1);
        put(&o, (uint64_t)pl->wizard, 2);
        put(&o, (uint64_t)pl->home_x, 1);
        put(&o, (uint64_t)pl->home_y, 1);
        put(&o, (uint64_t)pl->vp, 4);
        for (int y = 0; y < g->map.h; y++) put_bytes(&o, pl->seen[y], (size_t)g->map.w);
        for (int y = 0; y < g->map.h; y++) put_bytes(&o, pl->vis[y], (size_t)g->map.w);
    }
    put(&o, (uint64_t)g->ncr, 2);
    for (int i = 0; i < g->ncr; i++) {
        const Creature *c = &g->cr[i];
        put(&o, c->alive, 1);
        put(&o, c->type, 1);
        put(&o, (uint64_t)(c->owner + 1), 1);
        put(&o, (uint64_t)c->x, 1);
        put(&o, (uint64_t)c->y, 1);
        put(&o, c->air, 1);
        put(&o, c->facing, 1);
        put(&o, (uint64_t)(uint16_t)c->ap, 2);
        put(&o, (uint64_t)(uint16_t)c->stamina, 2);
        put(&o, (uint64_t)(uint16_t)c->con, 2);
        put(&o, (uint64_t)(uint16_t)c->mana, 2);
        put(&o, (uint64_t)(uint8_t)c->invisible, 1);
        put(&o, (uint64_t)(c->rider + 1), 2);
        put(&o, (uint64_t)(c->mount + 1), 2);
        put(&o, c->ninv, 1);
        put_bytes(&o, c->inv, MAX_CARRY);
    }
    put(&o, (uint64_t)g->nobj, 1);
    for (int i = 0; i < g->nobj; i++) {
        const Object *ob = &g->obj[i];
        put(&o, ob->used, 1);
        put(&o, ob->type, 1);
        put(&o, (uint64_t)ob->x, 1);
        put(&o, (uint64_t)ob->y, 1);
    }
    if (o.n + 4 > cap) return 0;
    size_t body = o.n - HEADER;
    for (int i = 0; i < 4; i++) buf[6 + i] = (uint8_t)(body >> (8 * i));
    uint32_t sum = save_checksum(buf + HEADER, body);
    put(&o, sum, 4);
    return o.n;
}

/* ------------------------------------------------------------- reading */

typedef struct {
    const uint8_t *p;
    size_t n, pos;
    bool ok;
} In;

static uint64_t get(In *in, int bytes)
{
    uint64_t v = 0;
    if (in->pos + (size_t)bytes > in->n) {
        in->ok = false;
        return 0;
    }
    for (int i = 0; i < bytes; i++) v |= (uint64_t)in->p[in->pos++] << (8 * i);
    return v;
}

static void get_bytes(In *in, uint8_t *dst, size_t len)
{
    for (size_t i = 0; i < len; i++) dst[i] = (uint8_t)get(in, 1);
}

/* Range check helper: marks the input bad when v is outside [lo, hi]. */
static int64_t chk(In *in, int64_t v, int64_t lo, int64_t hi)
{
    if (v < lo || v > hi) in->ok = false;
    return v;
}

bool save_read(Game *out, SaveMeta *meta, const uint8_t *buf, size_t len)
{
    if (len < HEADER + 4 || memcmp(buf, "MAGS", 4) != 0) return false;
    In in = {buf, len, 4, true};
    if (get(&in, 2) != SAVE_VERSION) return false;
    size_t body = (size_t)get(&in, 4);
    if (body != len - HEADER - 4) return false;
    uint32_t sum = (uint32_t)buf[len - 4] | (uint32_t)buf[len - 3] << 8 | (uint32_t)buf[len - 2] << 16 |
                   (uint32_t)buf[len - 1] << 24;
    if (save_checksum(buf + HEADER, body) != sum) return false;
    in.n = len - 4;

    static Game g;
    memset(&g, 0, sizeof g);
    SaveMeta m;
    m.humans = (int)chk(&in, (int64_t)get(&in, 1), 0, MAX_PLAYERS);
    m.ais = (int)chk(&in, (int64_t)get(&in, 1), 0, MAX_PLAYERS);
    m.seed = get(&in, 8);
    g.map.w = (int)chk(&in, (int64_t)get(&in, 1), 1, MAP_MAX);
    g.map.h = (int)chk(&in, (int64_t)get(&in, 1), 1, MAP_MAX);
    if (!in.ok) return false;
    for (int y = 0; y < g.map.h; y++)
        for (int x = 0; x < g.map.w; x++) g.map.t[y][x] = (uint8_t)chk(&in, (int64_t)get(&in, 1), 0, T_COUNT - 1);
    g.rng.s = get(&in, 8);
    g.npl = (int)chk(&in, (int64_t)get(&in, 1), 1, MAX_PLAYERS);
    g.humans = (int)chk(&in, (int64_t)get(&in, 1), 0, g.npl);
    g.turn = (int)chk(&in, (int64_t)get(&in, 2), 1, MAX_TURNS + 1);
    g.cur = (int)chk(&in, (int64_t)get(&in, 1), 0, g.npl) - 1;
    g.portal_x = (int)chk(&in, (int64_t)get(&in, 1), 0, g.map.w - 1);
    g.portal_y = (int)chk(&in, (int64_t)get(&in, 1), 0, g.map.h - 1);
    g.portal_turn = (int)get(&in, 2);
    g.portal_open = get(&in, 1) != 0;
    g.over = get(&in, 1) != 0;
    g.winner = (int)chk(&in, (int64_t)get(&in, 1), 0, g.npl) - 1;
    get_bytes(&in, (uint8_t *)g.msg, sizeof g.msg);
    g.msg[sizeof g.msg - 1] = 0;
    for (int p = 0; p < g.npl && in.ok; p++) {
        Player *pl = &g.pl[p];
        get_bytes(&in, (uint8_t *)pl->name, sizeof pl->name);
        pl->name[sizeof pl->name - 1] = 0;
        pl->ai = get(&in, 1) != 0;
        pl->alive = get(&in, 1) != 0;
        pl->wizard = (int)get(&in, 2);
        pl->home_x = (int)chk(&in, (int64_t)get(&in, 1), 0, g.map.w - 1);
        pl->home_y = (int)chk(&in, (int64_t)get(&in, 1), 0, g.map.h - 1);
        pl->vp = (int)(int32_t)get(&in, 4);
        for (int y = 0; y < g.map.h; y++)
            for (int x = 0; x < g.map.w; x++) pl->seen[y][x] = (uint8_t)chk(&in, (int64_t)get(&in, 1), 0, 1);
        for (int y = 0; y < g.map.h; y++)
            for (int x = 0; x < g.map.w; x++) pl->vis[y][x] = (uint8_t)chk(&in, (int64_t)get(&in, 1), 0, 1);
    }
    g.ncr = (int)chk(&in, (int64_t)get(&in, 2), 0, MAX_CREATURES);
    for (int i = 0; i < g.ncr && in.ok; i++) {
        Creature *c = &g.cr[i];
        c->alive = get(&in, 1) != 0;
        c->type = (uint8_t)chk(&in, (int64_t)get(&in, 1), 0, CR_COUNT - 1);
        c->owner = (int8_t)(chk(&in, (int64_t)get(&in, 1), 0, g.npl) - 1);
        c->x = (int16_t)chk(&in, (int64_t)get(&in, 1), 0, g.map.w - 1);
        c->y = (int16_t)chk(&in, (int64_t)get(&in, 1), 0, g.map.h - 1);
        c->air = get(&in, 1) != 0;
        c->facing = (uint8_t)chk(&in, (int64_t)get(&in, 1), 0, DIR_COUNT - 1);
        c->ap = (int16_t)get(&in, 2);
        c->stamina = (int16_t)get(&in, 2);
        c->con = (int16_t)get(&in, 2);
        c->mana = (int16_t)get(&in, 2);
        c->invisible = (int8_t)get(&in, 1);
        c->rider = (int16_t)(chk(&in, (int64_t)get(&in, 2), 0, g.ncr) - 1);
        c->mount = (int16_t)(chk(&in, (int64_t)get(&in, 2), 0, g.ncr) - 1);
        c->ninv = (uint8_t)chk(&in, (int64_t)get(&in, 1), 0, MAX_CARRY);
        for (int k = 0; k < MAX_CARRY; k++) c->inv[k] = (uint8_t)chk(&in, (int64_t)get(&in, 1), 0, OB_COUNT - 1);
    }
    g.nobj = (int)chk(&in, (int64_t)get(&in, 1), 0, MAX_OBJECTS);
    for (int i = 0; i < g.nobj && in.ok; i++) {
        Object *ob = &g.obj[i];
        ob->used = get(&in, 1) != 0;
        ob->type = (uint8_t)chk(&in, (int64_t)get(&in, 1), 0, OB_COUNT - 1);
        ob->x = (int16_t)chk(&in, (int64_t)get(&in, 1), 0, g.map.w - 1);
        ob->y = (int16_t)chk(&in, (int64_t)get(&in, 1), 0, g.map.h - 1);
    }
    for (int p = 0; p < g.npl && in.ok; p++) chk(&in, g.pl[p].wizard, 0, g.ncr - 1);
    if (!in.ok || in.pos != in.n) return false;
    *out = g;
    *meta = m;
    return true;
}
