#ifndef PLATFORM_H
#define PLATFORM_H

#define LOG_SIZE 256

void blitToScreen(void);
void *loadFile(const char *fileName);
void debugLog(const char *message);
void makeComputerMove(void);

#endif
