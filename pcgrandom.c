#include "pcgrandom.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct RngState
{
    uint64_t state;
    uint64_t inc;
} RngState;

RngState rngState;

// TODO: Need code for Windows. /dev/urandom works in Emscripten and Unix systems.
bool seedRng(void)
{
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1)
    {
        printf("Failed to open /dev/urandom: %s\n", strerror(errno));
        return false;
    }
    uint64_t randomBuffer[2];
    if (read(fd, randomBuffer, 16) != 16)
    {
        close(fd);
        return false;
    }
    rngState.state = randomBuffer[0];
    rngState.inc = randomBuffer[1] | 1;
    close(fd);
    return true;
}

uint32_t pcgGetRandom(void)
{
    uint64_t oldstate = rngState.state;
    rngState.state = oldstate * 6364136223846793005ULL + rngState.inc;
    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
    uint32_t rot = oldstate >> 59u;
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

uint32_t pcgRangedRandom(uint32_t range)
{
    uint32_t x = pcgGetRandom();
    uint64_t m = (uint64_t)x * (uint64_t)range;
    uint32_t l = (uint32_t)m;
    if (l < range)
    {
        uint32_t t = -range % range;
        while (l < t)
        {
            x = pcgGetRandom();
            m = (uint64_t)x * (uint64_t)range;
            l = (uint32_t)m;
        }
    }
    return m >> 32;
}
