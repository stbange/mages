#include "game.h"
#include "ai.h"
#include <stdio.h>
#include <string.h>

static void set_msg(Game *g, const char *m)
{
    snprintf(g->msg, sizeof g->msg, "%s", m);
}

static Result fail(Game *g, Result r, const char *m)
{
    set_msg(g, m);
    game_event(g, GE_ERROR);
    return r;
}

void game_event(Game *g, GameEvent e)
{
    if (g->nev < MAX_EVENTS) g->ev[g->nev++] = (uint8_t)e;
}

int game_pop_event(Game *g)
{
    if (g->nev == 0) return -1;
    int e = g->ev[0];
    memmove(g->ev, g->ev + 1, (size_t)(g->nev - 1));
    g->nev--;
    return e;
}

/* ---------------------------------------------------------------- queries */

bool game_on_map(const Creature *c) { return c->alive && c->mount == NONE; }
bool game_is_wizard(const Creature *c) { return c->type == CR_WIZARD; }

int game_creature_at(const Game *g, int x, int y, bool air)
{
    x = map_wx(&g->map, x);
    y = map_wy(&g->map, y);
    for (int i = 0; i < g->ncr; i++) {
        const Creature *c = &g->cr[i];
        if (game_on_map(c) && c->x == x && c->y == y && c->air == air) return i;
    }
    return NONE;
}

int game_object_at(const Game *g, int x, int y)
{
    x = map_wx(&g->map, x);
    y = map_wy(&g->map, y);
    for (int i = 0; i < g->nobj; i++) {
        if (g->obj[i].used && g->obj[i].x == x && g->obj[i].y == y) return i;
    }
    return NONE;
}

bool game_carries(const Game *g, int ci, int objtype)
{
    const Creature *c = &g->cr[ci];
    for (int i = 0; i < c->ninv; i++) {
        if (c->inv[i] == objtype) return true;
    }
    return false;
}

bool game_wounded(const Game *g, int ci)
{
    const Creature *c = &g->cr[ci];
    return c->con * 2 < CREATURES[c->type].con;
}

int game_max_ap(const Game *g, int ci)
{
    int ap = CREATURES[g->cr[ci].type].ap;
    return game_wounded(g, ci) ? ap * 3 / 4 : ap;
}

int game_combat(const Game *g, int ci)
{
    int v = CREATURES[g->cr[ci].type].combat + (game_carries(g, ci, OB_SWORD) ? 3 : 0);
    return game_wounded(g, ci) ? v * 3 / 4 : v;
}

int game_defence(const Game *g, int ci)
{
    int v = CREATURES[g->cr[ci].type].defence + (game_carries(g, ci, OB_SHIELD) ? 3 : 0);
    return game_wounded(g, ci) ? v * 3 / 4 : v;
}

static bool enemies(const Creature *a, const Creature *b)
{
    return a->owner != b->owner;
}

bool game_engaged(const Game *g, int ci)
{
    const Creature *c = &g->cr[ci];
    for (int i = 0; i < g->ncr; i++) {
        const Creature *o = &g->cr[i];
        if (i == ci || !game_on_map(o) || !enemies(c, o) || o->air != c->air) continue;
        if (o->invisible > 0) continue;
        if (map_dist(&g->map, c->x, c->y, o->x, o->y) == 1) return true;
    }
    return false;
}

bool game_los(const Map *m, int x0, int y0, int x1, int y1)
{
    int dx = wrap_delta(x0, x1, m->w), dy = wrap_delta(y0, y1, m->h);
    int adx = iabs(dx), ady = iabs(dy);
    int sx = dx < 0 ? -1 : 1, sy = dy < 0 ? -1 : 1;
    int x = 0, y = 0, err = adx - ady;
    while (x != dx || y != dy) {
        int e2 = 2 * err;
        if (e2 > -ady) { err -= ady; x += sx; }
        if (e2 < adx) { err += adx; y += sy; }
        if ((x != dx || y != dy) && map_has(m, x0 + x, y0 + y, TF_OPAQUE)) return false;
    }
    return true;
}

void game_update_vis(Game *g, int player)
{
    Player *p = &g->pl[player];
    memset(p->vis, 0, sizeof p->vis);
    for (int i = 0; i < g->ncr; i++) {
        const Creature *c = &g->cr[i];
        if (!game_on_map(c) || c->owner != player) continue;
        for (int dy = -SIGHT_RANGE; dy <= SIGHT_RANGE; dy++) {
            for (int dx = -SIGHT_RANGE; dx <= SIGHT_RANGE; dx++) {
                int x = map_wx(&g->map, c->x + dx), y = map_wy(&g->map, c->y + dy);
                if (p->vis[y][x]) continue;
                if (game_los(&g->map, c->x, c->y, x, y)) {
                    p->vis[y][x] = 1;
                    p->seen[y][x] = 1;
                }
            }
        }
    }
}

