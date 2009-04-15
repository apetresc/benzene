//----------------------------------------------------------------------------
// $Id: Game.cpp 1426 2008-06-04 21:18:22Z broderic $
//----------------------------------------------------------------------------

#include "Game.hpp"

//----------------------------------------------------------------------------

Game::Game(StoneBoard& board)
    : m_board(board)
{
    NewGame();
}

void Game::NewGame()
{
    hex::log << hex::fine << "Game::NewGame()" << hex::endl;
    m_board.startNewGame();
    m_history.clear();
}

Game::ReturnType Game::PlayMove(HexColor color, HexPoint cell)
{
    if (cell < 0 || cell >= FIRST_INVALID || !m_board.isValid(cell))
        return INVALID_MOVE;
    if (color == EMPTY) 
        return INVALID_MOVE;
    // FIXME: use board's methods to determine if swap is allowd?
    if (HexPointUtil::isSwap(cell) && m_history.size() != 1)
        return INVALID_MOVE;
    if (m_board.isPlayed(cell))
        return OCCUPIED_CELL;

    m_board.playMove(color, cell);
    m_history.push_back(Move(color, cell));

    return VALID_MOVE;
}

void  Game::UndoMove()
{
    if (m_history.empty())
        return;

    m_board.undoMove(m_history.back().point());
    m_history.pop_back();
}

//----------------------------------------------------------------------------
