#include "linux_common.h"
#include "renderer.h"

Display *display;
Window window;
XImage *ximage;
GC gc;

void linuxBlitToScreen(void)
{
    XPutImage(display, window, gc, ximage, 0, 0, 0, 0, FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT);
}