bool game_can_see_creature(const Game *g, int player, int ci)
{
    const Creature *c = &g->cr[ci];
    if (!game_on_map(c)) return false;
    if (c->owner == player) return true;
    if (player < 0 || c->invisible > 0) return false;
    return g->pl[player].vis[c->y][c->x] != 0;
}

int game_count_creatures(const Game *g, int owner)
{
    int n = 0;
    for (int i = 0; i < g->ncr; i++) {
        if (g->cr[i].alive && g->cr[i].owner == owner) n++;
    }
    return n;
}

int game_wizard_caster(const Game *g, int ci)
{
    const Creature *c = &g->cr[ci];
    if (game_is_wizard(c)) return ci;
    if (c->rider != NONE && game_is_wizard(&g->cr[c->rider])) return c->rider;
    return NONE;
}

bool game_can_afford(const Game *g, int wi, int spell)
{
    const Creature *w = &g->cr[wi];
    return w->mana >= SPELLS[spell].mana && w->ap >= CAST_AP;
}

bool game_spell_needs_target(int spell)
{
    return SPELLS[spell].kind != SK_SUMMON;
}

int game_terrain_cost(const Game *g, int ci, int x, int y)
{
    const Creature *c = &g->cr[ci];
    const TerrainInfo *t = &TERRAIN[map_get(&g->map, x, y)];
    if (c->air) return (t->flags & TF_ROOF) ? -1 : AIR_MOVE_AP;
    return (t->flags & TF_NOGROUND) ? -1 : t->cost;
}

uint32_t game_hash(const Game *g)
{
    uint32_t h = 2166136261u;
#define MIX(v) do { h ^= (uint32_t)(v); h *= 16777619u; } while (0)
    MIX(g->turn); MIX(g->cur); MIX(g->over); MIX(g->winner); MIX(g->ncr);
    for (int i = 0; i < g->ncr; i++) {
        const Creature *c = &g->cr[i];
        MIX(c->alive); MIX(c->type); MIX(c->owner); MIX(c->x); MIX(c->y);
        MIX(c->air); MIX(c->ap); MIX(c->con); MIX(c->mana); MIX(c->stamina);
    }
    for (int i = 0; i < g->npl; i++) MIX(g->pl[i].vp);
#undef MIX
    return h;
}

/* ------------------------------------------------------------------ setup */

int game_spawn(Game *g, int type, int owner, int x, int y)
{
    if (g->ncr >= MAX_CREATURES) return NONE;
    int i = g->ncr++;
    Creature *c = &g->cr[i];
    memset(c, 0, sizeof *c);
    c->alive = true;
    c->type = (uint8_t)type;
    c->owner = (int8_t)owner;
    c->x = (int16_t)map_wx(&g->map, x);
    c->y = (int16_t)map_wy(&g->map, y);
    c->facing = DIR_S;
    c->ap = CREATURES[type].ap;
    c->stamina = CREATURES[type].stamina;
    c->con = CREATURES[type].con;
    c->mana = type == CR_WIZARD ? START_MANA : 0;
    c->rider = NONE;
    c->mount = NONE;
    return i;
}

static void begin_side(Game *g, int owner)
{
    for (int i = 0; i < g->ncr; i++) {
        Creature *c = &g->cr[i];
        if (!c->alive || c->owner != owner) continue;
        const CreatureInfo *ci = &CREATURES[c->type];
        c->ap = (int16_t)game_max_ap(g, i);
        if (!(ci->flags & CF_TIRELESS) && c->stamina * 4 < ci->stamina) c->ap /= 2;
        c->stamina = (int16_t)imin(ci->stamina, c->stamina + imax(2, ci->stamina / 5));
        if (game_is_wizard(c)) c->mana = (int16_t)imin(99, c->mana + MANA_REGEN);
        if (c->invisible > 0) c->invisible--;
    }
}

