#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stdint.h>

// Special move flags
#define MOVE_MASK 63
#define ENPASSANT_MASK 64
#define CASTLE_MASK 128

// Castle availablity flags
#define CASTLE_WHITE_KING 1
#define CASTLE_WHITE_QUEEN 2
#define CASTLE_BLACK_KING 4
#define CASTLE_BLACK_QUEEN 8

// Piece flags
#define PIECE_TYPE_MASK 7
#define PIECE_OWNER_MASK 24

#define PAWN 1
#define BISHOP 2
#define KNIGHT 3
#define ROOK 4
#define QUEEN 5
#define KING 6

#define BLACK 16
#define WHITE 8

typedef struct GameState
{
    int halfMoves; // Resets when a pawn is moved or a piece is captured.  Used for 50 move draw rule.
    uint8_t playerToMove;
    uint8_t enPassantSquare;
    uint8_t castlingAvailablity;
    uint8_t board[64];
} GameState;

extern GameState gameState;

void movePiece(uint8_t moveTo, uint8_t moveFrom, GameState *state);
int pieceLegalMoves(uint8_t cell, uint8_t *moves, GameState *state);
void initGameState(void);
int getAllLegalMoves(uint8_t owner, uint8_t *moveTo, uint8_t *moveFrom, GameState *state);
bool playerInCheck(uint8_t player);
uint64_t calculatePositions(int depth, GameState *state, uint8_t player);
void loadFenString(const char *str);

#endif
