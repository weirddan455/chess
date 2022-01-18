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

static uint8_t handleNextEvent(int *newWidth, int *newHeight, bool playerGame)
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
                if (playerGame)
                {
                    leftClickEvent(event.xbutton.x, event.xbutton.y);
                    return RENDER;
                }
                else if (gameOverString != NULL)
                {
                    gameOverString = NULL;
                    initGameState();
                    return RENDER;
                }
            }
            else if (event.xbutton.button == 3)
            {
                if (playerGame)
                {
                    rightClickEvent();
                    return RENDER;
                }
            }
            break;
        }
    }
    return 0;
}

static void AIVsAILoop(void)
{
    while(true)
    {
        int newWidth = 0;
        int newHeight = 0;
        uint8_t flags = 0;
        while (XPending(display) > 0)
        {
            flags |= handleNextEvent(&newWidth, &newHeight, false);
            if (flags & QUIT)
            {
                return;
            }
        }
        if (newWidth > 0 && newHeight > 0 && (newWidth != framebuffer.width || newHeight != framebuffer.height))
        {
            XDestroyImage(ximage);
            framebuffer.width = newWidth;
            framebuffer.height = newHeight;
            if (!newFramebuffer(newWidth, newHeight))
            {
                puts("Failed to resize framebuffer");
                return;
            }
        }
        uint16_t move = getComputerMove(gameState.playerToMove);
        movePiece(move, &gameState);
        enum GameEnd end = checkGameEnd(&gameState, gameState.playerToMove);
        if (end == CHECKMATE)
        {
            if (gameState.playerToMove == WHITE)
            {
                gameOverString = "Black Wins - Checkmate";
            }
            else
            {
                gameOverString = "White Wins - Checkmate";
            }
        }
        else if (end == STALEMATE)
        {
            gameOverString = "Stalemate";
        }
        else if (end == DRAW_50_MOVE)
        {
            gameOverString = "Draw by 50 move rule";
        }
        renderFrame();
        while (gameOverString != NULL)
        {
            flags = 0;
            newWidth = 0;
            newHeight = 0;
            do
            {
                flags |= handleNextEvent(&newWidth, &newHeight, false);
                if (flags & QUIT)
                {
                    return;
                }
            } while (XPending(display) > 0);
            if (newWidth > 0 && newHeight > 0 && (newWidth != framebuffer.width || newHeight != framebuffer.height))
            {
                XDestroyImage(ximage);
                framebuffer.width = newWidth;
                framebuffer.height = newHeight;
                if (!newFramebuffer(newWidth, newHeight))
                {
                    puts("Failed to resize framebuffer");
                    return;
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
    }
}

static void playerVsAILoop(void)
{
    while(true)
    {
        int newWidth = 0;
        int newHeight = 0;
        uint8_t flags = 0;
        do
        {
            flags |= handleNextEvent(&newWidth, &newHeight, true);
            if (flags & QUIT)
            {
                return;
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
                return;
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

int main(int argc, char **argv)
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
    loadFont();
    initGameState();
    renderFrame();

    if (argc > 1 && strcmp(argv[1], "-ai") == 0)
    {
        AIVsAILoop();
    }
    else
    {
        playerVsAILoop();
    }

    return 0;
}