bool game_init(Game *g, const Scenario *s, int humans, int ais, uint64_t seed)
{
    static const int seat_start[MAX_PLAYERS] = {0, 1, 2, 3};
    int n = humans + ais;
    if (humans < 0 || ais < 0 || n < 1 || n > MAX_PLAYERS) return false;
    memset(g, 0, sizeof *g);
    if (!map_load(&g->map, s->rows, s->h)) return false;
    rng_seed(&g->rng, seed);
    g->npl = n;
    g->humans = humans;
    g->winner = NONE;
    g->turn = 1;
    g->portal_x = s->portal_x;
    g->portal_y = s->portal_y;
    g->portal_turn = s->portal_turn;
    for (int p = 0; p < n; p++) {
        Player *pl = &g->pl[p];
        pl->ai = p >= humans;
        pl->alive = true;
        snprintf(pl->name, sizeof pl->name, pl->ai ? "MAGUS %d" : "WIZARD %d", p + 1);
        int st = seat_start[p];
        pl->wizard = game_spawn(g, CR_WIZARD, p, s->start_x[st], s->start_y[st]);
        pl->home_x = s->start_x[st];
        pl->home_y = s->start_y[st];
    }
    for (int i = 0; i < s->nneutrals; i++) {
        game_spawn(g, s->neutrals[i].type, NEUTRAL, s->neutrals[i].x, s->neutrals[i].y);
    }
    for (int i = 0; i < s->nobjects && i < MAX_OBJECTS; i++) {
        Object *o = &g->obj[g->nobj++];
        o->used = true;
        o->type = s->objects[i].type;
        o->x = s->objects[i].x;
        o->y = s->objects[i].y;
    }
    for (int p = 0; p < n; p++) game_update_vis(g, p);
    g->cur = 0;
    begin_side(g, 0);
    set_msg(g, "TURN 1");
    if (g->pl[0].ai) game_end_turn_from_ai(g);
    return true;
}

/* ---------------------------------------------------------------- victory */

static void check_winner(Game *g)
{
    int alive = 0, last = NONE, humans_alive = 0;
    for (int p = 0; p < g->npl; p++) {
        if (g->pl[p].alive) {
            alive++;
            last = p;
            if (!g->pl[p].ai) humans_alive++;
        }
    }
    if (g->npl > 1 && alive == 1) {
        g->over = true;
        g->winner = last;
    } else if (alive == 0 || (g->humans > 0 && humans_alive == 0)) {
        g->over = true;
        g->winner = alive == 1 ? last : NONE;
    }
    if (g->over) {
        if (g->winner != NONE) {
            char m[64];
            snprintf(m, sizeof m, "%s IS VICTORIOUS", g->pl[g->winner].name);
            set_msg(g, m);
        } else {
            set_msg(g, "THE WORLD FALLS TO CHAOS");
        }
        game_event(g, GE_WIN);
    }
}

static void check_portal(Game *g, int ci)
{
    if (!g->portal_open) return;
    const Creature *c = &g->cr[ci];
    if (c->air || c->x != g->portal_x || c->y != g->portal_y) return;
    int w = game_wizard_caster(g, ci);
    if (w == NONE) return;
    int p = g->cr[w].owner;
    g->pl[p].vp += 100;
    g->over = true;
    g->winner = p;
    char m[64];
    snprintf(m, sizeof m, "%s ESCAPES!", g->pl[p].name);
    set_msg(g, m);
    game_event(g, GE_WIN);
}

/* ----------------------------------------------------------------- combat */

static void place_object(Game *g, int type, int x, int y)
{
    for (int k = 0; k < MAX_OBJECTS; k++) {
        if (!g->obj[k].used) {
            g->obj[k].used = true;
            g->obj[k].type = (uint8_t)type;
            g->obj[k].x = (int16_t)x;
            g->obj[k].y = (int16_t)y;
            if (k >= g->nobj) g->nobj = k + 1;
            return;
        }
    }
}

static void remove_creature(Game *g, int ci)
{
    Creature *c = &g->cr[ci];
    if (!c->alive) return;
    c->alive = false;
    for (int i = 0; i < c->ninv; i++) place_object(g, c->inv[i], c->x, c->y);
    c->ninv = 0;
    if (c->mount != NONE) {
        g->cr[c->mount].rider = NONE;
        c->mount = NONE;
    }
    if (c->rider != NONE) {
        int ri = c->rider;
        Creature *r = &g->cr[ri];
        c->rider = NONE;
        r->mount = NONE;
        r->x = c->x;
        r->y = c->y;
        r->air = false;
        r->alive = false; /* exclude the rider itself from the occupancy test */
        bool blocked = map_has(&g->map, c->x, c->y, TF_NOGROUND) || game_creature_at(g, c->x, c->y, false) != NONE;
        r->alive = true;
        if (blocked) {
            game_kill(g, ri, NEUTRAL); /* the rider falls to its death */
        }
    }
}

