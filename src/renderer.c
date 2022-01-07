#include "renderer.h"
#include "game.h"
#include "platform.h"

#include <string.h>

// Framebuffer is always a square.
// frameBufferSize is number of pixels wide.
uint8_t *frameBuffer;
int frameBufferSize;

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
Image glyphTest;

static void drawGlyph(int x, int y)
{
    x *= 4;
    y *= 4;
    uint8_t *writePointer = frameBuffer + x + (y * frameBufferSize);
    uint8_t *readPointer = glyphTest.data;
    for (int h = 0; h < glyphTest.height; h++)
    {
        for (int w = 0; w < glyphTest.width; w++)
        {
            float alpha = 1.0f - (*readPointer / 255.0f);
            for (int i = 0; i < 4; i++)
            {
                *writePointer *= alpha;
                writePointer++;
            }
            readPointer++;
        }
        writePointer += (frameBufferSize - glyphTest.width) * 4;
    }
}

static void blitImage(Image image, int cell)
{
    int gridSize = frameBufferSize / 8;
    int x = cell % 8;
    int y = cell / 8;
    int borderX = (gridSize - image.width) / 2;
    int borderY = (gridSize - image.height) / 2;
    uint8_t *frameBufferPointer = frameBuffer;
    frameBufferPointer += gridSize * 4 * x;
    frameBufferPointer += borderX * 4;
    frameBufferPointer += gridSize * 4 * 8 * gridSize * y;
    frameBufferPointer += borderY * 4 * 8 * gridSize;
    uint8_t *imagePointer = image.data;
    int yAdvance = (frameBufferSize * 4) - (image.width * 4);
    for (int i = 0; i < image.height; i++)
    {
        for (int j = 0; j < image.width; j++)
        {
            float alpha = imagePointer[3];
            alpha = alpha / 255.0f;
            float inverseAlpha = 1.0f - alpha;
            *frameBufferPointer = (*imagePointer * alpha) + (*frameBufferPointer * inverseAlpha);
            imagePointer++;
            frameBufferPointer++;
            *frameBufferPointer = (*imagePointer * alpha) + (*frameBufferPointer * inverseAlpha);
            imagePointer++;
            frameBufferPointer++;
            *frameBufferPointer = (*imagePointer * alpha) + (*frameBufferPointer * inverseAlpha);
            imagePointer += 2;
            frameBufferPointer += 2;
        }
        frameBufferPointer += yAdvance;
    }
}

static void drawPieces(void)
{
    for (int i = 0; i < 64; i++)
    {
        if (gameState.board[i] != NULL)
        {
            Image image;
            if (gameState.board[i]->owner == BLACK)
            {
                switch(gameState.board[i]->type)
                {
                    case ROOK:
                        image = blackRook;
                        break;
                    case KNIGHT:
                        image = blackKnight;
                        break;
                    case BISHOP:
                        image = blackBishop;
                        break;
                    case QUEEN:
                        image = blackQueen;
                        break;
                    case KING:
                        image = blackKing;
                        break;
                    default:
                        image = blackPawn;
                        break;
                }
            }
            else
            {
                switch(gameState.board[i]->type)
                {
                    case ROOK:
                        image = whiteRook;
                        break;
                    case KNIGHT:
                        image = whiteKnight;
                        break;
                    case BISHOP:
                        image = whiteBishop;
                        break;
                    case QUEEN:
                        image = whiteQueen;
                        break;
                    case KING:
                        image = whiteKing;
                        break;
                    default:
                        image = whitePawn;
                        break;
                }
            }
            blitImage(image, i);
        }
    }
}

static void drawGrid(uint8_t *highlighted, int numHightlighted)
{
    int gridSize = frameBufferSize / 8;
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
    uint8_t *writePointer = frameBuffer;
    int frameBufferPixels = frameBufferSize * frameBufferSize;
    int pixel = 0;
    while (pixel < frameBufferPixels)
    {
        for (int j = 0; j < gridSize; j++)
        {
            memcpy(writePointer, gridColor, 4);
            pixel++;
            writePointer += 4;
        }
        if (pixel % (frameBufferPixels / 8) != 0)
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
    int yAdvance = (frameBufferSize * 4) - (gridSize * 4);
    for (int i = 0; i < numHightlighted; i++)
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
        writePointer = frameBuffer + (gridSize * 4 * x) + (gridSize * 4 * 8 * gridSize * y);
        for (int y = 0; y < gridSize; y++)
        {
            for (int x = 0; x < gridSize; x++)
            {
                memcpy(writePointer, highlightColor, 4);
                writePointer += 4;
            }
            writePointer += yAdvance;
        }
    }
}

void renderFrame(uint8_t *highlighted, int numHighlighted)
{
    drawGrid(highlighted, numHighlighted);
    drawPieces();
    blitToScreen();
}
