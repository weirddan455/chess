#ifndef GAME_H
#define GAME_H

#include <stdint.h>

#define MOVE_MASK 63
#define ENPASSANT_MASK 64
#define CASTLE_MASK 128

enum PieceOwner
{
    BLACK,
    WHITE
};

enum PieceType
{
    BISHOP,
    KING,
    KNIGHT,
    PAWN,
    QUEEN,
    ROOK
};

typedef struct Piece
{
    enum PieceOwner owner;
    enum PieceType type;
    int previousPosition;
    int lastMoved;
} Piece;

typedef struct GameState
{
    int turn;
    Piece pieces[32];
    Piece *board[64];
} GameState;

extern GameState gameState;

void movePiece(uint8_t moveTo, uint8_t moveFrom, GameState *state);
int pieceLegalMoves(uint8_t cell, uint8_t *moves);
void initGameState(void);
int getAllLegalMoves(enum PieceOwner owner, uint8_t *moveTo, uint8_t *moveFrom);

#endif
