#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "game.h"
#include "pcgrandom.h"
#include "platform.h"

#define CHECKMATE_EVALUATION -9001
#define STALEMATE_EVALUATION 0

GameState gameState;
static Zobrist zobrist;
static Position positionTable[1024];

static int zobristPieceLookup(int cell, uint8_t piece)
{
    if (piece == 0)
    {
        debugLog("Called zobrist piece lookup on empty cell (fix the bug!)");
        return 0;
    }
    int pieceOffset = (piece & PIECE_TYPE_MASK) - 1;
    if ((piece & PIECE_OWNER_MASK) == BLACK)
    {
        pieceOffset += 6;
    }
    return (cell * 12) + pieceOffset;
}

static bool comparePosition(GameState *state, Position *position)
{
    if (state->playerToMove != position->playerToMove)
    {
        return false;
    }
    if (state->enPassantSquare != position->enPassantSquare)
    {
        return false;
    }
    if (state->castlingAvailablity != position->castlingAvailability)
    {
        return false;
    }
    for (int i = 0; i < 64; i++)
    {
        if (state->board[i] != position->board[i])
        {
            return false;
        }
    }
    return true;
}

static int getPositionOccurences(GameState *state)
{
    uint64_t startingLookup = state->hash & 1023;
    uint64_t lookup = startingLookup;
    while (positionTable[lookup].occurences > 0)
    {
        if (positionTable[lookup].hash == state->hash)
        {
            if (comparePosition(state, &positionTable[lookup]))
            {
                return positionTable[lookup].occurences;
            }
            debugLog("Zobrist hash collision (should be very rare)");
        }
        lookup = (lookup + 1) & 1023;
        if (lookup == startingLookup)
        {
            debugLog("Position table is full");
            return 0;
        }
    }
    return 0;
}

static void addPosition(GameState *state)
{
    uint64_t startingLookup = state->hash & 1023;
    uint64_t lookup = startingLookup;
    while (positionTable[lookup].occurences > 0)
    {
        if (positionTable[lookup].hash == state->hash)
        {
            if (comparePosition(state, &positionTable[lookup]))
            {
                positionTable[lookup].occurences++;
                return;
            }
            debugLog("Zobrist hash collision (should be very rare)");
        }
        lookup = (lookup + 1) & 1023;
        if (lookup == startingLookup)
        {
            debugLog("Position table is full");
            return;
        }
    }
    positionTable[lookup].hash = state->hash;
    positionTable[lookup].occurences = 1;
    positionTable[lookup].playerToMove = state->playerToMove;
    positionTable[lookup].enPassantSquare = state->enPassantSquare;
    positionTable[lookup].castlingAvailability = state->castlingAvailablity;
    for (int i = 0; i < 64; i++)
    {
        positionTable[lookup].board[i] = state->board[i];
    }
}

static void hashStartingPosition(void)
{
    gameState.hash = 0;
    for (int i = 0; i < 64; i++)
    {
        if (gameState.board[i] != 0)
        {
            gameState.hash ^= zobrist.pieces[zobristPieceLookup(i, gameState.board[i])];
        }
    }
    if (gameState.castlingAvailablity & CASTLE_BLACK_QUEEN)
    {
        gameState.hash ^= zobrist.blackQueenCastle;
    }
    if (gameState.castlingAvailablity & CASTLE_WHITE_QUEEN)
    {
        gameState.hash ^= zobrist.whiteQueenCastle;
    }
    if (gameState.castlingAvailablity & CASTLE_BLACK_KING)
    {
        gameState.hash ^= zobrist.blackKingCastle;
    }
    if (gameState.castlingAvailablity & CASTLE_WHITE_KING)
    {
        gameState.hash ^= zobrist.whiteKingCastle;
    }
    if (gameState.enPassantSquare != 255)
    {
        gameState.hash ^= zobrist.enPassantFile[gameState.enPassantSquare % 8];
    }
    memset(positionTable, 0, 1024 * sizeof(Position));
    addPosition(&gameState);
}

