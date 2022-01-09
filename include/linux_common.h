#ifndef LINUX_COMMON_H
#define LINUX_COMMON_H

#include <stdint.h>
#include <X11/Xlib.h>

extern Display *display;
extern Window window;
extern XImage *ximage;
extern GC gc;

void linuxBlitToScreen(void);
void *linuxAllocateMemory(uint64_t size);
void linuxFreeMemory(void *ptr);
void *linuxLoadFile(const char *fileName);

#endif
