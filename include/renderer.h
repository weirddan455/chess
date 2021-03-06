#ifndef RENDERER_H
#define RENDERER_H

#include <stdbool.h>
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

typedef struct Glyph
{
    int width;
    int height;
    int xOffset;
    int yOffset;
    int advance;
    int kerning[94];
    uint8_t *data;
} Glyph;

typedef struct FontMetrics
{
    int ascent;
    int descent;
    int lineGap;
} FontMetrics;

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

extern Glyph glyphs[94];
extern FontMetrics fontMetrics;

extern uint8_t highlighted[64];
extern int numHightlighted;
extern const char *gameOverString;
extern uint16_t pawnPromoteMove;

GameArea getGameArea(void);
void renderFrame(void);
bool handleGameOver(void);

#endif
