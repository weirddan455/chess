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
#include "renderer.h"
#include "linux_common.h"
#include "events.h"
#include "fonts.h"
#include "assets.h"

#define QUIT 1
#define RENDER 2
#define BLIT 4

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

static uint8_t handleNextEvent(int *newWidth, int *newHeight)
{
    XEvent event;
    XNextEvent(display, &event);
    switch(event.type)
    {
        case ClientMessage:
        {
            if (event.xclient.data.l[0] == wm_delete)
            {
                return QUIT;
            }
            break;
        }
        case Expose:
        {
            return BLIT;
        }
        case ConfigureNotify:
        {
            *newWidth = event.xconfigure.width;
            *newHeight = event.xconfigure.height;
            break;
        }
        case ButtonPress:
        {
            if (event.xbutton.button == 1)
            {
                leftClickEvent(event.xbutton.x, event.xbutton.y);
                return RENDER;
            }
            else if (event.xbutton.button == 3)
            {
                rightClickEvent();
                return RENDER;
            }
            break;
        }
    }
    return 0;
}

int main(void)
{
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
    loadFont();
    initGameState();
    renderFrame();

    while(true)
    {
        int newWidth = 0;
        int newHeight = 0;
        uint8_t flags = 0;
        do
        {
            flags |= handleNextEvent(&newWidth, &newHeight);
            if (flags & QUIT)
            {
                return 0;
            }
        } while(XPending(display) > 0);
        if (newWidth > 0 && newHeight > 0 && (newWidth != framebuffer.width || newHeight != framebuffer.height))
        {
            XDestroyImage(ximage);
            framebuffer.width = newWidth;
            framebuffer.height = newHeight;
            if (!newFramebuffer(newWidth, newHeight))
            {
                puts("Failed to resize framebuffer");
                return 0;
            }
            renderFrame();
        }
        else if (flags & RENDER)
        {
            renderFrame();
        }
        else if (flags & BLIT)
        {
            linuxBlitToScreen();
        }
    }

    return 0;
}
