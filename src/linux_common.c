#include "linux_common.h"
#include "renderer.h"

Display *display;
Window window;
XImage *ximage;
GC gc;
int screen;

void linuxBlitToScreen(void)
{
    XPutImage(display, window, gc, ximage, 0, 0, 0, 0, framebuffer.width, framebuffer.height);
}
