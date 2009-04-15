//----------------------------------------------------------------------------
// $Id: MoHexPlayer.hpp 1699 2008-09-24 23:55:56Z broderic $
//----------------------------------------------------------------------------

#ifndef MOHEXPLAYER_HPP
#define MOHEXPLAYER_HPP

#include "UofAPlayer.hpp"

//----------------------------------------------------------------------------

/** Player using UctEngine to generate moves. */
class MoHexPlayer : public UofAPlayer
{
public:

    /** Constructor. */
    MoHexPlayer();

    /** Destructor. */
    virtual ~MoHexPlayer();

    /** Returns "mohex". */
    std::string name() const;
    
protected:

    /** Generates a move in the given gamestate using uct. */
    virtual HexPoint search(HexBoard& brd, const Game& game_state,
			    HexColor color, const bitset_t& consider,
                            double time_remaining, double& score);

    /** Does the search. */
    HexPoint Search(HexBoard& b, HexColor color, HexPoint lastMove,
                    const bitset_t& given_to_consider, 
                    double time_remaining, double& score);

    /** Does the one-ply pre-search. */
    void ComputeUctInitialData(int numthreads, 
                               HexBoard& brd, HexColor color,
			       bitset_t& consider,
			       HexUctInitialData& data, 
			       HexPoint& oneMoveWin);

    int NumberOfGames(const ConstBoard& brd) const;

};

inline std::string MoHexPlayer::name() const
{
    return "mohex";
}

//----------------------------------------------------------------------------

#endif // MOHEXPLAYER_HPP
