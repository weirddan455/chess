#ifndef WINDOWS_COMMON_H
#define WINDOWS_COMMON_H

#include <windows.h>
#include <stdbool.h>

extern HDC windowDC;
extern HDC frameBufferDC;

extern HANDLE event;

void windowsBlitToScreen(void);
void *windowsLoadFile(const char *fileName);
void windowsDebugLog(const char *message);
void windowsMakeComputerMove(void);

#endif
