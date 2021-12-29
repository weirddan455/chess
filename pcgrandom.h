#ifndef PCG_RANDOM_H
#define PCG_RANDOM_H

#include <stdbool.h>
#include <stdint.h>

typedef struct RngState
{
    uint64_t state;
    uint64_t inc;
} RngState;

extern RngState rngState;

uint32_t pcgRangedRandom(uint32_t range);

#endif
