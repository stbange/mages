/* The 16-colour 8-bit home computer palette used for every pixel. */
#ifndef MAGES_PALETTE_H
#define MAGES_PALETTE_H
#include <stdint.h>

enum {
    C_BLACK, C_WHITE, C_RED, C_CYAN, C_PURPLE, C_GREEN, C_BLUE, C_YELLOW,
    C_ORANGE, C_BROWN, C_LRED, C_DGREY, C_GREY, C_LGREEN, C_LBLUE, C_LGREY,
    C_COUNT
};

extern const uint32_t PALETTE_RGB[C_COUNT]; /* 0xRRGGBB */
/* One step darker colour of similar hue (used for distance fog / memory). */
extern const uint8_t PALETTE_DARKER[C_COUNT];

uint8_t palette_darken(uint8_t c, int steps);

#endif
