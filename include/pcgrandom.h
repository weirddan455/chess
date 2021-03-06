#ifndef PCG_RANDOM_H
#define PCG_RANDOM_H

#include <stdint.h>

typedef struct RngState
{
    uint64_t state;
    uint64_t inc;
} RngState;

extern RngState rngState;

uint32_t pcgGetRandom(void);
uint64_t pcgGetRandom64(void);
uint32_t pcgRangedRandom(uint32_t range);

#endif
