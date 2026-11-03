/* Common definitions shared by all core modules. */
#ifndef MAGES_DEFS_H
#define MAGES_DEFS_H

#include <stdbool.h>
#include <stdint.h>

#define SCREEN_W 320
#define SCREEN_H 200

#define MAX_PLAYERS 4
#define MAX_CREATURES 160
#define MAX_OBJECTS 64
#define MAX_CARRY 4
#define NEUTRAL (-1)
#define NONE (-1)

/* Eight compass directions, clockwise from north. */
enum { DIR_N, DIR_NE, DIR_E, DIR_SE, DIR_S, DIR_SW, DIR_W, DIR_NW, DIR_COUNT };

extern const int8_t DIR_DX[DIR_COUNT];
extern const int8_t DIR_DY[DIR_COUNT];

int dir_from_delta(int dx, int dy); /* -1 when dx==dy==0 */
int clampi(int v, int lo, int hi);
int imax(int a, int b);
int imin(int a, int b);
int iabs(int v);

#endif
