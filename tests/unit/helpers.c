#include "test.h"

static const char *const ROWS[] = {
    "................",
    "................",
    "..#####.........",
    "..#,,,#.........",
    "..#,*,+.........",
    "..#,,,#.........",
    "..#####.........",
    "................",
    "........~~......",
    "........~~......",
    "....T...........",
    "....%...^.......",
    "................",
    "................",
    "................",
    "...........o....",
};

const Scenario TEST_SCENARIO = {
    "TEST", ROWS, 16,
    {4, 12, 12, 3}, {4, 12, 3, 12},
    NULL, 0, NULL, 0,
    11, 15, 5,
};

void setup(Game *g, int humans, int ais)
{
    game_init(g, &TEST_SCENARIO, humans, ais, 1);
    g->turn = 2;
    g->nev = 0;
}

int spawn(Game *g, int type, int owner, int x, int y)
{
    int i = game_spawn(g, type, owner, x, y);
    return i;
}
