#include "assets.h"
#include "renderer.h"
#include "platform.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void loadBmp(const char *fileName, Image *image)
{
    uint8_t *bmpData = loadFile(fileName);
    if (bmpData == NULL)
    {
        return;
    }
    uint32_t startingPixelIndex = *(uint32_t *)&bmpData[10];
    int32_t width = *(int32_t *)&bmpData[18];
    int32_t height = *(int32_t *)&bmpData[22];
    uint32_t pixelDataSize = *(uint32_t *)&bmpData[34];
    uint32_t expectedDataSize = width * height * 4;
    if (expectedDataSize != pixelDataSize)
    {
        char message[LOG_SIZE];
        snprintf(message, LOG_SIZE, "%s: Expected %u bytes. Header reads %u", fileName, expectedDataSize, pixelDataSize);
        debugLog(message);
        free(bmpData);
        return;
    }
    uint8_t *pixelData = malloc(pixelDataSize);
    if (pixelData == NULL)
    {
        debugLog("malloc failed");
        free(bmpData);
        return;
    }
    // BMP images have origin at bottom left.
    // Move data so that origin is top left.
    uint8_t *readPointer = bmpData + startingPixelIndex;
    uint8_t *writePointer = (pixelData + pixelDataSize) - (width * 4);
    for (int i = 0; i < height; i++)
    {
        memcpy(writePointer, readPointer, width * 4);
        readPointer += (width * 4);
        writePointer -= (width * 4);
    }
    image->width = width;
    image->height = height;
    image->data = pixelData;
    free(bmpData);
}

void loadImages(void)
{
    loadBmp("assets/images/black-bishop.bmp", &blackBishop);
    loadBmp("assets/images/black-king.bmp", &blackKing);
    loadBmp("assets/images/black-knight.bmp", &blackKnight);
    loadBmp("assets/images/black-pawn.bmp", &blackPawn);
    loadBmp("assets/images/black-queen.bmp", &blackQueen);
    loadBmp("assets/images/black-rook.bmp", &blackRook);
    loadBmp("assets/images/white-bishop.bmp", &whiteBishop);
    loadBmp("assets/images/white-king.bmp", &whiteKing);
    loadBmp("assets/images/white-knight.bmp", &whiteKnight);
    loadBmp("assets/images/white-pawn.bmp", &whitePawn);
    loadBmp("assets/images/white-queen.bmp", &whiteQueen);
    loadBmp("assets/images/white-rook.bmp", &whiteRook);
}
