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

#include <ft2build.h>
#include FT_FREETYPE_H

#include "game.h"
#include "pcgrandom.h"
#include "renderer.h"
#include "linux_common.h"
#include "events.h"

static Atom wm_delete;

static void loadBmp(const char *fileName, Image *image)
{
    image->width = 0;
    image->height = 0;
    image->data = NULL;
    int fd = open(fileName, O_RDONLY);
    if (fd == -1)
    {
        perror(fileName);
        return;
    }
    struct stat fileInfo;
    if (fstat(fd, &fileInfo) != 0)
    {
        perror(fileName);
        close(fd);
        return;
    }
    uint8_t *bmpData = malloc(fileInfo.st_size);
    if (bmpData == NULL)
    {
        puts("malloc failed");
        close(fd);
        return;
    }
    ssize_t bytesRead = read(fd, bmpData, fileInfo.st_size);
    if (bytesRead == -1)
    {
        perror(fileName);
        close(fd);
        free(bmpData);
        return;
    }
    if (bytesRead != fileInfo.st_size)
    {
        printf("%s: read %ld bytes. %ld expected.\n", fileName, bytesRead, fileInfo.st_size);
        close(fd);
        free(bmpData);
        return;
    }
    close(fd);
    uint32_t startingPixelIndex = *(uint32_t *)&bmpData[10];
    int32_t width = *(int32_t *)&bmpData[18];
    int32_t height = *(int32_t *)&bmpData[22];
    uint32_t pixelDataSize = *(uint32_t *)&bmpData[34];
    uint32_t expectedDataSize = width * height * 4;
    if (expectedDataSize != pixelDataSize)
    {
        printf("%s: Expected %u bytes. Header reads %u\n", fileName, expectedDataSize, pixelDataSize);
        free(bmpData);
        return;
    }
    uint8_t *pixelData = malloc(pixelDataSize);
    if (pixelData == NULL)
    {
        puts("malloc failed");
        free(bmpData);
        return;
    }
    // BMP images have origin at bottom left.
    // Move data so that origin is top left matching X11.
    uint8_t *readPointer = bmpData + startingPixelIndex;
    uint8_t *writePointer = (pixelData + pixelDataSize) - (width * 4);
    for (int i = 0; i < height; i++)
    {
        memcpy(writePointer, readPointer, width * 4);
        readPointer += (width * 4);
        writePointer -= (width * 4);
    }
    free(bmpData);
    image->width = width;
    image->height = height;
    image->data = pixelData;
}

static void loadImages(void)
{
    loadBmp("images/black-bishop.bmp", &blackBishop);
    loadBmp("images/black-king.bmp", &blackKing);
    loadBmp("images/black-knight.bmp", &blackKnight);
    loadBmp("images/black-pawn.bmp", &blackPawn);
    loadBmp("images/black-queen.bmp", &blackQueen);
    loadBmp("images/black-rook.bmp", &blackRook);
    loadBmp("images/white-bishop.bmp", &whiteBishop);
    loadBmp("images/white-king.bmp", &whiteKing);
    loadBmp("images/white-knight.bmp", &whiteKnight);
    loadBmp("images/white-pawn.bmp", &whitePawn);
    loadBmp("images/white-queen.bmp", &whiteQueen);
    loadBmp("images/white-rook.bmp", &whiteRook);
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
            if (event.xconfigure.width != framebuffer.width || event.xconfigure.height != framebuffer.height)
            {
                XDestroyImage(ximage);
                framebuffer.width = event.xconfigure.width;
                framebuffer.height = event.xconfigure.height;
                framebuffer.data = malloc(framebuffer.width * framebuffer.height * 4);
                if (framebuffer.data == NULL)
                {
                    puts("malloc failed");
                    return false;
                }
                ximage = XCreateImage(
                    display, DefaultVisual(display, screen), DefaultDepth(display, screen), ZPixmap, 0,
                    (char *)framebuffer.data, framebuffer.width, framebuffer.height, 32, 0
                );
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

void loadGlyphs(void)
{
    int error;
    FT_Library library;
    error = FT_Init_FreeType(&library);
    if (error)
    {
        puts("Failed to initialize FreeType");
        return;
    }

    FT_Face face;
    error = FT_New_Face(library, "/usr/share/fonts/liberation/LiberationSans-Regular.ttf", 0, &face);
    if (error)
    {
        puts("Failed to get face");
        FT_Done_FreeType(library);
        return;
    }

    error = FT_Set_Pixel_Sizes(face, 0, 35);
    if (error)
    {
        puts("Failed to set pixel sizes");
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return;
    }

    error = FT_Load_Char(face, 'L', FT_LOAD_RENDER);
    if (error)
    {
        puts("Failed to load char");
        return;
    }

    int width = face->glyph->bitmap.width;
    int height = face->glyph->bitmap.rows;
    size_t bytes = width * height;

    glyphTest.data = malloc(bytes);
    if (glyphTest.data == NULL)
    {
        puts("malloc failed");
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return;
    }

    memcpy(glyphTest.data, face->glyph->bitmap.buffer, bytes);
    glyphTest.width = width;
    glyphTest.height = height;

    FT_Done_Face(face);
    FT_Done_FreeType(library);
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

    framebuffer.data = malloc(framebuffer.width * framebuffer.height * 4);
    if (framebuffer.data == NULL)
    {
        puts("malloc failed");
        return 1;
    }
    window = XCreateSimpleWindow(
        display, DefaultRootWindow(display),
        0, 0, framebuffer.width, framebuffer.height, 0, 0, 0
    );
    XSetWindowBackgroundPixmap(display, window, None);

    screen = DefaultScreen(display);

    XStoreName(display, window, "Chess");
    wm_delete = XInternAtom(display, "WM_DELETE_WINDOW", True);
    XSetWMProtocols(display, window, &wm_delete, 1);
    XSelectInput(display, window, ExposureMask | ButtonPressMask | StructureNotifyMask);
    XMapWindow(display, window);

    ximage = XCreateImage(
        display, DefaultVisual(display, screen), DefaultDepth(display, screen), ZPixmap, 0,
        (char *)framebuffer.data, framebuffer.width, framebuffer.height, 32, 0
    );
    gc = DefaultGC(display, screen);

    loadImages();
    loadGlyphs();
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
