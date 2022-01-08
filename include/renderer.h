#ifndef RENDERER_H
#define RENDERER_H

#include <stdint.h>

typedef struct Image
{
    int width;
    int height;
    uint8_t *data;
} Image;

typedef struct GameArea
{
    int size;
    int gridSize;
    int x;
    int y;
    uint8_t *buffer;
} GameArea;

extern Image framebuffer;

extern Image blackBishop;
extern Image blackKing;
extern Image blackKnight;
extern Image blackPawn;
extern Image blackQueen;
extern Image blackRook;
extern Image whiteBishop;
extern Image whiteKing;
extern Image whiteKnight;
extern Image whitePawn;
extern Image whiteQueen;
extern Image whiteRook;
extern Image glyphTest;

GameArea getGameArea(void);
void renderFrame(uint8_t *highlighted, int numHighlighted);

#endif
