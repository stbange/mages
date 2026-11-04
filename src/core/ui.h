/* User interface state machine: turns abstract inputs into game actions. */
#ifndef MAGES_UI_H
#define MAGES_UI_H
#include "fb.h"
#include "game.h"
#include "save.h"
#include <stddef.h>

typedef enum {
    IN_NONE,
    /* keyboard / gamepad directions (cursor, 3D turning) */
    IN_UP, IN_DOWN, IN_LEFT, IN_RIGHT, IN_UPLEFT, IN_UPRIGHT, IN_DOWNLEFT, IN_DOWNRIGHT,
    IN_FIRE,   /* act on the cursor square (Enter/Space, gamepad A) */
    IN_CANCEL, /* Escape, right mouse button, gamepad B */
    IN_NEXT, IN_PREV,
    /* hotkeys */
    IN_VIEW, IN_CAST, IN_FLY, IN_PICKUP, IN_DROP, IN_DRINK, IN_USE, IN_RIDE, IN_END_TURN, IN_HELP,
    IN_DIGIT, /* x = 1..9 */
    /* pointer: x,y are screen coordinates (IN_DRAG: deltas, IN_WHEEL: y = +1 down / -1 up) */
    IN_POINT, IN_CLICK, IN_TAP, IN_DRAG, IN_WHEEL
} InputKind;

typedef struct { InputKind kind; int x, y; } Input;

typedef enum { M_TITLE, M_HANDOVER, M_PLAY, M_LIST, M_TARGET, M_VIEW3D, M_GAMEOVER, M_COUNT } Mode;

typedef enum {
    A_NONE, A_CAST, A_VIEW3D, A_FLY, A_LAND, A_PICKUP, A_DROP, A_DRINK, A_RIDE, A_DISMOUNT,
    A_DOOR, A_MORE, A_NEXT, A_PREV, A_END_TURN, A_HELP, A_CANCEL, A_SPELL, A_SPELL_PAGE,
    A_HUMANS, A_AIS, A_START, A_TURN_LEFT, A_TURN_RIGHT, A_FORWARD, A_EXIT3D,
    A_CONTINUE, A_MENU, A_QUIT, A_COUNT
} Action;

#define MENU_MAX 24

typedef struct {
    int n, sel;
    uint8_t act[MENU_MAX];
    uint8_t arg[MENU_MAX];
    bool disabled[MENU_MAX];
    char label[MENU_MAX][22];
    char title[22];
} Menu;

typedef struct {
    Rect r;
    uint8_t act;
    bool enabled;
    char label[10];
    char key[4];
} Button;

/* Screen layout */
#define VIEW_X 8
#define VIEW_Y 8
#define VIEW_TILES 11
#define VIEW_PX (VIEW_TILES * 16)
#define PANEL_X 192
#define MENU_Y 16
#define MENU_ROW 14
#define SPELLS_PER_PAGE 9
#define MSG_Y 189
#define BTN_Y 122
#define BTN_H 20
#define BTN_W 60
#define MAX_BUTTONS 6
#define WALK_TICKS 4
#define CONFIRM_TICKS 180
#define MAX_PATH 64

typedef enum { SAVE_NONE, SAVE_WRITE, SAVE_CLEAR } SaveRequest;

typedef struct {
    Game game;
    Mode mode;
    /* the current save (autosave); the platform persists it on request */
    uint8_t saved[SAVE_MAX_BYTES];
    size_t saved_len;
    SaveRequest save_request;
    int humans, ais;
    uint64_t seed;
    /* cursor and pointer */
    int cx, cy;          /* cursor square (follows mouse, keyboard or taps) */
    int px, py;          /* last pointer position on screen */
    bool cursor_shown;   /* cursor visible on the map (pointer over map or keyboard used) */
    bool touch;          /* last pointer input came from a touch screen */
    int armed_x, armed_y; /* touch: square previewed by the first tap, or -1 */
    int vx, vy;          /* top-left map square of the view */
    int drag_x, drag_y;  /* accumulated drag for square-by-square scrolling */
    int sel;             /* selected creature or NONE */
    /* lists (spell book, MORE) */
    Menu menu;
    bool list_spells;
    int spell_page;
    int pending_spell;
    /* movement range of the selection: AP cost and arriving direction */
    int16_t reach[MAP_MAX][MAP_MAX];
    int8_t reach_dir[MAP_MAX][MAP_MAX];
    /* animated walk */
    uint8_t path[MAX_PATH];
    int path_len, path_pos, walk_timer, walk_ci, walk_attack;
    int end_confirm; /* ticks left to confirm END TURN */
    bool help;
    /* first-person view */
    int pov;
    int pov_facing;
    float cam_x, cam_y, cam_ang;
    int view_drag;
    /* messages, timing, sound */
    char msg[64];
    int msg_timer;
    uint32_t tick;
    int sfx[16];
    int nsfx;
    int fx_timer; /* flashes the screen border after a hit */
} Ui;

void ui_init(Ui *u, uint64_t seed);
void ui_input(Ui *u, Input in);
void ui_tick(Ui *u);
int ui_pop_sfx(Ui *u);
void ui_render(const Ui *u, Framebuffer *fb);
/* Viewing player for rendering; NONE means "reveal everything". */
int ui_viewer(const Ui *u);
/* Screen rectangle of list entry i (valid while a list or the title is shown). */
Rect ui_menu_item_rect(const Ui *u, int i);
/* Screen position of the centre of map square (mx,my) if in view. */
bool ui_map_to_screen(const Ui *u, int mx, int my, int *sx, int *sy);
const char *ui_mode_name(Mode m);
void ui_message(Ui *u, const char *m);
void ui_start_game(Ui *u);
/* The action bar for the current state. Returns the number of buttons. */
int ui_buttons(const Ui *u, Button *out);
/* Message-line text describing what acting on the cursor square would do. */
void ui_hover_text(const Ui *u, char *out, size_t n);
/* AP needed to reach a square with the selection, or -1. */
int ui_reach_cost(const Ui *u, int x, int y);
/* True when the selection can reach and attack creature ci; *cost = total AP. */
bool ui_attackable(const Ui *u, int ci, int *cost);
/* True when the pending spell may be cast on the square. */
bool ui_valid_target(const Ui *u, int x, int y);
bool ui_walking(const Ui *u);
/* Offer stored save bytes at start-up; returns true when valid (adds CONTINUE). */
bool ui_offer_save(Ui *u, const uint8_t *buf, size_t len);
/* Returns and clears the pending save request for the platform. */
SaveRequest ui_take_save_request(Ui *u);
/* Creature whose details the panel shows (hovered, else selected), or NONE. */
int ui_panel_creature(const Ui *u);

#endif
