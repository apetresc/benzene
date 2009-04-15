//----------------------------------------------------------------------------
// $Id: SwapCheck.hpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#ifndef SWAPCHECK_HPP
#define SWAPCHECK_HPP

#include "UofAPlayer.hpp"

//----------------------------------------------------------------------------

/** Checks swap before search. */
class SwapCheck : public UofAPlayerFunctionality
{
public:

    /** Adds pre-check for swap rule decision to the given player. */
    SwapCheck(UofAPlayer* player);

    /** Destructor. */
    virtual ~SwapCheck();

    /** If first move of game has been played and swap rule is being used,
	determines whether or not to swap.
	Note: when does not swap, assumes player will search for a valid
	cell (i.e. non-swap) response.
    */
    virtual HexPoint pre_search(HexBoard& brd, const Game& game_state,
				HexColor color, bitset_t& consider,
                                double time_remaining, double& score);

private:
    UofAPlayer* m_player;
};

//----------------------------------------------------------------------------

#endif // SWAPCHECK_HPP
