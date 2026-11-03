#include "ai.h"

/* Nearest enemy creature that `viewer` can see (player vis or, for
   neutrals, direct line of sight within `range`). */
static int nearest_enemy(const Game *g, int ci, int range)
{
    const Creature *c = &g->cr[ci];
    int best = NONE, bestd = range + 1;
    for (int i = 0; i < g->ncr; i++) {
        const Creature *o = &g->cr[i];
        if (!game_on_map(o) || o->owner == c->owner || o->invisible > 0) continue;
        int d = map_dist(&g->map, c->x, c->y, o->x, o->y);
        if (d >= bestd) continue;
        bool seen = c->owner >= 0 ? g->pl[c->owner].vis[o->y][o->x] != 0
                                  : game_los(&g->map, c->x, c->y, o->x, o->y);
        if (!seen) continue;
        best = i;
        bestd = d;
    }
    return best;
}

static bool passable_for(const Game *g, int ci, int x, int y, bool allow_doors)
{
    const Creature *c = &g->cr[ci];
    int occ = game_creature_at(g, x, y, c->air);
    if (occ != NONE && occ != ci && g->cr[occ].owner == c->owner) return false;
    if (game_terrain_cost(g, ci, x, y) >= 0) return true;
    return allow_doors && !c->air && map_get(&g->map, x, y) == T_DOOR_CLOSED;
}

bool ai_step_toward(Game *g, int ci, int tx, int ty)
{
    static int16_t dist[MAP_MAX][MAP_MAX];
    static int16_t qx[MAP_MAX * MAP_MAX], qy[MAP_MAX * MAP_MAX];
    Creature *c = &g->cr[ci];
    const Map *m = &g->map;
    bool hands = (CREATURES[c->type].flags & CF_HANDS) != 0;
    tx = map_wx(m, tx);
    ty = map_wy(m, ty);
    for (int y = 0; y < m->h; y++)
        for (int x = 0; x < m->w; x++) dist[y][x] = -1;
    int head = 0, tail = 0;
    dist[ty][tx] = 0;
    qx[tail] = (int16_t)tx;
    qy[tail++] = (int16_t)ty;
    while (head < tail && dist[c->y][c->x] < 0) {
        int x = qx[head], y = qy[head++];
        for (int d = 0; d < DIR_COUNT; d++) {
            int nx = map_wx(m, x + DIR_DX[d]), ny = map_wy(m, y + DIR_DY[d]);
            if (dist[ny][nx] >= 0) continue;
            if (!(nx == c->x && ny == c->y) && !passable_for(g, ci, nx, ny, hands)) continue;
            dist[ny][nx] = (int16_t)(dist[y][x] + 1);
            qx[tail] = (int16_t)nx;
            qy[tail++] = (int16_t)ny;
        }
    }
    if (dist[c->y][c->x] < 0) return false;
    int bestdir = -1, bestd = dist[c->y][c->x];
    for (int d = 0; d < DIR_COUNT; d++) {
        int nx = map_wx(m, c->x + DIR_DX[d]), ny = map_wy(m, c->y + DIR_DY[d]);
        if (dist[ny][nx] >= 0 && dist[ny][nx] < bestd) {
            bestd = dist[ny][nx];
            bestdir = d;
        }
    }
    int nx = c->x + DIR_DX[bestdir], ny = c->y + DIR_DY[bestdir];
    int16_t ap = c->ap;
    Result r = map_get(m, nx, ny) == T_DOOR_CLOSED ? game_use_door(g, ci, nx, ny)
                                                    : game_move(g, ci, bestdir);
    return r == R_OK && c->ap != ap;
}

