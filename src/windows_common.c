#include "windows_common.h"
#include "renderer.h"

HDC windowDC;
HDC frameBufferDC;

void windowsBlitToScreen(void)
{
	BitBlt(windowDC, 0, 0, FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT, frameBufferDC, 0, 0, SRCCOPY);
}
