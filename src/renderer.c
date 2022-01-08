#include "renderer.h"
#include "game.h"
#include "platform.h"

#include <string.h>

Image framebuffer;
GameArea gameArea;

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

GameArea getGameArea(void)
{
    GameArea gameArea;
    if (framebuffer.width < framebuffer.height)
    {
        gameArea.size = framebuffer.width;
    }
    else
    {
        gameArea.size = framebuffer.height;
    }
    gameArea.size -= gameArea.size % 8;
    gameArea.gridSize = gameArea.size / 8;
    gameArea.x = (framebuffer.width - gameArea.size) / 2;
    gameArea.y = (framebuffer.height - gameArea.size) / 2;
    gameArea.buffer = (framebuffer.data + (gameArea.x * 4)) + (gameArea.y * framebuffer.width * 4);
    return gameArea;
}

static void drawGlyph(int x, int y)
{
    x *= 4;
    y *= 4;
    uint8_t *writePointer = framebuffer.data + x + (y * framebuffer.width);
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
        writePointer += (framebuffer.width - glyphTest.width) * 4;
    }
}

static void scaleImage(Image image, int cell)
{
    GameArea gameArea = getGameArea();
    int bufferSquare = gameArea.gridSize - 10;
    if (bufferSquare < 1)
    {
        return;
    }
    int scaledWidth;
    int scaledHeight;
    if (image.width > image.height)
    {
        float scale = (float)bufferSquare / (float)image.width;
        scaledWidth = bufferSquare;
        scaledHeight = ((float)image.height * scale) + 0.5f;
    }
    else
    {
        float scale = (float)bufferSquare / (float)image.height;
        scaledWidth = ((float)image.width * scale) + 0.5f;
        scaledHeight = bufferSquare;
    }
    float scaleX = (float)image.width / (float)scaledWidth;
    float scaleY = (float)image.height / (float)scaledHeight;
    int cellX = cell % 8;
    int cellY = cell / 8;
    int borderX = (gameArea.gridSize - scaledWidth) / 2;
    int borderY = (gameArea.gridSize - scaledHeight) / 2;
    uint8_t *frameBufferPointer = gameArea.buffer;
    frameBufferPointer += cellX * gameArea.gridSize * 4;
    frameBufferPointer += borderX * 4;
    frameBufferPointer += cellY * gameArea.gridSize * framebuffer.width * 4;
    frameBufferPointer += borderY * framebuffer.width * 4;
    int yAdvance = (framebuffer.width * 4) - (scaledWidth * 4);
    for (int y = 0; y < scaledHeight; y++)
    {
        int imageY = ((float)y * scaleY) + 0.5f;
        uint8_t *imageRow = image.data + (imageY * image.width * 4);
        for (int x = 0; x < scaledWidth; x++)
        {
            int imageX = ((float)x * scaleX) + 0.5f;
            uint8_t *imagePointer = imageRow + (imageX * 4);
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
            scaleImage(image, i);
        }
    }
}

static void drawGrid(uint8_t *highlighted, int numHightlighted)
{
    GameArea gameArea = getGameArea();
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
    for (int y = 0; y < gameArea.size; y++)
    {
        if (y % gameArea.gridSize == 0)
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
        uint8_t *writePointer = gameArea.buffer + (y * framebuffer.width * 4);
        for (int x = 0; x < gameArea.size; x++)
        {
            if (x % gameArea.gridSize == 0)
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
            memcpy(writePointer, gridColor, 4);
            writePointer += 4;
        }
    }
    int yAdvance = (framebuffer.width * 4) - (gameArea.gridSize * 4);
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
        uint8_t *writePointer = gameArea.buffer + (x * gameArea.gridSize * 4) + (y * gameArea.gridSize * framebuffer.width * 4);
        for (int y = 0; y < gameArea.gridSize; y++)
        {
            for (int x = 0; x < gameArea.gridSize; x++)
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
    memset(framebuffer.data, 0, framebuffer.width * framebuffer.height * 4);
    drawGrid(highlighted, numHighlighted);
    drawPieces();
    blitToScreen();
}
