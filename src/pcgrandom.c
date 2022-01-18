#include "pcgrandom.h"

RngState rngState;

uint32_t pcgGetRandom(void)
{
    uint64_t oldstate = rngState.state;
    rngState.state = oldstate * 6364136223846793005ULL + rngState.inc;
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((0 - rot) & 31));
}

uint32_t pcgRangedRandom(uint32_t range)
{
    uint32_t x = pcgGetRandom();
    uint64_t m = (uint64_t)x * (uint64_t)range;
    uint32_t l = (uint32_t)m;
    if (l < range)
    {
        uint32_t t = (0 - range) % range;
        while (l < t)
        {
            x = pcgGetRandom();
            m = (uint64_t)x * (uint64_t)range;
            l = (uint32_t)m;
        }
    }
    return m >> 32;
}