void game_kill(Game *g, int ci, int killer_owner)
{
    Creature *c = &g->cr[ci];
    if (!c->alive) return;
    if (killer_owner >= 0 && killer_owner != c->owner) g->pl[killer_owner].vp += CREATURES[c->type].vp;
    char m[64];
    snprintf(m, sizeof m, "%s DIES", CREATURES[c->type].name);
    set_msg(g, m);
    game_event(g, GE_DEATH);
    remove_creature(g, ci);
    if (game_is_wizard(c) && c->owner >= 0) {
        int p = c->owner;
        g->pl[p].alive = false;
        for (int i = 0; i < g->ncr; i++) {
            if (g->cr[i].alive && g->cr[i].owner == p) remove_creature(g, i);
        }
        snprintf(m, sizeof m, "%s HAS FALLEN", g->pl[p].name);
        set_msg(g, m);
        check_winner(g);
    }
}

void game_damage(Game *g, int ti, int dmg, int attacker_owner)
{
    Creature *t = &g->cr[ti];
    t->con = (int16_t)(t->con - dmg);
    if (t->con <= 0) game_kill(g, ti, attacker_owner);
}

static bool strike(Game *g, int ai, int ti)
{
    Creature *a = &g->cr[ai], *t = &g->cr[ti];
    int atk = game_combat(g, ai), def = game_defence(g, ti);
    int chance = clampi(50 + (atk - def) * 5, 10, 95);
    char m[64];
    if (!rng_chance(&g->rng, chance)) {
        snprintf(m, sizeof m, "%s MISSES", CREATURES[a->type].name);
        set_msg(g, m);
        game_event(g, GE_MISS);
        return false;
    }
    game_event(g, GE_HIT);
    bool undead_t = (CREATURES[t->type].flags & CF_UNDEAD) != 0;
    bool undead_a = (CREATURES[a->type].flags & CF_UNDEAD) != 0;
    if (undead_t && !undead_a && !game_carries(g, ai, OB_SWORD)) {
        snprintf(m, sizeof m, "NO EFFECT ON %s", CREATURES[t->type].name);
        set_msg(g, m);
        return true;
    }
    int dmg = rng_range(&g->rng, 1, imax(2, atk / 2 + 1));
    snprintf(m, sizeof m, "%s HITS %s", CREATURES[a->type].name, CREATURES[t->type].name);
    set_msg(g, m);
    game_damage(g, ti, dmg, a->owner);
    return true;
}

Result game_attack(Game *g, int ci, int ti)
{
    if (g->over) return R_GAME_OVER;
    Creature *c = &g->cr[ci], *t = &g->cr[ti];
    if (!game_on_map(c) || c->owner != g->cur) return fail(g, R_NOT_YOURS, "NOT YOUR CREATURE");
    if (!game_on_map(t) || !enemies(c, t) || t->air != c->air ||
        map_dist(&g->map, c->x, c->y, t->x, t->y) != 1) {
        return fail(g, R_NO_TARGET, "NOTHING TO ATTACK");
    }
    if (c->ap < ATTACK_AP) return fail(g, R_NO_AP, "NOT ENOUGH ACTION POINTS");
    c->ap -= ATTACK_AP;
    if (!(CREATURES[c->type].flags & CF_TIRELESS)) c->stamina = (int16_t)imax(0, c->stamina - 2);
    c->facing = (uint8_t)dir_from_delta(wrap_delta(c->x, t->x, g->map.w), wrap_delta(c->y, t->y, g->map.h));
    if (c->invisible) c->invisible = 0; /* attacking reveals */
    strike(g, ci, ti);
    if (t->alive && c->alive && !g->over) {
        bool tired = !(CREATURES[t->type].flags & CF_TIRELESS) &&
                     t->stamina * 4 < CREATURES[t->type].stamina;
        if (!tired && rng_chance(&g->rng, 50)) strike(g, ti, ci);
    }
    if (g->cur >= 0 && !g->over) game_update_vis(g, g->cur);
    return R_OK;
}

/* --------------------------------------------------------------- movement */

static void sync_rider(Game *g, int ci)
{
    Creature *c = &g->cr[ci];
    if (c->rider != NONE) {
        g->cr[c->rider].x = c->x;
        g->cr[c->rider].y = c->y;
        g->cr[c->rider].air = c->air;
    }
}

