//----------------------------------------------------------------------------
// $Id: EndgameCheck.hpp 1536 2008-07-09 22:47:27Z broderic $
//----------------------------------------------------------------------------

#ifndef ENDGAMECHECK_HPP
#define ENDGAMECHECK_HPP

#include "UofAPlayer.hpp"

//----------------------------------------------------------------------------

/** Handles VC endgames and prunes the moves to consider to
    the set return by PlayerUtils::MovesToConsider(). 
*/
class EndgameCheck : public UofAPlayerFunctionality
{
public:

    /** Extends the given player. */
    EndgameCheck(UofAPlayer* player);

    /** Destructor. */
    virtual ~EndgameCheck();

    /** If PlayerUtils::IsDeterminedState() is true, returns
        PlayerUtils::PlayInDeterminedState().  Otherwise, returns
        INVALID_POINT and prunes consider by
        PlayerUtils::MovesToConsider().
     */
    virtual HexPoint pre_search(HexBoard& brd, const Game& game_state,
				HexColor color, bitset_t& consider,
                                double time_remaining, double& score);
private:

};

//----------------------------------------------------------------------------

#endif // ENDGAMECHECK_HPP