void movePiece(uint16_t move, GameState *state)
{
    uint8_t moveTo = move & MOVE_TO_MASK;
    uint8_t moveFrom = (move & MOVE_FROM_MASK) >> MOVE_FROM_SHIFT;
    uint8_t piece = state->board[moveFrom];
    uint8_t pieceOwner = piece & PIECE_OWNER_MASK;
    uint8_t pieceType = piece & PIECE_TYPE_MASK;
    uint8_t capturedPiece = state->board[moveTo];
    uint8_t prevCastling = state->castlingAvailablity;
    state->hash ^= zobrist.pieces[zobristPieceLookup(moveFrom, piece)];
    if (capturedPiece != 0)
    {
        state->hash ^= zobrist.pieces[zobristPieceLookup(moveTo, capturedPiece)];
    }
    if (capturedPiece != 0 || pieceType == PAWN)
    {
        state->halfMoves = 0;
    }
    else
    {
        state->halfMoves++;
    }
    if (state->enPassantSquare != 255)
    {
        state->hash ^= zobrist.enPassantFile[state->enPassantSquare % 8];
    }
    state->enPassantSquare = 255;
    if (pieceType == PAWN)
    {
        if (pieceOwner == BLACK)
        {
            if (moveTo - moveFrom == 16)
            {
                state->enPassantSquare = moveTo - 8;
            }
            else if (move & CASTLE_ENPASSANT_FLAG)
            {
                state->hash ^= zobrist.pieces[zobristPieceLookup(moveTo - 8, state->board[moveTo - 8])];
                state->board[moveTo - 8] = 0;
            }
        }
        else
        {
            if (moveFrom - moveTo == 16)
            {
                state->enPassantSquare = moveTo + 8;
            }
            else if (move & CASTLE_ENPASSANT_FLAG)
            {
                state->hash ^= zobrist.pieces[zobristPieceLookup(moveTo + 8, state->board[moveTo + 8])];
                state->board[moveTo + 8] = 0;
            }
        }
        uint16_t promotion = move & PAWN_PROMOTE_MASK;
        if (promotion)
        {
            piece = pieceOwner | (promotion >> PAWN_PROMOTE_SHIFT);
        }
    }
    else if (pieceType == KING)
    {
        if (pieceOwner == WHITE)
        {
            state->castlingAvailablity &= ~(CASTLE_WHITE_KING | CASTLE_WHITE_QUEEN);
        }
        else
        {
            state->castlingAvailablity &= ~(CASTLE_BLACK_KING | CASTLE_BLACK_QUEEN);
        }
        if (move & CASTLE_ENPASSANT_FLAG)
        {
            if (moveTo > moveFrom)
            {
                state->hash ^= zobrist.pieces[zobristPieceLookup(moveTo + 1, state->board[moveTo + 1])];
                state->hash ^= zobrist.pieces[zobristPieceLookup(moveTo - 1, state->board[moveTo + 1])];
                state->board[moveTo - 1] = state->board[moveTo + 1];
                state->board[moveTo + 1] = 0;
            }
            else
            {
                state->hash ^= zobrist.pieces[zobristPieceLookup(moveTo - 2, state->board[moveTo - 2])];
                state->hash ^= zobrist.pieces[zobristPieceLookup(moveTo + 1, state->board[moveTo - 2])];
                state->board[moveTo + 1] = state->board[moveTo - 2];
                state->board[moveTo - 2] = 0;
            }
        }
    }
    if (moveTo == 0 || moveFrom == 0)
    {
        state->castlingAvailablity &= ~CASTLE_BLACK_QUEEN;
    }
    if (moveTo == 7 || moveFrom == 7)
    {
        state->castlingAvailablity &= ~CASTLE_BLACK_KING;
    }
    if (moveTo == 56 || moveFrom == 56)
    {
        state->castlingAvailablity &= ~CASTLE_WHITE_QUEEN;
    }
    if (moveTo == 63 || moveFrom == 63)
    {
        state->castlingAvailablity &= ~CASTLE_WHITE_KING;
    }
    state->board[moveTo] = piece;
    state->board[moveFrom] = 0;
    if (state->playerToMove == WHITE)
    {
        state->playerToMove = BLACK;
    }
    else
    {
        state->playerToMove = WHITE;
    }
    uint8_t newCastling = state->castlingAvailablity;
    if (prevCastling != newCastling)
    {
        if ((prevCastling & CASTLE_BLACK_QUEEN) != (newCastling & CASTLE_BLACK_QUEEN))
        {
            state->hash ^= zobrist.blackQueenCastle;
        }
        if ((prevCastling & CASTLE_WHITE_QUEEN) != (newCastling & CASTLE_WHITE_QUEEN))
        {
            state->hash ^= zobrist.whiteQueenCastle;
        }
        if ((prevCastling & CASTLE_BLACK_KING) != (newCastling & CASTLE_BLACK_KING))
        {
            state->hash ^= zobrist.blackKingCastle;
        }
        if ((prevCastling & CASTLE_WHITE_KING) != (newCastling & CASTLE_WHITE_KING))
        {
            state->hash ^= zobrist.whiteKingCastle;
        }
    }
    if (state->enPassantSquare != 255)
    {
        state->hash ^= zobrist.enPassantFile[state->enPassantSquare % 8];
    }
    state->hash ^= zobrist.playerToMove;
    state->hash ^= zobrist.pieces[zobristPieceLookup(moveTo, piece)];
    if (state == &gameState)
    {
        addPosition(state);
    }
}

static uint8_t getKingLocation(uint8_t owner, GameState *state)
{
    uint8_t king = owner | KING;
    for (int i = 0; i < 64; i++)
    {
        if (state->board[i] == king)
        {
            return i;
        }
    }
    debugLog("wtf couldn't find king");
    return 0;
}