static Result check_actor(Game *g, int ci)
{
    if (g->over) return R_GAME_OVER;
    Creature *c = &g->cr[ci];
    if (!c->alive || c->owner != g->cur) return fail(g, R_NOT_YOURS, "NOT YOUR CREATURE");
    if (c->mount != NONE) return fail(g, R_CANNOT, "DISMOUNT FIRST");
    return R_OK;
}

Result game_move(Game *g, int ci, int dir)
{
    Result r = check_actor(g, ci);
    if (r != R_OK) return r;
    Creature *c = &g->cr[ci];
    int nx = map_wx(&g->map, c->x + DIR_DX[dir]), ny = map_wy(&g->map, c->y + DIR_DY[dir]);
    int other = game_creature_at(g, nx, ny, c->air);
    if (other != NONE) {
        if (enemies(c, &g->cr[other])) return game_attack(g, ci, other);
        return fail(g, R_BLOCKED, "SQUARE OCCUPIED");
    }
    if (g->turn == 1) return fail(g, R_FIRST_TURN, "NO MOVEMENT ON THE FIRST TURN");
    if (game_engaged(g, ci)) return fail(g, R_ENGAGED, "ENGAGED IN COMBAT");
    int cost = game_terrain_cost(g, ci, nx, ny);
    if (cost < 0) return fail(g, R_BLOCKED, "CANNOT GO THERE");
    if (c->ap < cost) return fail(g, R_NO_AP, "NOT ENOUGH ACTION POINTS");
    c->ap = (int16_t)(c->ap - cost);
    if (!(CREATURES[c->type].flags & CF_TIRELESS)) c->stamina = (int16_t)imax(0, c->stamina - 1);
    c->x = (int16_t)nx;
    c->y = (int16_t)ny;
    c->facing = (uint8_t)dir;
    sync_rider(g, ci);
    game_event(g, GE_STEP);
    set_msg(g, TERRAIN[map_get(&g->map, nx, ny)].name);
    check_portal(g, ci);
    if (g->cur >= 0) game_update_vis(g, g->cur);
    return R_OK;
}

Result game_fly(Game *g, int ci)
{
    Result r = check_actor(g, ci);
    if (r != R_OK) return r;
    Creature *c = &g->cr[ci];
    if (!(CREATURES[c->type].flags & CF_FLY) || c->air) return fail(g, R_CANNOT, "CANNOT FLY");
    if (map_has(&g->map, c->x, c->y, TF_ROOF)) return fail(g, R_BLOCKED, "THE ROOF IS IN THE WAY");
    if (game_creature_at(g, c->x, c->y, true) != NONE) return fail(g, R_BLOCKED, "AIR SPACE OCCUPIED");
    if (c->ap < 2) return fail(g, R_NO_AP, "NOT ENOUGH ACTION POINTS");
    c->ap -= 2;
    c->air = true;
    sync_rider(g, ci);
    game_event(g, GE_FLY);
    set_msg(g, "TAKES TO THE AIR");
    return R_OK;
}

Result game_land(Game *g, int ci)
{
    Result r = check_actor(g, ci);
    if (r != R_OK) return r;
    Creature *c = &g->cr[ci];
    if (!c->air) return fail(g, R_CANNOT, "NOT FLYING");
    if (map_has(&g->map, c->x, c->y, TF_NOGROUND) || game_creature_at(g, c->x, c->y, false) != NONE) {
        return fail(g, R_BLOCKED, "CANNOT LAND HERE");
    }
    if (c->ap < 2) return fail(g, R_NO_AP, "NOT ENOUGH ACTION POINTS");
    c->ap -= 2;
    c->air = false;
    sync_rider(g, ci);
    game_event(g, GE_FLY);
    set_msg(g, "LANDS");
    check_portal(g, ci);
    return R_OK;
}

/* ----------------------------------------------------------------- doors */

Result game_use_door(Game *g, int ci, int tx, int ty)
{
    Result r = check_actor(g, ci);
    if (r != R_OK) return r;
    Creature *c = &g->cr[ci];
    if (!(CREATURES[c->type].flags & CF_HANDS)) return fail(g, R_CANNOT, "NEEDS HANDS");
    if (c->air || map_dist(&g->map, c->x, c->y, tx, ty) != 1) return fail(g, R_OUT_OF_RANGE, "TOO FAR AWAY");
    Terrain t = map_get(&g->map, tx, ty);
    if (t != T_DOOR_CLOSED && t != T_DOOR_OPEN) return fail(g, R_NO_TARGET, "NO DOOR THERE");
    if (t == T_DOOR_OPEN && (game_creature_at(g, tx, ty, false) != NONE || game_object_at(g, tx, ty) != NONE)) {
        return fail(g, R_BLOCKED, "DOORWAY BLOCKED");
    }
    if (c->ap < 2) return fail(g, R_NO_AP, "NOT ENOUGH ACTION POINTS");
    c->ap -= 2;
    map_set(&g->map, tx, ty, t == T_DOOR_CLOSED ? T_DOOR_OPEN : T_DOOR_CLOSED);
    set_msg(g, t == T_DOOR_CLOSED ? "DOOR OPENS" : "DOOR CLOSES");
    game_event(g, GE_DOOR);
    game_update_vis(g, g->cur);
    return R_OK;
}