static void act_creature(Game *g, int ci, int range, int goal_x, int goal_y, bool has_goal)
{
    for (int guard = 0; guard < 24; guard++) {
        Creature *c = &g->cr[ci];
        if (!c->alive || g->over || c->ap <= 0) return;
        if (c->air) {
            if (game_land(g, ci) != R_OK) return;
            continue;
        }
        int e = nearest_enemy(g, ci, range);
        int tx, ty;
        if (e != NONE) {
            tx = g->cr[e].x;
            ty = g->cr[e].y;
        } else if (has_goal) {
            tx = goal_x;
            ty = goal_y;
        } else {
            return;
        }
        if (c->x == tx && c->y == ty) return;
        if (!ai_step_toward(g, ci, tx, ty)) return;
    }
}

void ai_neutral_turn(Game *g)
{
    for (int i = 0; i < g->ncr && !g->over; i++) {
        Creature *c = &g->cr[i];
        if (!game_on_map(c) || c->owner != NEUTRAL) continue;
        act_creature(g, i, 6, 0, 0, false);
    }
}

static const uint8_t SUMMON_PREF[] = {
    SP_DRAGON, SP_TROLL, SP_OGRE, SP_GHOST, SP_SKELETONS, SP_UNICORN,
    SP_WOLVES, SP_GOBLINS, SP_DWARF,
};

static void summon_phase(Game *g, int wi)
{
    for (int tries = 0; tries < 4; tries++) {
        Creature *w = &g->cr[wi];
        if (game_count_creatures(g, w->owner) >= 9 || w->mount != NONE) return;
        int chosen = -1;
        for (unsigned k = 0; k < sizeof SUMMON_PREF; k++) {
            int sp = SUMMON_PREF[k];
            /* keep a reserve for a lightning bolt after turn one */
            int reserve = g->turn > 1 ? SPELLS[SP_BOLT].mana : 0;
            if (game_can_afford(g, wi, sp) && w->mana - SPELLS[sp].mana >= reserve &&
                (sp != SP_DRAGON || rng_chance(&g->rng, 50))) {
                chosen = sp;
                break;
            }
        }
        if (chosen < 0 || game_cast(g, wi, chosen, 0, 0) != R_OK) return;
    }
}

static void attack_spell_phase(Game *g, int wi)
{
    Creature *w = &g->cr[wi];
    int e = nearest_enemy(g, wi, SPELL_RANGE);
    if (e == NONE || !game_can_afford(g, wi, SP_BOLT)) return;
    game_cast(g, wi, SP_BOLT, g->cr[e].x, g->cr[e].y);
    (void)w;
}

static void creature_goal(const Game *g, int player, int *gx, int *gy)
{
    *gx = g->portal_x;
    *gy = g->portal_y;
    if (g->turn < 5 || g->portal_open) return;
    const Creature *w = &g->cr[g->pl[player].wizard];
    int bestd = 1 << 20;
    for (int p = 0; p < g->npl; p++) {
        if (p == player || !g->pl[p].alive) continue;
        int d = map_dist(&g->map, w->x, w->y, g->pl[p].home_x, g->pl[p].home_y);
        if (d < bestd) {
            bestd = d;
            *gx = g->pl[p].home_x;
            *gy = g->pl[p].home_y;
        }
    }
}

void ai_take_turn(Game *g, int player)
{
    int wi = g->pl[player].wizard;
    game_update_vis(g, player);
    summon_phase(g, wi);
    if (g->over) return;
    attack_spell_phase(g, wi);
    int gx, gy;
    creature_goal(g, player, &gx, &gy);
    int n = g->ncr;
    /* creatures with hands go first so they can open doors for the others */
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < n && !g->over; i++) {
            Creature *c = &g->cr[i];
            if (!game_on_map(c) || c->owner != player) continue;
            bool hands = (CREATURES[c->type].flags & CF_HANDS) != 0;
            if (hands != (pass == 0)) continue;
            if (game_wizard_caster(g, i) != NONE) {
                /* wizards (and their mounts) wait at home until the portal opens */
                if (g->portal_open) act_creature(g, i, 0, g->portal_x, g->portal_y, true);
                continue;
            }
            act_creature(g, i, 8, gx, gy, true);
        }
    }
}
