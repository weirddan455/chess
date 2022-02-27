#ifndef PCG_RANDOM_H
#define PCG_RANDOM_H

#include <stdint.h>

typedef struct RngState
{
    uint64_t state;
    uint64_t inc;
} RngState;

uint32_t pcgRangedRandom(uint32_t range, RngState *rng);

#endif
