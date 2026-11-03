/* Scenario definitions (map, starts, neutral creatures, objects, portal). */
#ifndef MAGES_SCENARIO_H
#define MAGES_SCENARIO_H
#include "defs.h"

typedef struct { uint8_t type; int16_t x, y; } Placement;

typedef struct {
    const char *name;
    const char *const *rows;
    int h;
    int start_x[MAX_PLAYERS], start_y[MAX_PLAYERS];
    const Placement *neutrals; int nneutrals;
    const Placement *objects; int nobjects;
    int portal_x, portal_y, portal_turn;
} Scenario;

extern const Scenario SCENARIO_VALE;

#endif
