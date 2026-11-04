/* Versioned, checksummed binary save format for a game in progress. */
#ifndef MAGES_SAVE_H
#define MAGES_SAVE_H
#include "game.h"
#include <stddef.h>

#define SAVE_VERSION 1
#define SAVE_MAX_BYTES 65536

typedef struct {
    int humans, ais;
    uint64_t seed;
} SaveMeta;

/* Serialise; returns bytes written or 0 if cap is too small. */
size_t save_write(const Game *g, const SaveMeta *m, uint8_t *buf, size_t cap);
/* Validate and load. On failure g and m are left untouched. */
bool save_read(Game *g, SaveMeta *m, const uint8_t *buf, size_t len);
uint32_t save_checksum(const uint8_t *p, size_t n);

#endif
