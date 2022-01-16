#include <stdlib.h>
#include <stdio.h>

#include "game.h"
#include "platform.h"

GameState gameState;

void movePiece(uint16_t move, GameState *state)
{
    uint8_t moveTo = move & MOVE_TO_MASK;
    uint8_t moveFrom = (move & MOVE_FROM_MASK) >> MOVE_FROM_SHIFT;
    uint8_t piece = state->board[moveFrom];
    uint8_t pieceOwner = piece & PIECE_OWNER_MASK;
    uint8_t pieceType = piece & PIECE_TYPE_MASK;
    if (pieceType == PAWN || state->board[moveTo] != 0)
    {
        state->halfMoves = 0;
    }
    else
    {
        state->halfMoves++;
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
                state->board[moveTo - 1] = state->board[moveTo + 1];
                state->board[moveTo + 1] = 0;
            }
            else
            {
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

int getAllLegalMoves(uint8_t owner, uint16_t *moves, GameState *state)
{
    int totalMoves = 0;
    for (uint8_t cell = 0; cell < 64; cell++)
    {
        if ((state->board[cell] & PIECE_OWNER_MASK) == owner)
        {
            int numMoves = pieceLegalMoves(cell, moves + totalMoves, state);
            totalMoves += numMoves;
        }
    }
    return totalMoves;
}

bool playerInCheck(uint8_t player)
{
    uint8_t opponent;
    if (player == BLACK)
    {
        opponent = WHITE;
    }
    else
    {
        opponent = BLACK;
    }
    uint8_t king = getKingLocation(player, &gameState);
    for (uint8_t cell = 0; cell < 64; cell++)
    {
        if ((gameState.board[cell] & PIECE_OWNER_MASK) == opponent)
        {
            uint16_t moves[64];
            int numMoves = piecePossibleMoves(cell, moves, &gameState);
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

uint64_t calculatePositions(int depth, GameState *state, uint8_t player)
{
    if (depth == 0)
    {
        return 1;
    }
    uint8_t opponent;
    if (player == BLACK)
    {
        opponent = WHITE;
    }
    else
    {
        opponent = BLACK;
    }
    uint64_t totalPositions = 0;
    uint16_t moves[1024];
    int numMoves = getAllLegalMoves(player, moves, state);
    for (int i = 0; i < numMoves; i++)
    {
        GameState copyState = *state;
        movePiece(moves[i], &copyState);
        uint64_t positions = calculatePositions(depth - 1, &copyState, opponent);
        totalPositions += positions;
        if (depth == 5)
        {
            uint8_t fromCell = moves[i] & MOVE_TO_MASK;
            uint8_t toCell = (moves[i] & MOVE_FROM_MASK) >> MOVE_FROM_SHIFT;
            char string[5];
            string[0] = 'a' + (fromCell % 8);
            string[1] = '8' - (fromCell / 8);
            string[2] = 'a' + (toCell % 8);
            string[3] = '8' - (toCell / 8);
            string[4] = 0;
            printf("%s: %lu\n", string, positions);
        }
    }
    return totalPositions;
}

void loadFenString(const char *str)
{
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
}
