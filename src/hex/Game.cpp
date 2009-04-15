//----------------------------------------------------------------------------
// $Id: Game.cpp 1801 2008-12-18 01:48:40Z broderic $
//----------------------------------------------------------------------------

#include "Game.hpp"

//----------------------------------------------------------------------------

Game::Game(StoneBoard& board)
    : m_board(&board),
      m_allow_swap(false),
      m_game_time(1800)
{
    NewGame();
}

void Game::NewGame()
{
    hex::log << hex::fine << "Game::NewGame()" << hex::endl;
    m_board->startNewGame();
    m_time_remaining[BLACK] = m_game_time;
    m_time_remaining[WHITE] = m_game_time;
    m_history.clear();
}

Game::ReturnType Game::PlayMove(HexColor color, HexPoint cell)
{
    if (cell < 0 || cell >= FIRST_INVALID || !m_board->isValid(cell))
        return INVALID_MOVE;
    if (color == EMPTY) 
        return INVALID_MOVE;
    if (HexPointUtil::isSwap(cell))
    {
        if (!m_allow_swap || m_history.size() != 1)
            return INVALID_MOVE;
    }
    if (m_board->isPlayed(cell))
        return OCCUPIED_CELL;

    m_board->playMove(color, cell);
    m_history.push_back(Move(color, cell));

    return VALID_MOVE;
}

void  Game::UndoMove()
{
    if (m_history.empty())
        return;

    m_board->undoMove(m_history.back().point());
    m_history.pop_back();
}

//----------------------------------------------------------------------------
