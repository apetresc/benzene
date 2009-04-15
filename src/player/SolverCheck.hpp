//----------------------------------------------------------------------------
// $Id$
//----------------------------------------------------------------------------

#ifndef SOLVERCHECK_HPP
#define SOLVERCHECK_HPP

#include "UofAPlayer.hpp"

//----------------------------------------------------------------------------

/** Runs solver for a short-time in an attempt to find simple wins
    that the players may miss. */
class SolverCheck : public UofAPlayerFunctionality
{
public:

    /** Extends the given player. */
    SolverCheck(UofAPlayer* player);

    /** Destructor. */
    virtual ~SolverCheck();

    /** Returns a winning move if Solver finds one, otherwise passes
	gamestate onto the player it is extending. Time remaining is
	modified in this case. */
    virtual HexPoint pre_search(HexBoard& brd, const Game& game_state,
				HexColor color, bitset_t& consider,
                                double time_remaining, double& score);
private:

};

//----------------------------------------------------------------------------

#endif // SOLVERCHECK_HPP
