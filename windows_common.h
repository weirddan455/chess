#ifndef WINDOWS_COMMON_H
#define WINDOWS_COMMON_H

#include <windows.h>
#include <stdint.h>

extern HDC windowDC;
extern HDC frameBufferDC;

void windowsBlitToScreen(void);

#endif
