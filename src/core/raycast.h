/* First-person grid raycaster drawing into the indexed framebuffer. */
#ifndef MAGES_RAYCAST_H
#define MAGES_RAYCAST_H
#include "fb.h"
#include "game.h"

/* Camera at (cx, cy) in map squares (square centres are at +0.5), angle in
   radians clockwise from north. Draws only creatures `viewer` can see
   (viewer NONE = all). The pov creature itself is skipped. */
void raycast_render(Framebuffer *fb, const Game *g, int viewer, int pov, float cx, float cy,
                    float ang, Rect r, uint32_t tick);

/* Angle for one of the eight facings. */
float raycast_facing_angle(int facing);

#endif
