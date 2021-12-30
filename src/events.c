#include "events.h"
#include "game.h"
#include "renderer.h"
#include "pcgrandom.h"

#include <stddef.h>

static bool selected;
static uint8_t moveFrom;
static uint8_t moves[64];
static int numMoves;

bool leftClickEvent(int x, int y)
{
	if (x < 0 || y < 0 || x >= FRAMEBUFFER_WIDTH || y >= FRAMEBUFFER_HEIGHT)
	{
		return true;
	}
	uint8_t cellX = x / GRID_SIZE;
	uint8_t cellY = y / GRID_SIZE;
	uint8_t cell = (cellY * 8) + cellX;
	if (gameState.board[cell] != NULL && gameState.board[cell]->owner == WHITE)
    {
        numMoves = pieceLegalMoves(cell, moves);
        if (numMoves > 0)
        {
            selected = true;
            moveFrom = cell;
            uint8_t highlighted[64];
            for (int i = 0; i < numMoves; i++)
            {
                highlighted[i] = moves[i] & MOVE_MASK;
            }
            highlighted[numMoves] = cell;
            renderFrame(highlighted, numMoves + 1);
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
            selected = false;
            uint8_t computerMoveTo[1024];
            uint8_t computerMoveFrom[1024];
            int numComputerMoves = getAllLegalMoves(BLACK, computerMoveTo, computerMoveFrom);
            if (numComputerMoves <= 0)
            {
                return false;
            }
            uint32_t randomMove = pcgRangedRandom(numComputerMoves);
            movePiece(computerMoveTo[randomMove], computerMoveFrom[randomMove], &gameState);
            renderFrame(NULL, 0);
        }
    }
    return true;
}

void rightClickEvent(void)
{
    selected = false;
    renderFrame(NULL, 0);
}
