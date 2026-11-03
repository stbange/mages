/* Computer wizards and independent creatures. */
#ifndef MAGES_AI_H
#define MAGES_AI_H
#include "game.h"

void ai_take_turn(Game *g, int player);
void ai_neutral_turn(Game *g);
/* Moves creature ci one step toward (tx,ty), attacking enemies in the way.
   Returns true if it acted. Exposed for tests. */
bool ai_step_toward(Game *g, int ci, int tx, int ty);

#endif
