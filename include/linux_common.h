#ifndef LINUX_COMMON_H
#define LINUX_COMMON_H

#include <pthread.h>
#include <stdint.h>
#include <X11/Xlib.h>

extern Display *display;
extern Window window;
extern XImage *ximage;
extern GC gc;

extern pthread_mutex_t mutex;
extern pthread_cond_t cond;

void linuxBlitToScreen(void);
void *linuxLoadFile(const char *fileName);
void linuxDebugLog(const char *message);
void linuxMakeComputerMove(void);

#endif
