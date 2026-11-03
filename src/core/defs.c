#include "defs.h"

const int8_t DIR_DX[DIR_COUNT] = {0, 1, 1, 1, 0, -1, -1, -1};
const int8_t DIR_DY[DIR_COUNT] = {-1, -1, 0, 1, 1, 1, 0, -1};

int dir_from_delta(int dx, int dy)
{
    int sx = (dx > 0) - (dx < 0);
    int sy = (dy > 0) - (dy < 0);
    for (int d = 0; d < DIR_COUNT; d++) {
        if (DIR_DX[d] == sx && DIR_DY[d] == sy) return d;
    }
    return -1;
}

int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
int imax(int a, int b) { return a > b ? a : b; }
int imin(int a, int b) { return a < b ? a : b; }
int iabs(int v) { return v < 0 ? -v : v; }