static int pawnPossibleMoves(uint8_t cell, uint16_t *moves, GameState *state)
{
    uint8_t pawnOwner = state->board[cell] & PIECE_OWNER_MASK;
    uint16_t moveFrom = (uint16_t)cell << MOVE_FROM_SHIFT;
    int numMoves = 0;
    int direction;
    int startingRow;
    if (pawnOwner == BLACK)
    {
        direction = 8;
        startingRow = 8;
    }
    else
    {
        direction = -8;
        startingRow = 48;
    }
    uint16_t move = cell + direction;
    if (move < 64 && state->board[move] == 0)
    {
        if ((pawnOwner == BLACK && move >= 56) || (pawnOwner == WHITE && move <= 7))
        {
            moves[numMoves++] = move | moveFrom | PAWN_PROMOTE_BISHOP;
            moves[numMoves++] = move | moveFrom | PAWN_PROMOTE_QUEEN;
            moves[numMoves++] = move | moveFrom | PAWN_PROMOTE_KNIGHT;
            moves[numMoves++] = move | moveFrom | PAWN_PROMOTE_ROOK;
        }
        else
        {
            moves[numMoves++] = move | moveFrom;
        }
        if (cell >= startingRow && cell < startingRow + 8)
        {
            move += direction;
            if (move < 64 && state->board[move] == 0)
            {
                if ((pawnOwner == BLACK && move >= 56) || (pawnOwner == WHITE && move <= 7))
                {
                    moves[numMoves++] = move | moveFrom | PAWN_PROMOTE_BISHOP;
                    moves[numMoves++] = move | moveFrom | PAWN_PROMOTE_QUEEN;
                    moves[numMoves++] = move | moveFrom | PAWN_PROMOTE_KNIGHT;
                    moves[numMoves++] = move | moveFrom | PAWN_PROMOTE_ROOK;
                }
                else
                {
                    moves[numMoves++] = move | moveFrom;
                }
            }
        }
    }
    uint8_t col = cell % 8;
    uint16_t captureMoves[2];
    int numCaptureMoves = 0;
    if (pawnOwner == BLACK)
    {
        if (col > 0)
        {
            captureMoves[numCaptureMoves++] = cell + 7;
        }
        if (col < 7)
        {
            captureMoves[numCaptureMoves++] = cell + 9;
        }
    }
    else
    {
        if (col > 0)
        {
            captureMoves[numCaptureMoves++] = cell - 9;
        }
        if (col < 7)
        {
            captureMoves[numCaptureMoves++] = cell - 7;
        }
    }
    for (int i = 0; i < numCaptureMoves; i++)
    {
        if (captureMoves[i] < 64)
        {
            if (state->board[captureMoves[i]] != 0 && (state->board[captureMoves[i]] & PIECE_OWNER_MASK) != pawnOwner)
            {
                if ((pawnOwner == BLACK && move >= 56) || (pawnOwner == WHITE && move <= 7))
                {
                    moves[numMoves++] = captureMoves[i] | moveFrom | PAWN_PROMOTE_BISHOP;
                    moves[numMoves++] = captureMoves[i] | moveFrom | PAWN_PROMOTE_QUEEN;
                    moves[numMoves++] = captureMoves[i] | moveFrom | PAWN_PROMOTE_KNIGHT;
                    moves[numMoves++] = captureMoves[i] | moveFrom | PAWN_PROMOTE_ROOK;
                }
                else
                {
                    moves[numMoves++] = captureMoves[i] | moveFrom;
                }
            }
            // Check for en passant
            else if (captureMoves[i] == state->enPassantSquare)
            {
                moves[numMoves++] = captureMoves[i] | moveFrom | CASTLE_ENPASSANT_FLAG;
            }
        }
    }
    return numMoves;
}

static int knightPossibleMoves(uint8_t cell, uint16_t *moves, GameState *state)
{
    uint8_t knightOwner = state->board[cell] & PIECE_OWNER_MASK;
    uint16_t moveFrom = (uint16_t)cell << MOVE_FROM_SHIFT;
    uint8_t col = cell % 8;
    uint16_t possibleMoves[8];
    int numPossibleMoves = 0;
    if (col < 7)
    {
        possibleMoves[numPossibleMoves++] = cell + 17;
        possibleMoves[numPossibleMoves++] = cell - 15;
        if (col < 6)
        {
            possibleMoves[numPossibleMoves++] = cell + 10;
            possibleMoves[numPossibleMoves++] = cell - 6;
        }
    }
    if (col > 0)
    {
        possibleMoves[numPossibleMoves++] = cell + 15;
        possibleMoves[numPossibleMoves++] = cell - 17;
        if (col > 1)
        {
            possibleMoves[numPossibleMoves++] = cell + 6;
            possibleMoves[numPossibleMoves++] = cell - 10;
        }
    }
    int numMoves = 0;
    for (int i = 0; i < numPossibleMoves; i++)
    {
        if (possibleMoves[i] < 64 && (state->board[possibleMoves[i]] & PIECE_OWNER_MASK) != knightOwner)
        {
            moves[numMoves++] = possibleMoves[i] | moveFrom;
        }
    }
    return numMoves;
}

