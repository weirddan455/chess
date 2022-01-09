#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>

void blitToScreen(void);
void *loadFile(const char *fileName);
void *allocateMemory(uint64_t size);
void freeMemory(void *ptr);

#endif
