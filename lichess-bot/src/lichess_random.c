#include "lichess_random.h"

static uint32_t pcgGetRandom(RngState *rng)
{
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((0 - rot) & 31));
}

uint32_t pcgRangedRandom(uint32_t range, RngState *rng)
{
    uint32_t x = pcgGetRandom(rng);
    uint64_t m = (uint64_t)x * (uint64_t)range;
    uint32_t l = (uint32_t)m;
    if (l < range)
    {
        uint32_t t = (0 - range) % range;
        while (l < t)
        {
            x = pcgGetRandom(rng);
            m = (uint64_t)x * (uint64_t)range;
            l = (uint32_t)m;
        }
    }
    return m >> 32;
}
