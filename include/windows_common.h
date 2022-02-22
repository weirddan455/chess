#ifndef WINDOWS_COMMON_H
#define WINDOWS_COMMON_H

#include <windows.h>
#include <stdbool.h>

extern HDC windowDC;
extern HDC frameBufferDC;

extern CONDITION_VARIABLE cond;
extern SRWLOCK lock;
extern bool AIThreadWakeup;

void windowsBlitToScreen(void);
void *windowsLoadFile(const char *fileName);
void windowsDebugLog(const char *message);
void windowsMakeComputerMove(void);

#endif