static int bishopPossibleMoves(uint8_t cell, uint16_t *moves, GameState *state)
{
    uint8_t bishopOwner = state->board[cell] & PIECE_OWNER_MASK;
    uint16_t moveFrom = (uint16_t)cell << MOVE_FROM_SHIFT;
    uint8_t col = cell % 8;
    uint8_t row = cell / 8;
    uint8_t x = col;
    uint8_t y = row;
    int numMoves = 0;
    // Up left
    uint16_t move = cell - 9;
    while (x > 0 && y > 0)
    {
        if (state->board[move] != 0)
        {
            if ((state->board[move] & PIECE_OWNER_MASK) != bishopOwner)
            {
                moves[numMoves++] = move | moveFrom;
            }
            break;
        }
        moves[numMoves++] = move | moveFrom;
        move -= 9;
        x--;
        y--;
    }
    x = col;
    y = row;
    // Down left
    move = cell + 7;
    while (x > 0 && y < 7)
    {
        if (state->board[move] != 0)
        {
            if ((state->board[move] & PIECE_OWNER_MASK) != bishopOwner)
            {
                moves[numMoves++] = move | moveFrom;
            }
            break;
        }
        moves[numMoves++] = move | moveFrom;
        move += 7;
        x--;
        y++;
    }
    x = col;
    y = row;
    // Up right
    move = cell - 7;
    while (x < 7 && y > 0)
    {
        if (state->board[move] != 0)
        {
            if ((state->board[move] & PIECE_OWNER_MASK) != bishopOwner)
            {
                moves[numMoves++] = move | moveFrom;
            }
            break;
        }
        moves[numMoves++] = move | moveFrom;
        move -= 7;
        x++;
        y--;
    }
    x = col;
    y = row;
    // Down right
    move = cell + 9;
    while (x < 7 && y < 7)
    {
        if (state->board[move] != 0)
        {
            if ((state->board[move] & PIECE_OWNER_MASK) != bishopOwner)
            {
                moves[numMoves++] = move | moveFrom;
            }
            break;
        }
        moves[numMoves++] = move | moveFrom;
        move += 9;
        x++;
        y++;
    }
    return numMoves;
}

static int rookPossibleMoves(uint8_t cell, uint16_t *moves, GameState *state)
{
    uint8_t rookOwner = state->board[cell] & PIECE_OWNER_MASK;
    uint16_t moveFrom = (uint16_t)cell << MOVE_FROM_SHIFT;
    uint8_t col = cell % 8;
    uint8_t row = cell / 8;
    int numMoves = 0;
    uint8_t x = col;
    uint16_t move = cell - 1;
    while (x > 0)
    {
        if (state->board[move] != 0)
        {
            if ((state->board[move] & PIECE_OWNER_MASK) != rookOwner)
            {
                moves[numMoves++] = move | moveFrom;
            }
            break;
        }
        moves[numMoves++] = move | moveFrom;
        move--;
        x--;
    }
    x = col;
    move = cell + 1;
    while (x < 7)
    {
        if (state->board[move] != 0)
        {
            if ((state->board[move] & PIECE_OWNER_MASK) != rookOwner)
            {
                moves[numMoves++] = move | moveFrom;
            }
            break;
        }
        moves[numMoves++] = move | moveFrom;
        move++;
        x++;
    }
    uint8_t y = row;
    move = cell - 8;
    while (y > 0)
    {
        if (state->board[move] != 0)
        {
            if ((state->board[move] & PIECE_OWNER_MASK) != rookOwner)
            {
                moves[numMoves++] = move | moveFrom;
            }
            break;
        }
        moves[numMoves++] = move | moveFrom;
        move -= 8;
        y--;
    }
    y = row;
    move = cell + 8;
    while (y < 7)
    {
        if (state->board[move] != 0)
        {
            if ((state->board[move] & PIECE_OWNER_MASK) != rookOwner)
            {
                moves[numMoves++] = move | moveFrom;
            }
            break;
        }
        moves[numMoves++] = move | moveFrom;
        move += 8;
        y++;
    }
    return numMoves;
}

static int queenPossibleMoves(uint8_t cell, uint16_t *moves, GameState *state)
{
    int numMoves = bishopPossibleMoves(cell, moves, state);
    return numMoves + rookPossibleMoves(cell, moves + numMoves, state);
}

static int getCastlingMoves(uint8_t kingOwner, uint16_t moveFrom, uint16_t *moves, GameState *state)
{
    uint16_t backRow;
    bool queenSide;
    bool kingSide;
    if (kingOwner == BLACK)
    {
        backRow = 0;
        if (state->castlingAvailablity & CASTLE_BLACK_QUEEN)
        {
            queenSide = true;
        }
        else
        {
            queenSide = false;
        }
        if (state->castlingAvailablity & CASTLE_BLACK_KING)
        {
            kingSide = true;
        }
        else
        {
            kingSide = false;
        }
    }
    else
    {
        backRow = 56;
        if (state->castlingAvailablity & CASTLE_WHITE_QUEEN)
        {
            queenSide = true;
        }
        else
        {
            queenSide = false;
        }
        if (state->castlingAvailablity & CASTLE_WHITE_KING)
        {
            kingSide = true;
        }
        else
        {
            kingSide = false;
        }
    }
    if (queenSide)
    {
        for (int i = 1; i < 4; i++)
        {
            if (state->board[backRow + i] != 0)
            {
                queenSide = false;
                break;
            }
        }
    }
    if (kingSide)
    {
        for (int i = 5; i < 7; i++)
        {
            if (state->board[backRow + i] != 0)
            {
                kingSide = false;
                break;
            }
        }
    }
    int numMoves = 0;
    if (queenSide)
    {
        moves[numMoves++] = (backRow + 2) | moveFrom | CASTLE_ENPASSANT_FLAG;
    }
    if (kingSide)
    {
        moves[numMoves++] = (backRow + 6) | moveFrom | CASTLE_ENPASSANT_FLAG;
    }
    return numMoves;
}

