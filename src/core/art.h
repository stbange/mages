/* Original pixel art. Sprites are 16x16 strings of palette hex digits,
   '.' is transparent, 'X' is the owner colour and 'x' its darker shade. */
#ifndef MAGES_ART_H
#define MAGES_ART_H
#include <stdint.h>

#define SPR 16
#define ICON 8

typedef const char *const Sprite[SPR];
typedef const char *const Icon[ICON];

const char *const *art_creature(int type);
/* Terrain tile; anim is a frame counter used by animated tiles (portal, water). */
const char *const *art_terrain(int terrain);
const char *const *art_object(int type);

enum { IC_UNDEAD, IC_FLY, IC_MOUNT, IC_WOUND, IC_INVIS,
       IC_AP, IC_STAMINA, IC_CON, IC_COMBAT, IC_DEFENCE, IC_MANA, IC_COUNT };
const char *const *art_icon(int icon);

/* Decode one pixel: returns palette index, or -1 for transparent. */
int art_pixel(char ch, int owner_colour);

#endif
