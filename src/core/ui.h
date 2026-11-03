/* User interface state machine: turns abstract inputs into game actions. */
#ifndef MAGES_UI_H
#define MAGES_UI_H
#include "fb.h"
#include "game.h"

typedef enum {
    IN_NONE, IN_UP, IN_DOWN, IN_LEFT, IN_RIGHT, IN_UPLEFT, IN_UPRIGHT,
    IN_DOWNLEFT, IN_DOWNRIGHT, IN_FIRE, IN_CANCEL, IN_NEXT, IN_VIEW, IN_TAP
} InputKind;

typedef struct { InputKind kind; int x, y; } Input;

typedef enum {
    M_TITLE, M_HANDOVER, M_CURSOR, M_SELECTED, M_MENU, M_SPELLS, M_TARGET,
    M_VIEW3D, M_GAMEOVER, M_COUNT
} Mode;

typedef enum {
    A_CANCEL, A_SELECT_G, A_SELECT_A, A_NEXT, A_VIEW3D, A_END_TURN, A_CONFIRM_END,
    A_CONTINUE, A_CAST, A_FLY, A_LAND, A_PICKUP, A_DROP, A_DRINK, A_USE, A_RIDE,
    A_DISMOUNT, A_END_MOVE, A_HUMANS, A_AIS, A_START, A_SPELL, A_MORE, A_COUNT
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

typedef struct {
    Game game;
    Mode mode;
    Mode menu_from; /* mode to return to when a menu is cancelled */
    int humans, ais;
    uint64_t seed;
    int cx, cy;     /* cursor (map coordinates) */
    int vx, vy;     /* top-left map square of the view */
    int sel;        /* selected creature or NONE */
    Menu menu;
    int pending;    /* spell id or action awaiting a target */
    bool pending_is_spell;
    int spell_page;
    /* first-person view */
    int pov;
    int pov_facing;
    float cam_x, cam_y, cam_ang;
    Mode pov_from;
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
/* Screen rectangle of menu entry i (valid while a menu is shown). */
Rect ui_menu_item_rect(const Ui *u, int i);
/* Screen position of the centre of map square (mx,my) if in view. */
bool ui_map_to_screen(const Ui *u, int mx, int my, int *sx, int *sy);
const char *ui_mode_name(Mode m);
void ui_message(Ui *u, const char *m);
void ui_start_game(Ui *u);

#endif