static int kingPossibleMoves(uint8_t cell, uint16_t *moves, GameState *state)
{
    uint8_t kingOwner = state->board[cell] & PIECE_OWNER_MASK;
    uint16_t moveFrom = (uint16_t)cell << MOVE_FROM_SHIFT;
    uint8_t col = cell % 8;
    uint8_t row = cell / 8;
    int numMoves = 0;
    uint16_t move = cell - 1;
    if (col > 0 && (state->board[move] & PIECE_OWNER_MASK) != kingOwner)
    {
        moves[numMoves++] = move | moveFrom;
    }
    move = cell + 1;
    if (col < 7 && (state->board[move] & PIECE_OWNER_MASK) != kingOwner)
    {
        moves[numMoves++] = move | moveFrom;
    }
    move = cell - 8;
    if (row > 0 && (state->board[move] & PIECE_OWNER_MASK) != kingOwner)
    {
        moves[numMoves++] = move | moveFrom;
    }
    move = cell + 8;
    if (row < 7 && (state->board[move] & PIECE_OWNER_MASK) != kingOwner)
    {
        moves[numMoves++] = move | moveFrom;
    }
    move = cell - 9;
    if (col > 0 && row > 0 && (state->board[move] & PIECE_OWNER_MASK) != kingOwner)
    {
        moves[numMoves++] = move | moveFrom;
    }
    move = cell + 7;
    if (col > 0 && row < 7 && (state->board[move] & PIECE_OWNER_MASK) != kingOwner)
    {
        moves[numMoves++] = move | moveFrom;
    }
    move = cell - 7;
    if (col < 7 && row > 0 && (state->board[move] & PIECE_OWNER_MASK) != kingOwner)
    {
        moves[numMoves++] = move | moveFrom;
    }
    move = cell + 9;
    if (col < 7 && row < 7 && (state->board[move] & PIECE_OWNER_MASK) != kingOwner)
    {
        moves[numMoves++] = move | moveFrom;
    }
    return numMoves + getCastlingMoves(kingOwner, moveFrom, moves + numMoves, state);
}

static int piecePossibleMoves(uint8_t cell, uint16_t *moves, GameState *state)
{
    switch(state->board[cell] & PIECE_TYPE_MASK)
    {
        case PAWN:
            return pawnPossibleMoves(cell, moves, state);
        case KNIGHT:
            return knightPossibleMoves(cell, moves, state);
        case BISHOP:
            return bishopPossibleMoves(cell, moves, state);
        case ROOK:
            return rookPossibleMoves(cell, moves, state);
        case QUEEN:
            return queenPossibleMoves(cell, moves, state);
        case KING:
            return kingPossibleMoves(cell, moves, state);
    }
    return 0;
}

