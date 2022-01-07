#include "linux_common.h"
#include "renderer.h"

Display *display;
Window window;
XImage *ximage;
GC gc;
int screen;

int screenWidth;
int screenHeight;

void linuxBlitToScreen(void)
{
    int x = (screenWidth - frameBufferSize) / 2;
    int y = (screenHeight - frameBufferSize) / 2;
    XPutImage(display, window, gc, ximage, 0, 0, x, y, frameBufferSize, frameBufferSize);
}
