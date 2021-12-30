#include "renderer.h"
#include "game.h"
#include "platform.h"

#include <string.h>

uint8_t *frameBuffer;

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

static void blitImage(Image *image, int cell)
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
        bufferIndex += (FRAMEBUFFER_WIDTH * 4) - (image->width * 4);
    }
}

static void drawPieces(void)
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

static void drawGrid(uint8_t *highlighted, int numHightlighted)
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
    while (i < FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT * 4)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            memcpy(&frameBuffer[i], gridColor, 4);
            i += 4;
        }
        if (i % ((FRAMEBUFFER_WIDTH * FRAMEBUFFER_HEIGHT * 4) / 8) != 0)
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
            bufferIndex += (FRAMEBUFFER_WIDTH * 4) - (GRID_SIZE * 4);
        }
    }
}

void renderFrame(uint8_t *highlighted, int numHighlighted)
{
    drawGrid(highlighted, numHighlighted);
    drawPieces();
    blitToScreen();
}
