#include <stdbool.h>
#include <stdlib.h>

#include "game.h"
#include "platform.h"

GameState gameState;

static void copyGameState(GameState *copy)
{
    copy->turn = gameState.turn;
    int pieceIndex = 0;
    for (int i = 0; i < 64; i++)
    {
        if (gameState.board[i] == NULL)
        {
            copy->board[i] = NULL;
        }
        else
        {
            copy->pieces[pieceIndex] = *(gameState.board[i]);
            copy->board[i] = &copy->pieces[pieceIndex];
            pieceIndex++;
        }
    }
}

void movePiece(uint8_t moveTo, uint8_t moveFrom, GameState *state)
{
    uint8_t destination = moveTo & MOVE_MASK;
    Piece *piece = state->board[moveFrom];
    state->board[destination] = piece;
    state->board[moveFrom] = NULL;
    piece->previousPosition = moveFrom;
    piece->lastMoved = state->turn++;
    if (moveTo & ENPASSANT_MASK)
    {
        if (piece->owner == BLACK)
        {
            state->board[destination - 8] = NULL;
        }
        else
        {
            state->board[destination + 8] = NULL;
        }
    }
    else if (moveTo & CASTLE_MASK)
    {
        if (destination > moveFrom)
        {
            state->board[destination - 1] = state->board[destination + 1];
            state->board[destination + 1] = NULL;
        }
        else
        {
            state->board[destination + 1] = state->board[destination - 2];
            state->board[destination - 2] = NULL;
        }
    }
    if (piece->type == PAWN)
    {
        if (piece->owner == BLACK)
        {
            if (destination >= 56)
            {
                piece->type = QUEEN;
            }
        }
        else
        {
            if (destination <= 7)
            {
                piece->type = QUEEN;
            }
        }
    }
}

static uint8_t getKingLocation(enum PieceOwner owner, GameState *state)
{
    for (int i = 0; i < 64; i++)
    {
        if (state->board[i] != NULL && state->board[i]->type == KING && state->board[i]->owner == owner)
        {
            return i;
        }
    }
    debugLog("wtf couldn't find king");
    return 0;
}

static int pawnPossibleMoves(uint8_t cell, uint8_t *moves, GameState *state)
{
    Piece *pawn = state->board[cell];
    int numMoves = 0;
    int direction = pawn->owner == BLACK ? 8: -8;
    int move = cell + direction;
    if (move >= 0 && move < 64 && state->board[move] == NULL)
    {
        moves[numMoves++] = move;
        if (pawn->lastMoved == -1)
        {
            move += direction;
            if (move >= 0 && move < 64 && state->board[move] == NULL)
            {
                moves[numMoves++] = move;
            }
        }
    }
    uint8_t col = cell % 8;
    int captureMoves[2];
    int numCaptureMoves = 0;
    if (pawn->owner == BLACK)
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
        if (captureMoves[i] >= 0 && captureMoves[i] < 64)
        {
            if (state->board[captureMoves[i]] != NULL && state->board[captureMoves[i]]->owner != pawn->owner)
            {
                moves[numMoves++] = captureMoves[i];
            }
            // Check for en passant
            else
            {
                int captureLocation = captureMoves[i] - direction;
                Piece *enemyPawn = state->board[captureLocation];
                if (enemyPawn != NULL && enemyPawn->lastMoved == state->turn - 1 && abs(captureLocation - enemyPawn->previousPosition) == 16)
                {
                    moves[numMoves++] = captureMoves[i] | ENPASSANT_MASK;
                }
            }
        }
    }
    return numMoves;
}

static int knightPossibleMoves(uint8_t cell, uint8_t *moves, GameState *state)
{
    Piece *knight = state->board[cell];
    uint8_t col = cell % 8;
    int possibleMoves[8];
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
        if (possibleMoves[i] >= 0 && possibleMoves[i] < 64 && (state->board[possibleMoves[i]] == NULL || state->board[possibleMoves[i]]->owner != knight->owner))
        {
            moves[numMoves++] = possibleMoves[i];
        }
    }
    return numMoves;
}