/* --------------------------------------------------------------- objects */

static Result check_hands(Game *g, int ci)
{
    Result r = check_actor(g, ci);
    if (r != R_OK) return r;
    if (!(CREATURES[g->cr[ci].type].flags & CF_HANDS)) return fail(g, R_CANNOT, "NEEDS HANDS");
    return R_OK;
}

Result game_pickup(Game *g, int ci)
{
    Result r = check_hands(g, ci);
    if (r != R_OK) return r;
    Creature *c = &g->cr[ci];
    int oi = c->air ? NONE : game_object_at(g, c->x, c->y);
    if (oi == NONE) return fail(g, R_NO_TARGET, "NOTHING HERE");
    if (c->ninv >= MAX_CARRY) return fail(g, R_CANNOT, "CANNOT CARRY MORE");
    if (c->ap < 2) return fail(g, R_NO_AP, "NOT ENOUGH ACTION POINTS");
    c->ap -= 2;
    c->inv[c->ninv++] = g->obj[oi].type;
    g->obj[oi].used = false;
    set_msg(g, OBJECTS[c->inv[c->ninv - 1]].name);
    game_event(g, GE_PICKUP);
    return R_OK;
}

Result game_drop(Game *g, int ci)
{
    Result r = check_hands(g, ci);
    if (r != R_OK) return r;
    Creature *c = &g->cr[ci];
    if (c->ninv == 0) return fail(g, R_NO_TARGET, "CARRYING NOTHING");
    if (c->ap < 1) return fail(g, R_NO_AP, "NOT ENOUGH ACTION POINTS");
    c->ap -= 1;
    place_object(g, c->inv[--c->ninv], c->x, c->y);
    set_msg(g, "DROPPED");
    game_event(g, GE_PICKUP);
    return R_OK;
}

Result game_drink(Game *g, int ci)
{
    Result r = check_hands(g, ci);
    if (r != R_OK) return r;
    Creature *c = &g->cr[ci];
    int slot = -1;
    for (int i = 0; i < c->ninv && slot < 0; i++) {
        if (c->inv[i] == OB_HEAL_POTION || (c->inv[i] == OB_MANA_POTION && game_is_wizard(c))) slot = i;
    }
    if (slot < 0) return fail(g, R_NO_TARGET, "NOTHING TO DRINK");
    if (c->ap < 2) return fail(g, R_NO_AP, "NOT ENOUGH ACTION POINTS");
    c->ap -= 2;
    if (c->inv[slot] == OB_HEAL_POTION) {
        c->con = (int16_t)imin(CREATURES[c->type].con, c->con + 12);
        set_msg(g, "FEELS BETTER");
    } else {
        c->mana = (int16_t)imin(99, c->mana + 20);
        set_msg(g, "MANA FLOWS");
    }
    for (int i = slot; i < c->ninv - 1; i++) c->inv[i] = c->inv[i + 1];
    c->ninv--;
    game_event(g, GE_DRINK);
    return R_OK;
}

/* ---------------------------------------------------------------- riding */

Result game_ride(Game *g, int ci, int mi)
{
    Result r = check_actor(g, ci);
    if (r != R_OK) return r;
    Creature *c = &g->cr[ci], *m = &g->cr[mi];
    if (!(CREATURES[c->type].flags & CF_RIDER) || !(CREATURES[m->type].flags & CF_MOUNT) ||
        !game_on_map(m) || m->owner != c->owner || m->rider != NONE || c->rider != NONE || c->air ||
        m->air || map_dist(&g->map, c->x, c->y, m->x, m->y) != 1) {
        return fail(g, R_CANNOT, "CANNOT RIDE THAT");
    }
    if (c->ap < 2) return fail(g, R_NO_AP, "NOT ENOUGH ACTION POINTS");
    c->ap -= 2;
    c->mount = (int16_t)mi;
    m->rider = (int16_t)ci;
    sync_rider(g, mi);
    set_msg(g, "MOUNTED");
    game_event(g, GE_STEP);
    return R_OK;
}

