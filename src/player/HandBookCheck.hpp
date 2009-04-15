//----------------------------------------------------------------------------
// $Id$
//----------------------------------------------------------------------------

#ifndef HANDBOOKCHECK_HPP
#define HANDBOOKCHECK_HPP

#include "UofAPlayer.hpp"

//----------------------------------------------------------------------------

/** Checks book before search. */
class HandBookCheck : public UofAPlayerFunctionality
{
public:

    /** Adds hand-created book check to the given player. */
    HandBookCheck(UofAPlayer* player);
    
    /** Destructor. */
    virtual ~HandBookCheck();
    
    /** Checks if any hand-created move suggestion corresponds to the
	current state if "player-use-hand-book" is true. If matching
	suggestion is found, returns hand book move. Otherwise
        calls player's pre_search() and returns its computed move.
    */
    virtual HexPoint pre_search(HexBoard& brd, const Game& game_state,
				HexColor color, bitset_t& consider,
                                double time_remaining, double& score);
private:

    //----------------------------------------------------------------------
    
    /** Reads in the hand-book from a file and stores the
	board ID-response pairs. */
    void LoadHandBook();
    
    /** Uses the hand book to determine a response (if possible).
        @return INVALID_POINT on failure, valid move on success.
    */
    HexPoint HandBookResponse(const StoneBoard& brd, HexColor color);
    
    bool m_handBookLoaded;
    std::vector<std::string> m_id;
    std::vector<HexPoint> m_response;
};

//----------------------------------------------------------------------------

#endif // HANDBOOKCHECK_HPP
