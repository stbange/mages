/* Deterministic xorshift64* random number generator. */
#ifndef MAGES_RNG_H
#define MAGES_RNG_H
#include <stdint.h>

typedef struct { uint64_t s; } Rng;

void rng_seed(Rng *r, uint64_t seed);
uint32_t rng_next(Rng *r);
/* Uniform integer in [lo, hi] (inclusive). Returns lo when hi <= lo. */
int rng_range(Rng *r, int lo, int hi);
/* True with probability pct/100. */
int rng_chance(Rng *r, int pct);

#endif
