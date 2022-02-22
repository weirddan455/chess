#include "events.h"
#include "game.h"
#include "renderer.h"
#include "platform.h"

#include <stddef.h>

bool AIisThinking;

static uint16_t moves[64];
static int numMoves;

void leftClickEvent(int x, int y, bool playerGame)
{
    if (AIisThinking)
    {
        return;
    }
    if (gameOverString != NULL)
    {
        gameOverString = NULL;
        initGameState();
        numHightlighted = 0;
        if (!playerGame)
        {
            AIisThinking = true;
            makeComputerMove();
        }
        return;
    }
    GameArea gameArea = getGameArea();
    x -= gameArea.x;
    y -= gameArea.y;
	if (x < 0 || y < 0 || x >= gameArea.size || y >= gameArea.size)
	{
		return;
	}
	uint8_t cellX = x / gameArea.gridSize;
	uint8_t cellY = y / gameArea.gridSize;
	uint8_t cell = (cellY * 8) + cellX;
    if (pawnPromoteMove)
    {
        uint16_t promotion = 0;
        switch (cell)
        {
            case 10:
            {
                promotion = PAWN_PROMOTE_QUEEN;
                break;
            }
            case 11:
            {
                promotion = PAWN_PROMOTE_BISHOP;
                break;
            }
            case 12:
            {
                promotion = PAWN_PROMOTE_KNIGHT;
                break;
            }
            case 13:
            {
                promotion = PAWN_PROMOTE_ROOK;
                break;
            }
        }
        if (promotion)
        {
            movePiece((pawnPromoteMove & (~PAWN_PROMOTE_MASK)) | promotion, &gameState);
            pawnPromoteMove = 0;
            if (!handleGameOver())
            {
                AIisThinking = true;
                makeComputerMove();
            }
        }
    }
	else if ((gameState.board[cell] & PIECE_OWNER_MASK) == WHITE)
    {
        numMoves = pieceLegalMoves(cell, moves, &gameState);
        if (numMoves > 0)
        {
            for (int i = 0; i < numMoves; i++)
            {
                highlighted[i] = moves[i] & MOVE_TO_MASK;
            }
            highlighted[numMoves] = cell;
            numHightlighted = numMoves + 1;
        }
    }
    else
    {
        uint16_t move = 0;
        for (int i = 0; i < numMoves; i++)
        {
            if ((moves[i] & MOVE_TO_MASK) == cell)
            {
                move = moves[i];
                break;
            }
        }
        if (move)
        {
            numHightlighted = 0;
            numMoves = 0;
            if (move & PAWN_PROMOTE_MASK)
            {
                pawnPromoteMove = move;
            }
            else
            {
                movePiece(move, &gameState);
                if (!handleGameOver())
                {
                    AIisThinking = true;
                    makeComputerMove();
                }
            }
        }
    }
}

void rightClickEvent(void)
{
    numHightlighted = 0;
    numMoves = 0;
}
