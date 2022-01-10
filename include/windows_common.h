#ifndef WINDOWS_COMMON_H
#define WINDOWS_COMMON_H

#include <windows.h>

extern HDC windowDC;
extern HDC frameBufferDC;

void windowsBlitToScreen(void);
void *windowsLoadFile(const char *fileName);
void windowsDebugLog(const char *message);

#endif
