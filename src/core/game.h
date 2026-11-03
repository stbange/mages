/* Game state and rules. All player and AI actions go through this API. */
#ifndef MAGES_GAME_H
#define MAGES_GAME_H
#include "data.h"
#include "map.h"
#include "rng.h"
#include "scenario.h"

typedef struct {
    bool alive;
    uint8_t type;
    int8_t owner;   /* player index or NEUTRAL */
    int16_t x, y;
    bool air;
    uint8_t facing;
    int16_t ap, stamina, con, mana;
    int8_t invisible; /* own turns remaining */
    int16_t rider;    /* creature riding this one, or NONE */
    int16_t mount;    /* creature this one rides (then off-map), or NONE */
    uint8_t inv[MAX_CARRY];
    uint8_t ninv;
} Creature;

typedef struct {
    bool used;
    uint8_t type;
    int16_t x, y;
} Object;

typedef struct {
    char name[12];
    bool ai;
    bool alive;
    int wizard;
    int home_x, home_y;
    int vp;
    uint8_t vis[MAP_MAX][MAP_MAX];
    uint8_t seen[MAP_MAX][MAP_MAX];
} Player;

typedef enum {
    R_OK, R_NO_AP, R_BLOCKED, R_ENGAGED, R_NOT_YOURS, R_FIRST_TURN, R_NO_MANA,
    R_NO_TARGET, R_OUT_OF_RANGE, R_NOT_VISIBLE, R_CANNOT, R_NO_ROOM, R_GAME_OVER
} Result;

typedef enum {
    GE_STEP, GE_HIT, GE_MISS, GE_DEATH, GE_CAST, GE_SUMMON, GE_DOOR, GE_PORTAL,
    GE_TURN, GE_PICKUP, GE_DRINK, GE_FLY, GE_ERROR, GE_WIN, GE_COUNT
} GameEvent;

#define MAX_EVENTS 32
#define MAX_TURNS 300

typedef struct {
    Map map;
    Rng rng;
    Creature cr[MAX_CREATURES];
    int ncr;
    Object obj[MAX_OBJECTS];
    int nobj;
    Player pl[MAX_PLAYERS];
    int npl;
    int humans;
    int turn;
    int cur; /* player whose turn it is, NEUTRAL during the independent phase */
    int portal_x, portal_y, portal_turn;
    bool portal_open;
    bool over;
    int winner; /* player index or NONE */
    char msg[64];
    uint8_t ev[MAX_EVENTS];
    int nev;
} Game;

/* Setup. humans + ais must be 1..4. Humans take the first seats. */
bool game_init(Game *g, const Scenario *s, int humans, int ais, uint64_t seed);

/* Queries */
int game_creature_at(const Game *g, int x, int y, bool air);
int game_object_at(const Game *g, int x, int y);
bool game_on_map(const Creature *c);
bool game_is_wizard(const Creature *c);
int game_max_ap(const Game *g, int ci);
int game_combat(const Game *g, int ci);
int game_defence(const Game *g, int ci);
bool game_wounded(const Game *g, int ci);
bool game_engaged(const Game *g, int ci);
bool game_carries(const Game *g, int ci, int objtype);
bool game_can_see_creature(const Game *g, int player, int ci);
bool game_los(const Map *m, int x0, int y0, int x1, int y1);
void game_update_vis(Game *g, int player);
int game_count_creatures(const Game *g, int owner);
bool game_can_afford(const Game *g, int wi, int spell);
bool game_spell_needs_target(int spell);
int game_wizard_caster(const Game *g, int ci); /* ci or its rider if wizard */
int game_terrain_cost(const Game *g, int ci, int x, int y); /* -1 if impossible */

/* Actions */
Result game_move(Game *g, int ci, int dir);
Result game_attack(Game *g, int ci, int ti);
Result game_fly(Game *g, int ci);
Result game_land(Game *g, int ci);
Result game_cast(Game *g, int wi, int spell, int tx, int ty);
Result game_use_door(Game *g, int ci, int tx, int ty);
Result game_pickup(Game *g, int ci);
Result game_drop(Game *g, int ci);
Result game_drink(Game *g, int ci);
Result game_ride(Game *g, int ci, int mi);
Result game_dismount(Game *g, int mi);
void game_end_turn(Game *g);
/* Runs computer players until a human is to move or the game ends. */
void game_end_turn_from_ai(Game *g);

/* Events */
void game_event(Game *g, GameEvent e);
int game_pop_event(Game *g); /* -1 when empty */

/* Internal, exposed for tests */
int game_spawn(Game *g, int type, int owner, int x, int y);
void game_kill(Game *g, int ci, int killer_owner);
void game_damage(Game *g, int ti, int dmg, int attacker_owner);
uint32_t game_hash(const Game *g);

#endif
