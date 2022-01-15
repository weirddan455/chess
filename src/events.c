#include "events.h"
#include "game.h"
#include "renderer.h"
#include "pcgrandom.h"

#include <stddef.h>

static bool selected;
static uint8_t moveFrom;
static uint8_t moves[64];
static int numMoves;

void leftClickEvent(int x, int y)
{
    if (renderString != NULL)
    {
        renderString = NULL;
        initGameState();
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
	if ((gameState.board[cell] & PIECE_OWNER_MASK) == WHITE)
    {
        numMoves = pieceLegalMoves(cell, moves, &gameState);
        if (numMoves > 0)
        {
            selected = true;
            moveFrom = cell;
            for (int i = 0; i < numMoves; i++)
            {
                highlighted[i] = moves[i] & MOVE_MASK;
            }
            highlighted[numMoves] = cell;
            numHightlighted = numMoves + 1;
        }
    }
    else if (selected)
    {
        uint8_t moveTo = 255;
        for (int i = 0; i < numMoves; i++)
        {
            if ((moves[i] & MOVE_MASK) == cell)
            {
                moveTo = moves[i];
                break;
            }
        }
        if (moveTo != 255)
        {
            movePiece(moveTo, moveFrom, &gameState);
            uint8_t legalMovesTo[1024];
            uint8_t legalMovesFrom[1024];
            int numComputerMoves = getAllLegalMoves(BLACK, legalMovesTo, legalMovesFrom, &gameState);
            if (numComputerMoves <= 0)
            {
                if (playerInCheck(BLACK))
                {
                    renderString = "White Wins - Checkmate";
                }
                else
                {
                    renderString = "Stalemate";
                }
            }
            else if (gameState.halfMoves >= 100)
            {
                renderString = "Draw by 50 rule move";
            }
            else
            {
                uint32_t randomMove = pcgRangedRandom(numComputerMoves);
                movePiece(legalMovesTo[randomMove], legalMovesFrom[randomMove], &gameState);
                int numPlayerMoves = getAllLegalMoves(WHITE, legalMovesTo, legalMovesFrom, &gameState);
                if (numPlayerMoves <= 0)
                {
                    if (playerInCheck(WHITE))
                    {
                        renderString = "Black Wins - Checkmate";
                    }
                    else
                    {
                        renderString = "Stalemate";
                    }
                }
                else if (gameState.halfMoves >= 100)
                {
                    renderString = "Draw by 50 rule move";
                }
            }
            selected = false;
            numHightlighted = 0;
        }
    }
}

void rightClickEvent(void)
{
    selected = false;
    numHightlighted = 0;
}
