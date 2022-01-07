#ifndef LINUX_COMMON_H
#define LINUX_COMMON_H

#include <X11/Xlib.h>

extern Display *display;
extern Window window;
extern XImage *ximage;
extern GC gc;
extern int screen;

extern int screenWidth;
extern int screenHeight;

void linuxBlitToScreen(void);

#endif
