/* Original 5x7 pixel font in 6x8 character cells (upper case only). */
#ifndef MAGES_FONT_H
#define MAGES_FONT_H
#include <stdint.h>

#define FONT_W 6
#define FONT_H 8

/* Returns 7 rows of 5 bits (bit 4 = leftmost) for the character, or a box
   for unknown characters. Lower case maps to upper case. */
const uint8_t *font_glyph(char c);

#endif
