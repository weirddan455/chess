#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <X11/Xlib.h>

#include "game.h"
#include "pcgrandom.h"

#define WIDTH 720
#define HEIGHT 720
#define GRID_SIZE (WIDTH / 8)

typedef struct Image
{
    int width;
    int height;
    uint8_t *data;
} Image;

Display *display;
Window window;
Atom wm_delete;
uint8_t *frameBuffer;
int screen;
XImage *ximage;
GC gc;

Image blackBishop;
Image blackKing;
Image blackKnight;
Image blackPawn;
Image blackQueen;
Image blackRook;
Image whiteBishop;
Image whiteKing;
Image whiteKnight;
Image whitePawn;
Image whiteQueen;
Image whiteRook;

void loadBmp(const char *fileName, Image *image)
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
    uint32_t bmpDataIndex = startingPixelIndex;
    uint32_t pixelDataIndex = pixelDataSize - (width * 4);
    for (int i = 0; i < height; i++)
    {
        memcpy(&pixelData[pixelDataIndex], &bmpData[bmpDataIndex], width * 4);
        bmpDataIndex += (width * 4);
        pixelDataIndex -= (width * 4);
    }
    free(bmpData);
    image->width = width;
    image->height = height;
    image->data = pixelData;
}

void loadImages(void)
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

void blitImage(Image *image, int cell)
{
    int x = cell % 8;
    int y = cell / 8;
    int borderX = (GRID_SIZE - image->width) / 2;
    int borderY = (GRID_SIZE - image->height) / 2;
    uint64_t bufferIndex = GRID_SIZE * 4 * x;
    bufferIndex += borderX * 4;
    bufferIndex += GRID_SIZE * 4 * 8 * GRID_SIZE * y;
    bufferIndex += borderY * 4 * 8 * GRID_SIZE;
    uint64_t dataIndex = 0;
    for (int i = 0; i < image->height; i++)
    {
        for (int j = 0; j < image->width; j++)
        {
            float alpha = image->data[dataIndex + 3];
            alpha = alpha / 255.0f;
            float inverseAlpha = 1.0f - alpha;
            frameBuffer[bufferIndex] = (image->data[dataIndex] * alpha) + (frameBuffer[bufferIndex] * inverseAlpha);
            bufferIndex++;
            dataIndex++;
            frameBuffer[bufferIndex] = (image->data[dataIndex] * alpha) + (frameBuffer[bufferIndex] * inverseAlpha);
            bufferIndex++;
            dataIndex++;
            frameBuffer[bufferIndex] = (image->data[dataIndex] * alpha) + (frameBuffer[bufferIndex] * inverseAlpha);
            bufferIndex += 2;
            dataIndex += 2;
        }
        bufferIndex += (WIDTH * 4) - (image->width * 4);
    }
}

void drawPieces(void)
{
    for (int i = 0; i < 64; i++)
    {
        if (gameState.board[i] != NULL)
        {
            Image *image;
            if (gameState.board[i]->owner == BLACK)
            {
                switch(gameState.board[i]->type)
                {
                    case ROOK:
                        image = &blackRook;
                        break;
                    case KNIGHT:
                        image = &blackKnight;
                        break;
                    case BISHOP:
                        image = &blackBishop;
                        break;
                    case QUEEN:
                        image = &blackQueen;
                        break;
                    case KING:
                        image = &blackKing;
                        break;
                    default:
                        image = &blackPawn;
                        break;
                }
            }
            else
            {
                switch(gameState.board[i]->type)
                {
                    case ROOK:
                        image = &whiteRook;
                        break;
                    case KNIGHT:
                        image = &whiteKnight;
                        break;
                    case BISHOP:
                        image = &whiteBishop;
                        break;
                    case QUEEN:
                        image = &whiteQueen;
                        break;
                    case KING:
                        image = &whiteKing;
                        break;
                    default:
                        image = &whitePawn;
                        break;
                }
            }
            blitImage(image, i);
        }
    }
}

