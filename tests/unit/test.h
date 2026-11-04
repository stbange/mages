/* Minimal unit test harness. */
#ifndef MAGES_TEST_H
#define MAGES_TEST_H
#include <stdio.h>
#include <string.h>
#include "game.h"

extern int t_fail, t_count;

#define CHECK(c) do { t_count++; if (!(c)) { t_fail++; \
    printf("%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #c); } } while (0)
#define CHECK_EQ(a, b) do { long long _a = (long long)(a), _b = (long long)(b); t_count++; \
    if (_a != _b) { t_fail++; printf("%s:%d: %s == %s failed (%lld != %lld)\n", \
    __FILE__, __LINE__, #a, #b, _a, _b); } } while (0)
#define CHECK_STR(a, b) do { t_count++; if (strcmp((a), (b)) != 0) { t_fail++; \
    printf("%s:%d: \"%s\" != \"%s\"\n", __FILE__, __LINE__, (a), (b)); } } while (0)

/* A small 16x16 test world (see helpers.c). */
extern const Scenario TEST_SCENARIO;
/* Initialise a game on the test world and skip to turn 2 so creatures may move. */
void setup(Game *g, int humans, int ais);
/* Spawn and return a creature with full AP. */
int spawn(Game *g, int type, int owner, int x, int y);

void test_basic(void);
void test_game(void);
void test_spells(void);
void test_ai(void);
void test_audio(void);
void test_render(void);
void test_ui(void);
void test_script(void);
void test_save(void);

#endif
