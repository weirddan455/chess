#ifndef PCG_RANDOM_H
#define PCG_RANDOM_H

#include <stdbool.h>
#include <stdint.h>

bool seedRng(void);
uint32_t pcgGetRandom(void);
uint32_t pcgRangedRandom(uint32_t range);

#endif
