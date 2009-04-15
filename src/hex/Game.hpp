//----------------------------------------------------------------------------
// $Id: Game.hpp 1426 2008-06-04 21:18:22Z broderic $
//----------------------------------------------------------------------------

#ifndef GAME_HPP
#define GAME_HPP

#include "Hex.hpp"
#include "Move.hpp"
#include "StoneBoard.hpp"

//----------------------------------------------------------------------------

/** History of moves played in the game. */
typedef std::vector<Move> GameHistory;

//----------------------------------------------------------------------------

/** A Game of Hex. */
class Game
{
public:
    
    /** Creates a new game on the given board. Calls NewGame(). */
    explicit Game(StoneBoard& brd);

    //----------------------------------------------------------------------

    /** Starts a new game. The board and move history are cleared. */
    void NewGame();

    //----------------------------------------------------------------------

    typedef enum { INVALID_MOVE, OCCUPIED_CELL, VALID_MOVE } ReturnType;

    /** If move is invalid (color is not BLACK or WHITE, point is an
        invalid point, point is swap when swap not available) then
        INVALID_MOVE is returned and game/board is not changed.  If
        point is occupied, returns CELL_OCCUPIED.  Otherwise, returns
        VALID_MOVE and plays the move to the board and adds it to the
        game's history.  */
    ReturnType PlayMove(HexColor color, HexPoint point);

    /** The last move is cleared from the board and removed from the
        game history.  Does nothing if history is empty. */
    void UndoMove();

    //----------------------------------------------------------------------

    /** Returns the game board. */
    StoneBoard& Board();

    /** Returns a constant reference to the game board. */
    const StoneBoard& Board() const;

    /** Returns the history of moves. */
    const GameHistory& History() const;

private:
    StoneBoard& m_board;
    GameHistory m_history;
};

inline StoneBoard& Game::Board() 
{
    return m_board;
}

inline const StoneBoard& Game::Board() const
{
    return m_board;
}

inline const GameHistory& Game::History() const
{
    return m_history;
}

//----------------------------------------------------------------------------

#endif  // GAME_HPP
