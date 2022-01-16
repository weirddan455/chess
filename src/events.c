#include "events.h"
#include "game.h"
#include "renderer.h"
#include "pcgrandom.h"

#include <stddef.h>

static bool selected;
static uint16_t moves[64];
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
            for (int i = 0; i < numMoves; i++)
            {
                highlighted[i] = moves[i] & MOVE_TO_MASK;
            }
            highlighted[numMoves] = cell;
            numHightlighted = numMoves + 1;
        }
    }
    else if (selected)
    {
        uint16_t move;
        bool validMove = false;
        for (int i = 0; i < numMoves; i++)
        {
            if ((moves[i] & MOVE_TO_MASK) == cell)
            {
                move = moves[i];
                validMove = true;
                break;
            }
        }
        if (validMove)
        {
            movePiece(move, &gameState);
            uint16_t legalMoves[1024];
            int numComputerMoves = getAllLegalMoves(BLACK, legalMoves, &gameState);
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
                movePiece(legalMoves[randomMove], &gameState);
                int numPlayerMoves = getAllLegalMoves(WHITE, legalMoves, &gameState);
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
