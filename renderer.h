#ifndef RENDERER_H
#define RENDERER_H

#include <stdint.h>

typedef struct Image
{
    int width;
    int height;
    uint8_t *data;
} Image;

#define FRAMEBUFFER_WIDTH 720
#define FRAMEBUFFER_HEIGHT 720
#define GRID_SIZE (FRAMEBUFFER_WIDTH / 8)

extern uint8_t *frameBuffer;

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

void renderFrame(uint8_t *highlighted, int numHighlighted);

#endif
