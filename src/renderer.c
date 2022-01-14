#include "renderer.h"
#include "game.h"
#include "platform.h"

#include <string.h>

Image framebuffer;

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

Glyph glyphs[94];
FontMetrics fontMetrics;

uint8_t highlighted[64];
int numHightlighted;
char *renderString;

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

static int getStringWidth(const char *str)
{
    char c = *str;
    int width = 0;
    while (c != 0)
    {
        if (c == ' ')
        {
            width += 10;
        }
        else if (c >= 33 && c <= 126)
        {
            Glyph *glyph = &glyphs[c - 33];
            width += glyph->advance;
            char next = str[1];
            if (next >= 33 && next <= 126)
            {
                width += glyph->kerning[next - 33];
            }
        }
        str++;
        c = *str;
    }
    return width;
}

static void drawRectangle(int x, int y, int width, int height)
{
    float color[3];
    color[0] = 41;
    color[1] = 46;
    color[2] = 42;
    float alpha = 0.95f;
    float inverseAlpha = 1.0f - alpha;
    GameArea gameArea = getGameArea();
    if (x >= gameArea.size)
    {
        return;
    }
    if (y >= gameArea.size)
    {
        return;
    }
    if (x < 0)
    {
        x = 0;
    }
    if (y < 0)
    {
        y = 0;
    }
    if (x + width > gameArea.size)
    {
        width = gameArea.size - x;
    }
    if (y + height > gameArea.size)
    {
        height = gameArea.size - y;
    }
    uint8_t *writePointer = gameArea.buffer + (x * 4) + (y * framebuffer.width * 4);
    int yAdvance = (framebuffer.width - width) * 4;
    for (int h = 0; h < height; h++)
    {
        for (int w = 0; w < width; w++)
        {
            for (int i = 0; i < 3; i++)
            {
                *writePointer = (color[i] * alpha) + (*writePointer * inverseAlpha);
                writePointer++;
            }
            writePointer++;
        }
        writePointer += yAdvance;
    }
}

static void drawGlyph(Glyph *glyph, int x, int y)
{
    GameArea gameArea = getGameArea();
    int glyphWidth = glyph->width;
    int glyphHeight = glyph->height;
    x += glyph->xOffset;
    y += glyph->yOffset;
    if (x < 0 || y < 0 || x + glyphWidth > gameArea.size || y + glyphHeight > gameArea.size)
    {
        return;
    }
    float fontColor[3];
    fontColor[0] = 13;
    fontColor[1] = 13;
    fontColor[2] = 209;
    uint8_t *writePointer = gameArea.buffer + (x * 4) + (y * framebuffer.width * 4);
    uint8_t *readPointer = glyph->data;
    int yAdvance = (framebuffer.width - glyphWidth) * 4;
    for (int h = 0; h < glyphHeight; h++)
    {
        for (int w = 0; w < glyphWidth; w++)
        {
            float alpha = *readPointer / 255.0f;
            float inverseAlpha = 1.0f - alpha;
            for (int i = 0; i < 3; i++)
            {
                *writePointer = (fontColor[i] * alpha) + (*writePointer * inverseAlpha);
                writePointer++;
            }
            writePointer++;
            readPointer++;
        }
        writePointer += yAdvance;
    }
}

static void drawString(const char *str, int x, int y)
{
    char c = *str;
    while (c != 0)
    {
        if (c == ' ')
        {
            x += 10;
        }
        else if (c >= 33 && c <= 126)
        {
            Glyph *glyph = &glyphs[c - 33];
            drawGlyph(glyph, x, y);
            x += glyph->advance;
            char next = str[1];
            if (next >= 33 && next <= 126)
            {
                x += glyph->kerning[next - 33];
            }
        }
        str++;
        c = *str;
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
        float yFloatPixel = y * scaleY;
        int imageY0 = yFloatPixel;
        int imageY1;
        if (imageY0 >= image.height - 1)
        {
            imageY0 = image.height - 1;
            imageY1 = image.height - 1;
        }
        else
        {
            imageY1 = imageY0 + 1;
        }
        float y1Interp = yFloatPixel - imageY0;
        float y0Interp = 1.0f - y1Interp;
        uint8_t *row0 = image.data + (imageY0 * image.width * 4);
        uint8_t *row1 = image.data + (imageY1 * image.width * 4);
        for (int x = 0; x < scaledWidth; x++)
        {
            float xFloatPixel = x * scaleX;
            int imageX0 = xFloatPixel;
            int imageX1;
            if (imageX0 >= image.width - 1)
            {
                imageX0 = image.width - 1;
                imageX1 = image.width - 1;
            }
            else
            {
                imageX1 = imageX0 + 1;
            }
            float x1Interp = xFloatPixel - imageX0;
            float x0Interp = 1.0f - x1Interp;
            uint8_t *x0y0 = row0 + (imageX0 * 4);
            uint8_t *x1y0 = row0 + (imageX1 * 4);
            uint8_t *x0y1 = row1 + (imageX0 * 4);
            uint8_t *x1y1 = row1 + (imageX1 * 4);
            float row0Interp[4];
            float row1Interp[4];
            float finalInterp[4];
            for (int i = 0; i < 4; i++)
            {
                row0Interp[i] = (x0y0[i] * x0Interp) + (x1y0[i] * x1Interp);
            }
            for (int i = 0; i < 4; i++)
            {
                row1Interp[i] = (x0y1[i] * x0Interp) + (x1y1[i] * x1Interp);
            }
            for (int i = 0; i < 4; i++)
            {
                finalInterp[i] = (row0Interp[i] * y0Interp) + (row1Interp[i] * y1Interp);
            }
            float alpha = finalInterp[3] / 255.0f;
            float inverseAlpha = 1.0f - alpha;
            for (int i = 0; i < 3; i++)
            {
                frameBufferPointer[i] = (finalInterp[i] * alpha) + (frameBufferPointer[i] * inverseAlpha);
            }
            frameBufferPointer += 4;
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

static void drawGrid(void)
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

static void gameOverBox(const char *str)
{
    GameArea gameArea = getGameArea();
    int stringWidth = getStringWidth(str);
    int stringHeight = fontMetrics.ascent - fontMetrics.descent;
    int stringX = (gameArea.size / 2) - (stringWidth / 2);
    int stringY = ((gameArea.size / 2) + fontMetrics.ascent) - (stringHeight / 2);
    drawRectangle(stringX, stringY - fontMetrics.ascent, stringWidth, stringHeight);
    drawString(str, stringX, stringY);
}

void renderFrame(void)
{
    memset(framebuffer.data, 0, framebuffer.width * framebuffer.height * 4);
    drawGrid();
    drawPieces();
    if (renderString != NULL)
    {
        gameOverBox(renderString);
    }
    blitToScreen();
}
