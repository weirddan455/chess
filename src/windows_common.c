#include "windows_common.h"
#include "renderer.h"

HDC windowDC;
HDC frameBufferDC;

void windowsBlitToScreen(void)
{
	BitBlt(windowDC, 0, 0, framebuffer.width, framebuffer.height, frameBufferDC, 0, 0, SRCCOPY);
}
