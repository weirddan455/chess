#include "events.h"
#include "game.h"
#include "renderer.h"

#include <stddef.h>

static uint16_t moves[64];
static int numMoves;

static void makeComputerMove(void)
{
    enum GameEnd end = checkGameEnd(&gameState);
    if (end == CHECKMATE)
    {
        gameOverString = "White Wins - Checkmate";
        return;
    }
    else if (end == STALEMATE)
    {
        gameOverString = "Stalemate";
        return;
    }
    else if (end == DRAW_50_MOVE)
    {
        gameOverString = "Draw by 50 rule move";
        return;
    }
    uint16_t move = getComputerMove();
    movePiece(move, &gameState);
    end = checkGameEnd(&gameState);
    if (end == CHECKMATE)
    {
        gameOverString = "Black Wins - Checkmate";
    }
    else if (end == STALEMATE)
    {
        gameOverString = "Stalemate";
    }
    else if (end == DRAW_50_MOVE)
    {
        gameOverString = "Draw by 50 rule move";
    }
}

void leftClickEvent(int x, int y)
{
    if (gameOverString != NULL)
    {
        gameOverString = NULL;
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
    if (pawnPromoteCell != 255)
    {
        switch (cell)
        {
            case 10:
            {
                gameState.board[pawnPromoteCell] = WHITE | QUEEN;
                pawnPromoteCell = 255;
                break;
            }
            case 11:
            {
                gameState.board[pawnPromoteCell] = WHITE | BISHOP;
                pawnPromoteCell = 255;
                break;
            }
            case 12:
            {
                gameState.board[pawnPromoteCell] = WHITE | KNIGHT;
                pawnPromoteCell = 255;
                break;
            }
            case 13:
            {
                gameState.board[pawnPromoteCell] = WHITE | ROOK;
                pawnPromoteCell = 255;
                break;
            }
        }
        if (pawnPromoteCell == 255)
        {
            makeComputerMove();
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
    else if (numHightlighted > 0)
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
            numHightlighted = 0;
            if (move & PAWN_PROMOTE_MASK)
            {
                pawnPromoteCell = cell;
                movePiece(move & (~PAWN_PROMOTE_MASK), &gameState);
            }
            else
            {
                movePiece(move, &gameState);
                makeComputerMove();
            }
        }
    }
}

void rightClickEvent(void)
{
    numHightlighted = 0;
}
