#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stdint.h>

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

// Used for move arrays
#define MOVE_TO_MASK 63
#define MOVE_FROM_MASK 4032
#define MOVE_FROM_SHIFT 6
#define PAWN_PROMOTE_MASK 28672
#define PAWN_PROMOTE_SHIFT 12
#define PAWN_PROMOTE_QUEEN (QUEEN << PAWN_PROMOTE_SHIFT)
#define PAWN_PROMOTE_BISHOP (BISHOP << PAWN_PROMOTE_SHIFT)
#define PAWN_PROMOTE_ROOK (ROOK << PAWN_PROMOTE_SHIFT)
#define PAWN_PROMOTE_KNIGHT (KNIGHT << PAWN_PROMOTE_SHIFT)
#define CASTLE_ENPASSANT_FLAG 32768

// Castle availablity flags
#define CASTLE_WHITE_KING 1
#define CASTLE_WHITE_QUEEN 2
#define CASTLE_BLACK_KING 4
#define CASTLE_BLACK_QUEEN 8

typedef struct GameState
{
    int halfMoves; // Resets when a pawn is moved or a piece is captured.  Used for 50 move draw rule.
    uint8_t playerToMove;
    uint8_t enPassantSquare;
    uint8_t castlingAvailablity;
    uint8_t board[64];
} GameState;

enum GameEnd
{
    GAME_NOT_OVER, CHECKMATE, STALEMATE, DRAW_50_MOVE
};

extern GameState gameState;

void movePiece(uint16_t move, GameState *state);
int pieceLegalMoves(uint8_t cell, uint16_t *moves, GameState *state);
void initGameState(void);
int getAllLegalMoves(uint8_t owner, uint16_t *moves, GameState *state);
bool playerInCheck(uint8_t player);
uint64_t calculatePositions(int depth, GameState *state, uint8_t player);
void loadFenString(const char *str);
uint16_t getComputerMove(uint8_t player);
enum GameEnd checkGameEnd(GameState *state, uint8_t player);

#endif
