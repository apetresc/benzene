//----------------------------------------------------------------------------
// $Id: LadderCheck.hpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#ifndef LADDERCHECK_HPP
#define LADDERCHECK_HPP

#include "UofAPlayer.hpp"

//----------------------------------------------------------------------------

/** Checks for bad ladder probes and removes them from the moves to 
    consider. */
class LadderCheck : public UofAPlayerFunctionality
{
public:

    /** Adds pre-check for vulnerable cells to the given player. */
    LadderCheck(UofAPlayer* player);

    /** Destructor. */
    virtual ~LadderCheck();

    /** Removes bad ladder probes from the set of moves to consider. */
    virtual HexPoint pre_search(HexBoard& brd, const Game& game_state,
				HexColor color, bitset_t& consider,
                                double time_remaining, double& score);

private:

};

//----------------------------------------------------------------------------

#endif // LADDERCHECK_HPP