void drawGrid(uint8_t *highlighted, int numHightlighted)
{
    uint8_t black[4];
    black[0] = 10;
    black[1] = 56;
    black[2] = 75;
    black[3] = 0;
    uint8_t white[4];
    white[0] = 122;
    white[1] = 217;
    white[2] = 255;
    white[3] = 0;
    uint8_t blackHighlightColor[4];
    blackHighlightColor[0] = 13;
    blackHighlightColor[1] = 129;
    blackHighlightColor[2] = 133;
    blackHighlightColor[3] = 0;
    uint8_t whiteHightlightColor[4];
    whiteHightlightColor[0] = 16;
    whiteHightlightColor[1] = 217;
    whiteHightlightColor[2] = 224;
    whiteHightlightColor[3] = 0;
    uint8_t *gridColor = white;
    int i = 0;
    while (i < WIDTH * HEIGHT * 4)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            memcpy(&frameBuffer[i], gridColor, 4);
            i += 4;
        }
        if (i % ((WIDTH * HEIGHT * 4) / 8) != 0)
        {
            if (gridColor == white)
            {
                gridColor = black;
            }
            else
            {
                gridColor = white;
            }
        }
    }
    for (i = 0; i < numHightlighted; i++)
    {
        uint8_t x = highlighted[i] % 8;
        uint8_t y = highlighted[i] / 8;
        uint8_t *highlightColor;
        if (y % 2 == 0)
        {
            if (x % 2 == 0)
            {
                highlightColor = whiteHightlightColor;
            }
            else
            {
                highlightColor = blackHighlightColor;
            }
        }
        else if (x % 2 == 0)
        {
            highlightColor = blackHighlightColor;
        }
        else
        {
            highlightColor = whiteHightlightColor;
        }
        uint64_t bufferIndex = GRID_SIZE * 4 * x;
        bufferIndex += GRID_SIZE * 4 * 8 * GRID_SIZE * y;
        for (int y = 0; y < GRID_SIZE; y++)
        {
            for (int x = 0; x < GRID_SIZE; x++)
            {
                memcpy(&frameBuffer[bufferIndex], highlightColor, 4);
                bufferIndex += 4;
            }
            bufferIndex += (WIDTH * 4) - (GRID_SIZE * 4);
        }
    }
}

void renderFrame(uint8_t *highlighted, int numHighlighted)
{
    drawGrid(highlighted, numHighlighted);
    drawPieces();
    XPutImage(display, window, gc, ximage, 0, 0, 0, 0, WIDTH, HEIGHT);
}

bool handleNextEvent(void)
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
            XPutImage(display, window, gc, ximage, 0, 0, 0, 0, WIDTH, HEIGHT);
            break;
        }
        case ButtonPress:
        {
            static bool selected;
            static uint8_t moveFrom;
            static uint8_t moves[64];
            static int numMoves;
            if (event.xbutton.button == 1)
            {
                if (event.xbutton.x >= 0 && event.xbutton.y >= 0 && event.xbutton.x < WIDTH && event.xbutton.y < HEIGHT)
                {
                    uint8_t cellX = event.xbutton.x / GRID_SIZE;
                    uint8_t cellY = event.xbutton.y / GRID_SIZE;
                    uint8_t cell = (cellY * 8) + cellX;
                    if (gameState.board[cell] != NULL && gameState.board[cell]->owner == WHITE)
                    {
                        numMoves = pieceLegalMoves(cell, moves);
                        if (numMoves > 0)
                        {
                            selected = true;
                            moveFrom = cell;
                            uint8_t highlighted[64];
                            for (int i = 0; i < numMoves; i++)
                            {
                                highlighted[i] = moves[i] & MOVE_MASK;
                            }
                            highlighted[numMoves] = cell;
                            renderFrame(highlighted, numMoves + 1);
                        }
                    }
                    else if (selected)
                    {
                        uint8_t moveTo = 255;
                        for (int i = 0; i < numMoves; i++)
                        {
                            if ((moves[i] & MOVE_MASK) == cell)
                            {
                                moveTo = moves[i];
                                break;
                            }
                        }
                        if (moveTo != 255)
                        {
                            movePiece(moveTo, moveFrom, &gameState);
                            selected = false;
                            uint8_t computerMoveTo[1024];
                            uint8_t computerMoveFrom[1024];
                            int numComputerMoves = getAllLegalMoves(BLACK, computerMoveTo, computerMoveFrom);
                            if (numComputerMoves <= 0)
                            {
                                puts("Game over");
                                return false;
                            }
                            uint32_t randomMove = pcgRangedRandom(numComputerMoves);
                            movePiece(computerMoveTo[randomMove], computerMoveFrom[randomMove], &gameState);
                            renderFrame(NULL, 0);
                        }
                    }
                }
            }
            else if (event.xbutton.button == 3)
            {
                selected = false;
                renderFrame(NULL, 0);
            }
            break;
        }
    }
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

    frameBuffer = malloc(WIDTH * HEIGHT * 4);
    if (frameBuffer == NULL)
    {
        puts("malloc failed");
        return 1;
    }

    window = XCreateSimpleWindow(
        display, DefaultRootWindow(display),
        0, 0, WIDTH, HEIGHT, 0, 0, 0
    );

    screen = DefaultScreen(display);

    XStoreName(display, window, "Chess");
    wm_delete = XInternAtom(display, "WM_DELETE_WINDOW", True);
    XSetWMProtocols(display, window, &wm_delete, 1);
    XSelectInput(display, window, ExposureMask | ButtonPressMask);
    XMapWindow(display, window);

    ximage = XCreateImage(
        display, DefaultVisual(display, screen), DefaultDepth(display, screen), ZPixmap, 0,
        (char *)frameBuffer, WIDTH, HEIGHT, 32, 0
    );
    gc = DefaultGC(display, screen);

    loadImages();
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
