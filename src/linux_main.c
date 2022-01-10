#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "game.h"
#include "pcgrandom.h"
#include "renderer.h"
#include "linux_common.h"
#include "events.h"
#include "fonts.h"
#include "assets.h"

static Atom wm_delete;
static Visual *visual;
static unsigned int depth;

static bool newFramebuffer(int width, int height)
{
    framebuffer.data = malloc(width * height * 4);
    if (framebuffer.data == NULL)
    {
        puts("malloc failed");
        return false;
    }
    ximage = XCreateImage(display, visual, depth, ZPixmap, 0, (char *)framebuffer.data, width, height, 32, 0);
    return true;
}

static bool handleNextEvent(void)
{
    XEvent event;
    XNextEvent(display, &event);
    switch(event.type)
    {
        case ClientMessage:
        {
            if (event.xclient.data.l[0] == wm_delete)
            {
                return false;
            }
            break;
        }
        case Expose:
        {
            linuxBlitToScreen();
            break;
        }
        case ConfigureNotify:
        {
            if (event.xconfigure.width == 0 || event.xconfigure.height == 0)
            {
                break;
            }
            if (event.xconfigure.width != framebuffer.width || event.xconfigure.height != framebuffer.height)
            {
                XDestroyImage(ximage);
                if (!newFramebuffer(event.xconfigure.width, event.xconfigure.height))
                {
                    puts("Failed to resize framebuffer");
                    return false;
                }
                framebuffer.width = event.xconfigure.width;
                framebuffer.height = event.xconfigure.height;
                renderFrame(NULL, 0);
            }
            break;
        }
        case ButtonPress:
        {
            if (event.xbutton.button == 1)
            {
                if (!leftClickEvent(event.xbutton.x, event.xbutton.y))
                {
                    puts("Game Over");
                    return false;
                }
            }
            else if (event.xbutton.button == 3)
            {
                rightClickEvent();
            }
            break;
        }
    }
    return true;
}

static bool seedRng(void)
{
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd == -1)
    {
        printf("Failed to open /dev/urandom: %s\n", strerror(errno));
        return false;
    }
    uint64_t randomBuffer[2];
    if (read(fd, randomBuffer, 16) != 16)
    {
        close(fd);
        return false;
    }
    rngState.state = randomBuffer[0];
    rngState.inc = randomBuffer[1] | 1;
    close(fd);
    return true;
}

int main(void)
{
    if (!seedRng())
    {
        puts("Failed to seed RNG");
        return 1;
    }
    display = XOpenDisplay(NULL);
    if (display == NULL)
    {
        puts("Failed to open display");
        return 1;
    }

    framebuffer.width = 720;
    framebuffer.height = 720;

    window = XCreateSimpleWindow(
        display, DefaultRootWindow(display),
        0, 0, framebuffer.width, framebuffer.height, 0, 0, 0
    );
    XSetWindowBackgroundPixmap(display, window, None);

    XStoreName(display, window, "Chess");
    wm_delete = XInternAtom(display, "WM_DELETE_WINDOW", True);
    XSetWMProtocols(display, window, &wm_delete, 1);
    XSelectInput(display, window, ExposureMask | ButtonPressMask | StructureNotifyMask);
    XMapWindow(display, window);

    int screen = DefaultScreen(display);
    visual = DefaultVisual(display, screen);
    depth = DefaultDepth(display, screen);
    gc = DefaultGC(display, screen);

    if (!newFramebuffer(framebuffer.width, framebuffer.height))
    {
        puts("Failed to initalize framebuffer");
        return 1;
    }

    loadImages();
    loadGlyph();
    initGameState();
    renderFrame(NULL, 0);

    while(true)
    {
        if (!handleNextEvent())
        {
            break;
        }
    }

    return 0;
}