static int bishopPossibleMoves(uint8_t cell, uint8_t *moves, GameState *state)
{
    Piece *bishop = state->board[cell];
    uint8_t col = cell % 8;
    uint8_t row = cell / 8;
    uint8_t x = col;
    uint8_t y = row;
    int numMoves = 0;
    // Up left
    uint8_t move = cell - 9;
    while (x > 0 && y > 0)
    {
        if (state->board[move] != NULL)
        {
            if (state->board[move]->owner != bishop->owner)
            {
                moves[numMoves++] = move;
            }
            break;
        }
        moves[numMoves++] = move;
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
        if (state->board[move] != NULL)
        {
            if (state->board[move]->owner != bishop->owner)
            {
                moves[numMoves++] = move;
            }
            break;
        }
        moves[numMoves++] = move;
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
        if (state->board[move] != NULL)
        {
            if (state->board[move]->owner != bishop->owner)
            {
                moves[numMoves++] = move;
            }
            break;
        }
        moves[numMoves++] = move;
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
        if (state->board[move] != NULL)
        {
            if (state->board[move]->owner != bishop->owner)
            {
                moves[numMoves++] = move;
            }
            break;
        }
        moves[numMoves++] = move;
        move += 9;
        x++;
        y++;
    }
    return numMoves;
}

static int rookPossibleMoves(uint8_t cell, uint8_t *moves, GameState *state)
{
    Piece *rook = state->board[cell];
    uint8_t col = cell % 8;
    uint8_t row = cell / 8;
    int numMoves = 0;
    uint8_t x = col;
    uint8_t move = cell - 1;
    while (x > 0)
    {
        if (state->board[move] != NULL)
        {
            if (state->board[move]->owner != rook->owner)
            {
                moves[numMoves++] = move;
            }
            break;
        }
        moves[numMoves++] = move;
        move--;
        x--;
    }
    x = col;
    move = cell + 1;
    while (x < 7)
    {
        if (state->board[move] != NULL)
        {
            if (state->board[move]->owner != rook->owner)
            {
                moves[numMoves++] = move;
            }
            break;
        }
        moves[numMoves++] = move;
        move++;
        x++;
    }
    uint8_t y = row;
    move = cell - 8;
    while (y > 0)
    {
        if (state->board[move] != NULL)
        {
            if (state->board[move]->owner != rook->owner)
            {
                moves[numMoves++] = move;
            }
            break;
        }
        moves[numMoves++] = move;
        move -= 8;
        y--;
    }
    y = row;
    move = cell + 8;
    while (y < 7)
    {
        if (state->board[move] != NULL)
        {
            if (state->board[move]->owner != rook->owner)
            {
                moves[numMoves++] = move;
            }
            break;
        }
        moves[numMoves++] = move;
        move += 8;
        y++;
    }
    return numMoves;
}

static int queenPossibleMoves(uint8_t cell, uint8_t *moves, GameState *state)
{
    int numMoves = bishopPossibleMoves(cell, moves, state);
    return numMoves + rookPossibleMoves(cell, moves + numMoves, state);
}

static int getCastlingMoves(Piece *king, uint8_t *moves, GameState *state)
{
    if (king->lastMoved != -1)
    {
        return 0;
    }
    enum PieceOwner owner = king->owner;
    uint8_t backRow = owner == BLACK ? 0 : 56;
    bool queenSide = true;
    bool kingSide = true;
    if (state->board[backRow] == NULL || state->board[backRow]->lastMoved != -1)
    {
        queenSide = false;
    }
    if (state->board[backRow + 7] == NULL || state->board[backRow + 7]->lastMoved != -1)
    {
        kingSide = false;
    }
    if (queenSide)
    {
        for (int i = 1; i < 4; i++)
        {
            if (state->board[backRow + i] != NULL)
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
            if (state->board[backRow + i] != NULL)
            {
                kingSide = false;
                break;
            }
        }
    }
    int numMoves = 0;
    if (queenSide)
    {
        moves[numMoves++] = (backRow + 2) | CASTLE_MASK;
    }
    if (kingSide)
    {
        moves[numMoves++] = (backRow + 6) | CASTLE_MASK;
    }
    return numMoves;
}

