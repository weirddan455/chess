#ifndef LINUX_COMMON_H
#define LINUX_COMMON_H

#include <X11/Xlib.h>

extern Display *display;
extern Window window;
extern XImage *ximage;
extern GC gc;

void linuxBlitToScreen(void);

#endif