int pieceLegalMoves(uint8_t cell, uint16_t *moves, GameState *state)
{
    uint8_t owner = state->board[cell] & PIECE_OWNER_MASK;
    uint8_t pieceType = state->board[cell] & PIECE_TYPE_MASK;
    uint8_t opponent;
    if (owner == BLACK)
    {
        opponent = WHITE;
    }
    else
    {
        opponent = BLACK;
    }
    uint16_t possibleMoves[64];
    int numPossibleMoves = piecePossibleMoves(cell, possibleMoves, state);
    int numLegalMoves = 0;
    for (int i = 0; i < numPossibleMoves; i++)
    {
        bool legalMove = true;
        if (pieceType == KING && possibleMoves[i] & CASTLE_ENPASSANT_FLAG)
        {
            uint8_t backRow = owner == BLACK ? 0 : 56;
            uint8_t king = backRow + 4;
            bool kingSide = (possibleMoves[i] & MOVE_TO_MASK) > cell;
            // Check for enemy pawns attacking castle path
            uint8_t pawnCheckSquare = owner == BLACK ? 8 : 48;
            if (kingSide)
            {
                pawnCheckSquare += 3;
                for (int j = 0; j < 5; j++)
                {
                    uint8_t pawnCheckPiece = state->board[pawnCheckSquare + j];
                    if (((pawnCheckPiece & PIECE_OWNER_MASK) == opponent) && (((pawnCheckPiece & PIECE_TYPE_MASK) == PAWN)))
                    {
                        legalMove = false;
                        break;
                    }
                }
            }
            else
            {
                pawnCheckSquare++;
                for (int j = 0; j < 5; j++)
                {
                    uint8_t pawnCheckPiece = state->board[pawnCheckSquare + j];
                    if (((pawnCheckPiece & PIECE_OWNER_MASK) == opponent) && (((pawnCheckPiece & PIECE_TYPE_MASK) == PAWN)))
                    {
                        legalMove = false;
                        break;
                    }
                }
            }
            if (legalMove)
            {
                for (int j = 0; j < 64; j++)
                {
                    if ((state->board[j] & PIECE_OWNER_MASK) == opponent)
                    {
                        uint16_t opponentMoves[64];
                        int numOpponentMoves = piecePossibleMoves(j, opponentMoves, state);
                        for (int k = 0; k < numOpponentMoves; k++)
                        {
                            uint8_t move = opponentMoves[k] & MOVE_TO_MASK;
                            if (move == king)
                            {
                                legalMove = false;
                                break;
                            }
                            if (kingSide)
                            {
                                if (move == backRow + 5 || move == backRow + 6)
                                {
                                    legalMove = false;
                                    break;
                                }
                            }
                            else
                            {
                                if (move == backRow + 2 || move == backRow + 3)
                                {
                                    legalMove = false;
                                    break;
                                }
                            }
                        }
                        if (!legalMove)
                        {
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            GameState copyState = *state;
            movePiece(possibleMoves[i], &copyState);
            uint8_t king = getKingLocation(owner, &copyState);
            for (int j = 0; j < 64; j++)
            {
                if ((copyState.board[j] & PIECE_OWNER_MASK) == opponent)
                {
                    uint16_t opponentMoves[64];
                    int numOpponentMoves = piecePossibleMoves(j, opponentMoves, &copyState);
                    for (int k = 0; k < numOpponentMoves; k++)
                    {
                        if ((opponentMoves[k] & MOVE_TO_MASK) == king)
                        {
                            legalMove = false;
                            break;
                        }
                    }
                    if (!legalMove)
                    {
                        break;
                    }
                }
            }
        }
        if (legalMove)
        {
            moves[numLegalMoves++] = possibleMoves[i];
        }
    }
    return numLegalMoves;
}

static void orderMoves(uint16_t *moves, int numMoves, GameState *state)
{
    int goodMoves = 0;
    for (int i = 0; i < numMoves; i++)
    {
        uint16_t moveTo = moves[i] & MOVE_TO_MASK;
        uint16_t moveFrom = (moves[i] & MOVE_FROM_MASK) >> MOVE_FROM_SHIFT;
        uint8_t capturedPieceType = state->board[moveTo] & PIECE_TYPE_MASK;
        uint8_t capturingPieceType = state->board[moveFrom] & PIECE_TYPE_MASK;
        if (capturingPieceType < capturedPieceType)
        {
            uint16_t temp = moves[goodMoves];
            moves[goodMoves] = moves[i];
            moves[i] = temp;
            goodMoves++;
        }
    }
}

static int getAllLegalMoves(uint16_t *moves, GameState *state)
{
    uint8_t player = state->playerToMove;
    int totalMoves = 0;
    for (uint8_t cell = 0; cell < 64; cell++)
    {
        if ((state->board[cell] & PIECE_OWNER_MASK) == player)
        {
            int numMoves = pieceLegalMoves(cell, moves + totalMoves, state);
            totalMoves += numMoves;
        }
    }
    /* Order moves putting possible best moves first.
       This improves AI search performance with alpha-beta pruning.
       Doesn't actually change results.  It's just a guess. */
    orderMoves(moves, totalMoves, state);
    return totalMoves;
}

static bool playerInCheck(GameState *state)
{
    uint8_t player = state->playerToMove;
    uint8_t opponent;
    if (player == BLACK)
    {
        opponent = WHITE;
    }
    else
    {
        opponent = BLACK;
    }
    uint8_t king = getKingLocation(player, state);
    for (uint8_t cell = 0; cell < 64; cell++)
    {
        if ((state->board[cell] & PIECE_OWNER_MASK) == opponent)
        {
            uint16_t moves[64];
            int numMoves = piecePossibleMoves(cell, moves, state);
            for (int i = 0; i < numMoves; i++)
            {
                if ((moves[i] & MOVE_TO_MASK) == king)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

static uint64_t calculatePositionsEx(int depth, int startingDepth, GameState *state)
{
    if (depth == 0)
    {
        return 1;
    }
    uint64_t totalPositions = 0;
    uint16_t moves[1024];
    int numMoves = getAllLegalMoves(moves, state);
    for (int i = 0; i < numMoves; i++)
    {
        GameState copyState = *state;
        movePiece(moves[i], &copyState);
        uint64_t positions = calculatePositionsEx(depth - 1, startingDepth, &copyState);
        totalPositions += positions;
        if (depth == startingDepth)
        {
            uint8_t fromCell = (moves[i] & MOVE_FROM_MASK) >> MOVE_FROM_SHIFT;
            uint8_t toCell = moves[i] & MOVE_TO_MASK;
            char string[5];
            string[0] = 'a' + (fromCell % 8);
            string[1] = '8' - (fromCell / 8);
            string[2] = 'a' + (toCell % 8);
            string[3] = '8' - (toCell / 8);
            string[4] = 0;
            char logString[LOG_SIZE];
            snprintf(logString, LOG_SIZE, "%s: %" PRIu64, string, positions);
            debugLog(logString);
        }
    }
    return totalPositions;
}

static uint64_t calculatePositions(int depth, bool verbose)
{
    return calculatePositionsEx(depth, verbose ? depth : -1, &gameState);
}

enum GameEnd checkGameEnd(GameState *state)
{
    uint16_t moves[1024];
    int numMoves = getAllLegalMoves(moves, state);
    if (numMoves > 0)
    {
        if (state == &gameState)
        {
            if (getPositionOccurences(state) >= 3)
            {
                return DRAW_REPITITION;
            }
        }
        if (state->halfMoves >= 100)
        {
            return DRAW_50_MOVE;
        }
        return GAME_NOT_OVER;
    }
    if (playerInCheck(state))
    {
        return CHECKMATE;
    }
    return STALEMATE;
}

static int AIEvaluate(GameState *state)
{
    enum GameEnd end = checkGameEnd(state);
    if (end == CHECKMATE)
    {
        return CHECKMATE_EVALUATION;
    }
    if (end == STALEMATE)
    {
        return STALEMATE_EVALUATION;
    }
    uint8_t player = state->playerToMove;
    int evaluation = 0;
    for (int i = 0; i < 64; i++)
    {
        uint8_t pieceType = state->board[i] & PIECE_TYPE_MASK;
        uint8_t pieceOwner = state->board[i] & PIECE_OWNER_MASK;
        switch (pieceType)
        {
            case PAWN:
            {
                if (pieceOwner == player)
                {
                    evaluation++;
                }
                else
                {
                    evaluation--;
                }
                break;
            }
            case BISHOP:
            case KNIGHT:
            {
                if (pieceOwner == player)
                {
                    evaluation += 3;
                }
                else
                {
                    evaluation -= 3;
                }
                break;
            }
            case ROOK:
            {
                if (pieceOwner == player)
                {
                    evaluation += 5;
                }
                else
                {
                    evaluation -= 5;
                }
                break;
            }
            case QUEEN:
            {
                if (pieceOwner == player)
                {
                    evaluation += 9;
                }
                else
                {
                    evaluation -= 9;
                }
                break;
            }
        }
    }
    return evaluation;
}

static int AISearch(int depth, GameState *state, int alpha, int beta)
{
    if (depth == 0)
    {
        return AIEvaluate(state);
    }
    uint16_t moves[1024];
    int numMoves = getAllLegalMoves(moves, state);
    if (numMoves <= 0)
    {
        if (playerInCheck(state))
        {
            return CHECKMATE_EVALUATION;
        }
        else
        {
            return STALEMATE_EVALUATION;
        }
    }
    for (int i = 0; i < numMoves; i++)
    {
        GameState copyState = *state;
        movePiece(moves[i], &copyState);
        int score = AISearch(depth - 1, &copyState, -beta, -alpha);
        score = -score;
        if (score >= beta)
        {
            return beta;
        }
        if (score > alpha)
        {
            alpha = score;
        }
    }
    return alpha;
}

uint16_t getComputerMove(void)
{
    uint16_t bestMoves[1024];
    uint32_t numBestMoves = 0;
    uint16_t moves[1024];
    int numMoves = getAllLegalMoves(moves, &gameState);
    int alpha = CHECKMATE_EVALUATION;
    for (int i = 0 ; i < numMoves; i++)
    {
        GameState copyState = gameState;
        movePiece(moves[i], &copyState);
        int score = AISearch(3, &copyState, CHECKMATE_EVALUATION, -(alpha - 1));
        score = -score;
        if (score > alpha)
        {
            alpha = score;
            bestMoves[0] = moves[i];
            numBestMoves = 1;
        }
        else if (score == alpha)
        {
            bestMoves[numBestMoves] = moves[i];
            numBestMoves++;
        }
    }

    // Pick a move at random if multiple moves are tied for best evaluation.
    // Helps stop AI from repeating moves.
    if (numBestMoves == 0)
    {
        debugLog("getComputerMove: Did not find a move (this should never happen)");
    }
    return bestMoves[pcgRangedRandom(numBestMoves)];
}

static void loadFenString(const char *str)
{
    for (int i = 0; i < 64; i++)
    {
        gameState.board[i] = 0;
    }
    char c = *str;
    int boardIndex = 0;
    while (boardIndex < 64)
    {
        do
        {
            if (c == 'p')
            {
                gameState.board[boardIndex++] = BLACK | PAWN;
            }
            else if (c == 'n')
            {
                gameState.board[boardIndex++] = BLACK | KNIGHT;
            }
            else if (c == 'b')
            {
                gameState.board[boardIndex++] = BLACK | BISHOP;
            }
            else if (c == 'r')
            {
                gameState.board[boardIndex++] = BLACK | ROOK;
            }
            else if (c == 'q')
            {
                gameState.board[boardIndex++] = BLACK | QUEEN;
            }
            else if (c == 'k')
            {
                gameState.board[boardIndex++] = BLACK | KING;
            }
            else if (c == 'P')
            {
                gameState.board[boardIndex++] = WHITE | PAWN;
            }
            else if (c == 'N')
            {
                gameState.board[boardIndex++] = WHITE | KNIGHT;
            }
            else if (c == 'B')
            {
                gameState.board[boardIndex++] = WHITE | BISHOP;
            }
            else if (c == 'R')
            {
                gameState.board[boardIndex++] = WHITE | ROOK;
            }
            else if (c == 'Q')
            {
                gameState.board[boardIndex++] = WHITE | QUEEN;
            }
            else if (c == 'K')
            {
                gameState.board[boardIndex++] = WHITE | KING;
            }
            else if (c >= '1' && c <= '8')
            {
                boardIndex += c - 48;
            }
            str++;
            c = *str;
        } while (boardIndex % 8 != 0);
        str++;
        c = *str;
    }
    if (c == 'w')
    {
        gameState.playerToMove = WHITE;
    }
    else
    {
        gameState.playerToMove = BLACK;
    }
    str += 2;
    c = *str;
    gameState.castlingAvailablity = 0;
    while (c != ' ')
    {
        if (c == 'K')
        {
            gameState.castlingAvailablity |= CASTLE_WHITE_KING;
        }
        else if (c == 'Q')
        {
            gameState.castlingAvailablity |= CASTLE_WHITE_QUEEN;
        }
        else if (c == 'k')
        {
            gameState.castlingAvailablity |= CASTLE_BLACK_KING;
        }
        else if (c == 'q')
        {
            gameState.castlingAvailablity |= CASTLE_BLACK_QUEEN;
        }
        str++;
        c = *str;
    }
    str++;
    c = *str;
    gameState.enPassantSquare = 0;
    if (c != '-')
    {
        gameState.enPassantSquare += c - 97;
        str++;
        c = *str;
        gameState.enPassantSquare += 56 - ((c - 49) * 8);
    }
    str += 2;
    c = *str;
    char halfMoveString[8];
    int i = 0;
    while (c != ' ')
    {
        halfMoveString[i++] = c;
        str++;
        c = *str;
    }
    halfMoveString[i] = 0;
    gameState.halfMoves = atoi(halfMoveString);

    hashStartingPosition();
}

void initZobrist(void)
{
    for (int i = 0; i < 768; i++)
    {
        zobrist.pieces[i] = pcgGetRandom64();
    }
    for (int i = 0; i < 8; i++)
    {
        zobrist.enPassantFile[i] = pcgGetRandom64();
    }
    zobrist.playerToMove = pcgGetRandom64();
    zobrist.blackQueenCastle = pcgGetRandom64();
    zobrist.whiteQueenCastle = pcgGetRandom64();
    zobrist.blackKingCastle = pcgGetRandom64();
    zobrist.whiteKingCastle = pcgGetRandom64();
}

void initGameState(void)
{
    gameState.halfMoves = 0;
    gameState.enPassantSquare = 255;
    gameState.castlingAvailablity = 255;
    gameState.playerToMove = WHITE;

    gameState.board[0] = BLACK | ROOK;
    gameState.board[1] = BLACK | KNIGHT;
    gameState.board[2] = BLACK | BISHOP;
    gameState.board[3] = BLACK | QUEEN;
    gameState.board[4] = BLACK | KING;
    gameState.board[5] = BLACK | BISHOP;
    gameState.board[6] = BLACK | KNIGHT;
    gameState.board[7] = BLACK | ROOK;

    for (int i = 8; i < 16; i++)
    {
        gameState.board[i] = BLACK | PAWN;
    }

    for (int i = 16; i < 48; i++)
    {
        gameState.board[i] = 0;
    }

    for (int i = 48; i < 56; i++)
    {
        gameState.board[i] = WHITE | PAWN;
    }

    gameState.board[56] = WHITE | ROOK;
    gameState.board[57] = WHITE | KNIGHT;
    gameState.board[58] = WHITE | BISHOP;
    gameState.board[59] = WHITE | QUEEN;
    gameState.board[60] = WHITE | KING;
    gameState.board[61] = WHITE | BISHOP;
    gameState.board[62] = WHITE | KNIGHT;
    gameState.board[63] = WHITE | ROOK;

    hashStartingPosition();
}

static void testFen(const char *fen, int depth, uint64_t expected, bool verbose)
{
    debugLog(fen);
    loadFenString(fen);
    uint64_t positions = calculatePositions(depth, verbose);
    if (positions == expected)
    {
        debugLog("Success");
    }
    else
    {
        char logString[LOG_SIZE];
        snprintf(logString, LOG_SIZE, "Failed: Got: %" PRIu64 " Expected: %" PRIu64, positions, expected);
        debugLog(logString);
    }
}

// Test positions taken from https://www.chessprogramming.org/Perft_Results
void runTests(bool verbose)
{
    testFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5, 4865609, verbose);
    testFen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 0", 5, 193690690, verbose);
    testFen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 0", 5, 674624, verbose);
    testFen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 5, 15833292, verbose);
    testFen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 5, 89941194, verbose);
    testFen("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 5, 164075551, verbose);
}
