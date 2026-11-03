#include "rng.h"

void rng_seed(Rng *r, uint64_t seed)
{
    r->s = seed ? seed : 0x9E3779B97F4A7C15ull;
}

uint32_t rng_next(Rng *r)
{
    uint64_t x = r->s;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    r->s = x;
    return (uint32_t)((x * 0x2545F4914F6CDD1Dull) >> 32);
}

int rng_range(Rng *r, int lo, int hi)
{
    if (hi <= lo) return lo;
    return lo + (int)(rng_next(r) % (uint32_t)(hi - lo + 1));
}

int rng_chance(Rng *r, int pct)
{
    return rng_range(r, 0, 99) < pct;
}