Result game_dismount(Game *g, int mi)
{
    if (g->over) return R_GAME_OVER;
    Creature *m = &g->cr[mi];
    if (!game_on_map(m) || m->owner != g->cur || m->rider == NONE) return fail(g, R_CANNOT, "NO RIDER");
    if (m->air) return fail(g, R_CANNOT, "LAND FIRST");
    Creature *rd = &g->cr[m->rider];
    if (rd->ap < 2) return fail(g, R_NO_AP, "NOT ENOUGH ACTION POINTS");
    for (int d = 0; d < DIR_COUNT; d++) {
        int x = m->x + DIR_DX[d], y = m->y + DIR_DY[d];
        if (map_has(&g->map, x, y, TF_NOGROUND) || game_creature_at(g, x, y, false) != NONE) continue;
        rd->ap -= 2;
        rd->mount = NONE;
        rd->x = (int16_t)map_wx(&g->map, x);
        rd->y = (int16_t)map_wy(&g->map, y);
        rd->air = false;
        m->rider = NONE;
        set_msg(g, "DISMOUNTED");
        game_event(g, GE_STEP);
        game_update_vis(g, g->cur);
        return R_OK;
    }
    return fail(g, R_NO_ROOM, "NO ROOM TO DISMOUNT");
}

/* ---------------------------------------------------------------- spells */

static int creature_on(const Game *g, int x, int y)
{
    int c = game_creature_at(g, x, y, false);
    return c != NONE ? c : game_creature_at(g, x, y, true);
}

static Result cast_summon(Game *g, int wi, int spell)
{
    Creature *w = &g->cr[wi];
    const SpellInfo *s = &SPELLS[spell];
    int made = 0;
    for (int d = 0; d < DIR_COUNT && made < s->level; d++) {
        int x = w->x + DIR_DX[d], y = w->y + DIR_DY[d];
        if (map_has(&g->map, x, y, TF_NOGROUND) || game_creature_at(g, x, y, false) != NONE) continue;
        int ni = game_spawn(g, s->creature, w->owner, x, y);
        if (ni == NONE) break;
        g->cr[ni].ap = 0;
        g->cr[ni].facing = (uint8_t)d;
        made++;
    }
    if (made == 0) return fail(g, R_NO_ROOM, "NO ROOM TO SUMMON");
    char m[64];
    snprintf(m, sizeof m, "%s SUMMONED", CREATURES[s->creature].name);
    set_msg(g, m);
    game_event(g, GE_SUMMON);
    return R_OK;
}