static int kingPossibleMoves(uint8_t cell, uint8_t *moves, GameState *state)
{
    Piece *king = state->board[cell];
    uint8_t col = cell % 8;
    uint8_t row = cell / 8;
    int numMoves = 0;
    uint8_t move = cell - 1;
    if (col > 0 && (state->board[move] == NULL || state->board[move]->owner != king->owner))
    {
        moves[numMoves++] = move;
    }
    move = cell + 1;
    if (col < 7 && (state->board[move] == NULL || state->board[move]->owner != king->owner))
    {
        moves[numMoves++] = move;
    }
    move = cell - 8;
    if (row > 0 && (state->board[move] == NULL || state->board[move]->owner != king->owner))
    {
        moves[numMoves++] = move;
    }
    move = cell + 8;
    if (row < 7 && (state->board[move] == NULL || state->board[move]->owner != king->owner))
    {
        moves[numMoves++] = move;
    }
    move = cell - 9;
    if (col > 0 && row > 0 && (state->board[move] == NULL || state->board[move]->owner != king->owner))
    {
        moves[numMoves++] = move;
    }
    move = cell + 7;
    if (col > 0 && row < 7 && (state->board[move] == NULL || state->board[move]->owner != king->owner))
    {
        moves[numMoves++] = move;
    }
    move = cell - 7;
    if (col < 7 && row > 0 && (state->board[move] == NULL || state->board[move]->owner != king->owner))
    {
        moves[numMoves++] = move;
    }
    move = cell + 9;
    if (col < 7 && row < 7 && (state->board[move] == NULL || state->board[move]->owner != king->owner))
    {
        moves[numMoves++] = move;
    }
    return numMoves + getCastlingMoves(king, moves + numMoves, state);
}