Result game_cast(Game *g, int wi, int spell, int tx, int ty)
{
    if (g->over) return R_GAME_OVER;
    Creature *w = &g->cr[wi];
    if (!w->alive || w->owner != g->cur || !game_is_wizard(w)) return fail(g, R_NOT_YOURS, "ONLY YOUR WIZARD CASTS");
    const SpellInfo *s = &SPELLS[spell];
    if (w->ap < CAST_AP) return fail(g, R_NO_AP, "NOT ENOUGH ACTION POINTS");
    if (w->mana < s->mana) return fail(g, R_NO_MANA, "NOT ENOUGH MANA");
    Result r = R_OK;
    if (s->kind == SK_SUMMON) {
        if (w->mount != NONE) return fail(g, R_CANNOT, "DISMOUNT FIRST");
        r = cast_summon(g, wi, spell);
    } else {
        tx = map_wx(&g->map, tx);
        ty = map_wy(&g->map, ty);
        if (map_dist(&g->map, w->x, w->y, tx, ty) > SPELL_RANGE) return fail(g, R_OUT_OF_RANGE, "OUT OF RANGE");
        if (!game_los(&g->map, w->x, w->y, tx, ty)) return fail(g, R_NOT_VISIBLE, "CANNOT SEE TARGET");
        int t = creature_on(g, tx, ty);
        bool own = t != NONE && g->cr[t].owner == w->owner;
        switch (s->kind) {
        case SK_BOLT:
        case SK_SPARKS:
            if (t == NONE) return fail(g, R_NO_TARGET, "NO TARGET");
            set_msg(g, s->kind == SK_BOLT ? "LIGHTNING STRIKES" : "SPARKS FLY");
            game_event(g, GE_HIT);
            game_damage(g, t, s->kind == SK_BOLT ? rng_range(&g->rng, 4, 10) : rng_range(&g->rng, 2, 5), w->owner);
            break;
        case SK_FIREBALL:
            set_msg(g, "FIREBALL EXPLODES");
            game_event(g, GE_HIT);
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    for (int layer = 0; layer < 2; layer++) {
                        int v = game_creature_at(g, tx + dx, ty + dy, layer == 1);
                        if (v == NONE) continue;
                        int dmg = (dx || dy) ? rng_range(&g->rng, 1, 4) : rng_range(&g->rng, 3, 8);
                        game_damage(g, v, dmg, w->owner);
                    }
                }
            }
            break;
        case SK_HEAL:
            if (!own) return fail(g, R_NO_TARGET, "CHOOSE YOUR OWN CREATURE");
            g->cr[t].con = (int16_t)imin(CREATURES[g->cr[t].type].con, g->cr[t].con + 10);
            set_msg(g, "WOUNDS CLOSE");
            break;
        case SK_INVIS:
            if (!own) return fail(g, R_NO_TARGET, "CHOOSE YOUR OWN CREATURE");
            g->cr[t].invisible = 3;
            set_msg(g, "FADES FROM SIGHT");
            break;
        case SK_QUICKEN:
            if (!own || t == wi) return fail(g, R_NO_TARGET, "CHOOSE ANOTHER OWN CREATURE");
            g->cr[t].ap = (int16_t)game_max_ap(g, t);
            set_msg(g, "QUICKENED");
            break;
        case SK_TELEPORT:
            if (w->mount != NONE) return fail(g, R_CANNOT, "DISMOUNT FIRST");
            if (t != NONE || map_has(&g->map, tx, ty, TF_NOGROUND)) return fail(g, R_BLOCKED, "CANNOT GO THERE");
            w->x = (int16_t)tx;
            w->y = (int16_t)ty;
            w->air = false;
            set_msg(g, "TELEPORTED");
            break;
        default: /* SK_BANISH */
            if (t == NONE || game_is_wizard(&g->cr[t])) return fail(g, R_NO_TARGET, "NOTHING TO BANISH");
            if (rng_chance(&g->rng, clampi(100 - g->cr[t].con * 3, 10, 90))) {
                game_kill(g, t, w->owner);
                set_msg(g, "BANISHED");
            } else {
                set_msg(g, "BANISH FAILS");
            }
            break;
        }
        game_event(g, GE_CAST);
    }
    if (r != R_OK) return r;
    /* the caster may have died from its own fireball */
    w->mana = (int16_t)(w->mana - s->mana);
    w->ap = (int16_t)(w->ap - CAST_AP);
    if (w->alive && w->invisible && s->kind != SK_INVIS && s->kind != SK_SUMMON) w->invisible = 0;
    if (w->alive) check_portal(g, wi);
    if (g->cur >= 0 && !g->over) game_update_vis(g, g->cur);
    return R_OK;
}

/* ----------------------------------------------------------------- turns */

static void start_turn(Game *g)
{
    g->turn++;
    if (!g->portal_open && g->turn >= g->portal_turn) {
        g->portal_open = true;
        map_set(&g->map, g->portal_x, g->portal_y, T_PORTAL);
        game_event(g, GE_PORTAL);
    }
    if (g->turn > MAX_TURNS) {
        g->over = true;
        g->winner = NONE;
        set_msg(g, "THE WORLD FALLS TO CHAOS");
        return;
    }
    g->cur = NEUTRAL;
    begin_side(g, NEUTRAL);
    ai_neutral_turn(g);
}

static void advance(Game *g)
{
    int next = NONE;
    for (int p = (g->cur < 0 ? 0 : g->cur + 1); p < g->npl; p++) {
        if (g->pl[p].alive) { next = p; break; }
    }
    if (next == NONE) {
        start_turn(g);
        if (g->over) return;
        for (int p = 0; p < g->npl; p++) {
            if (g->pl[p].alive) { next = p; break; }
        }
    }
    g->cur = next;
    begin_side(g, next);
    game_update_vis(g, next);
    char m[64];
    snprintf(m, sizeof m, "TURN %d: %s", g->turn, g->pl[next].name);
    if (g->portal_open && g->turn == g->portal_turn) snprintf(m, sizeof m, "THE PORTAL HAS OPENED!");
    set_msg(g, m);
    game_event(g, GE_TURN);
}

void game_end_turn_from_ai(Game *g)
{
    while (!g->over && g->pl[g->cur].ai) {
        ai_take_turn(g, g->cur);
        if (g->over) return;
        advance(g);
    }
}

void game_end_turn(Game *g)
{
    if (g->over) return;
    advance(g);
    if (!g->over) game_end_turn_from_ai(g);
}