static int piecePossibleMoves(uint8_t cell, uint8_t *moves, GameState *state)
{
    switch(state->board[cell]->type)
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

int pieceLegalMoves(uint8_t cell, uint8_t *moves)
{
    enum PieceOwner owner = gameState.board[cell]->owner;
    uint8_t possibleMoves[64];
    int numPossibleMoves = piecePossibleMoves(cell, possibleMoves, &gameState);
    int numLegalMoves = 0;
    for (int i = 0; i < numPossibleMoves; i++)
    {
        bool legalMove = true;
        if (possibleMoves[i] & CASTLE_MASK)
        {
            uint8_t backRow = owner == BLACK ? 0 : 56;
            uint8_t king = backRow + 4;
            bool kingSide = (possibleMoves[i] & MOVE_MASK) > cell;
            for (int j = 0; j < 64; j++)
            {
                if (gameState.board[j] != NULL && gameState.board[j]->owner != owner)
                {
                    uint8_t opponentMoves[64];
                    int numOpponentMoves = piecePossibleMoves(j, opponentMoves, &gameState);
                    for (int k = 0; k < numOpponentMoves; k++)
                    {
                        uint8_t move = opponentMoves[k] & MOVE_MASK;
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
            GameState copyState;
            copyGameState(&copyState);
            movePiece(possibleMoves[i], cell, &copyState);
            uint8_t king = getKingLocation(owner, &copyState);
            for (int j = 0; j < 64; j++)
            {
                if (copyState.board[j] != NULL && copyState.board[j]->owner != owner)
                {
                    uint8_t opponentMoves[64];
                    int numOpponentMoves = piecePossibleMoves(j, opponentMoves, &copyState);
                    for (int k = 0; k < numOpponentMoves; k++)
                    {
                        if ((opponentMoves[k] & MOVE_MASK) == king)
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

int getAllLegalMoves(enum PieceOwner owner, uint8_t *moveTo, uint8_t *moveFrom)
{
    int totalMoves = 0;
    for (uint8_t cell = 0; cell < 64; cell++)
    {
        if (gameState.board[cell] != NULL && gameState.board[cell]->owner == owner)
        {
            int numMoves = pieceLegalMoves(cell, moveTo + totalMoves);
            for (int i = 0; i < numMoves; i++)
            {
                moveFrom[totalMoves + i] = cell;
            }
            totalMoves += numMoves;
        }
    }
    return totalMoves;
}

void initGameState(void)
{
    gameState.turn = 1;

    for (int i = 0; i < 32; i++)
    {
        gameState.pieces[i].previousPosition = -1;
        gameState.pieces[i].lastMoved = -1;
    }

    gameState.pieces[0].owner = BLACK;
    gameState.pieces[0].type = ROOK;
    gameState.pieces[1].owner = BLACK;
    gameState.pieces[1].type = KNIGHT;
    gameState.pieces[2].owner = BLACK;
    gameState.pieces[2].type = BISHOP;
    gameState.pieces[3].owner = BLACK;
    gameState.pieces[3].type = QUEEN;
    gameState.pieces[4].owner = BLACK;
    gameState.pieces[4].type = KING;
    gameState.pieces[5].owner = BLACK;
    gameState.pieces[5].type = BISHOP;
    gameState.pieces[6].owner = BLACK;
    gameState.pieces[6].type = KNIGHT;
    gameState.pieces[7].owner = BLACK;
    gameState.pieces[7].type = ROOK;

    gameState.pieces[8].owner = BLACK;
    gameState.pieces[8].type = PAWN;
    gameState.pieces[9].owner = BLACK;
    gameState.pieces[9].type = PAWN;
    gameState.pieces[10].owner = BLACK;
    gameState.pieces[10].type = PAWN;
    gameState.pieces[11].owner = BLACK;
    gameState.pieces[11].type = PAWN;
    gameState.pieces[12].owner = BLACK;
    gameState.pieces[12].type = PAWN;
    gameState.pieces[13].owner = BLACK;
    gameState.pieces[13].type = PAWN;
    gameState.pieces[14].owner = BLACK;
    gameState.pieces[14].type = PAWN;
    gameState.pieces[15].owner = BLACK;
    gameState.pieces[15].type = PAWN;

    gameState.pieces[16].owner = WHITE;
    gameState.pieces[16].type = PAWN;
    gameState.pieces[17].owner = WHITE;
    gameState.pieces[17].type = PAWN;
    gameState.pieces[18].owner = WHITE;
    gameState.pieces[18].type = PAWN;
    gameState.pieces[19].owner = WHITE;
    gameState.pieces[19].type = PAWN;
    gameState.pieces[20].owner = WHITE;
    gameState.pieces[20].type = PAWN;
    gameState.pieces[21].owner = WHITE;
    gameState.pieces[21].type = PAWN;
    gameState.pieces[22].owner = WHITE;
    gameState.pieces[22].type = PAWN;
    gameState.pieces[23].owner = WHITE;
    gameState.pieces[23].type = PAWN;

    gameState.pieces[24].owner = WHITE;
    gameState.pieces[24].type = ROOK;
    gameState.pieces[25].owner = WHITE;
    gameState.pieces[25].type = KNIGHT;
    gameState.pieces[26].owner = WHITE;
    gameState.pieces[26].type = BISHOP;
    gameState.pieces[27].owner = WHITE;
    gameState.pieces[27].type = QUEEN;
    gameState.pieces[28].owner = WHITE;
    gameState.pieces[28].type = KING;
    gameState.pieces[29].owner = WHITE;
    gameState.pieces[29].type = BISHOP;
    gameState.pieces[30].owner = WHITE;
    gameState.pieces[30].type = KNIGHT;
    gameState.pieces[31].owner = WHITE;
    gameState.pieces[31].type = ROOK;

    gameState.board[0] = &gameState.pieces[0];
    gameState.board[1] = &gameState.pieces[1];
    gameState.board[2] = &gameState.pieces[2];
    gameState.board[3] = &gameState.pieces[3];
    gameState.board[4] = &gameState.pieces[4];
    gameState.board[5] = &gameState.pieces[5];
    gameState.board[6] = &gameState.pieces[6];
    gameState.board[7] = &gameState.pieces[7];
    gameState.board[8] = &gameState.pieces[8];
    gameState.board[9] = &gameState.pieces[9];
    gameState.board[10] = &gameState.pieces[10];
    gameState.board[11] = &gameState.pieces[11];
    gameState.board[12] = &gameState.pieces[12];
    gameState.board[13] = &gameState.pieces[13];
    gameState.board[14] = &gameState.pieces[14];
    gameState.board[15] = &gameState.pieces[15];

    gameState.board[48] = &gameState.pieces[16];
    gameState.board[49] = &gameState.pieces[17];
    gameState.board[50] = &gameState.pieces[18];
    gameState.board[51] = &gameState.pieces[19];
    gameState.board[52] = &gameState.pieces[20];
    gameState.board[53] = &gameState.pieces[21];
    gameState.board[54] = &gameState.pieces[22];
    gameState.board[55] = &gameState.pieces[23];
    gameState.board[56] = &gameState.pieces[24];
    gameState.board[57] = &gameState.pieces[25];
    gameState.board[58] = &gameState.pieces[26];
    gameState.board[59] = &gameState.pieces[27];
    gameState.board[60] = &gameState.pieces[28];
    gameState.board[61] = &gameState.pieces[29];
    gameState.board[62] = &gameState.pieces[30];
    gameState.board[63] = &gameState.pieces[31];
}
