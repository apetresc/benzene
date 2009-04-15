//----------------------------------------------------------------------------
// $Id: HandicapPlayer.hpp 1536 2008-07-09 22:47:27Z broderic $
//----------------------------------------------------------------------------

#ifndef HEXHANDICAPPLAYER_HPP
#define HEXHANDICAPPLAYER_HPP

#include "UofAPlayer.hpp"

//----------------------------------------------------------------------------

/** Player using Handicap to generate moves. */
class HandicapPlayer : public UofAPlayer
{
public:

    explicit HandicapPlayer(ICEngine* ice);
    virtual ~HandicapPlayer();

    /** returns "handicap". */
    std::string name() const;

protected:

    /** Generates a move in the given gamestate using handicap
        strategy. The handicap player must always be WHITE.
    */
    virtual HexPoint search(HexBoard& brd, const Game& game_state,
			    HexColor color, const bitset_t& consider,
			    double time_remaining, double& score);
    
private:
    ICEngine* m_ice;
    PointToPoint m_responseMap;
    int m_width;
    bool m_addedStones;

    /** Build the mapping of what the mirror of each HexPoint is */
    void buildResponseMap(const StoneBoard& brd);

    /** Takes two HexPoints and maps them to each other */
    void makeMiai(HexPoint p1, HexPoint p2);

    /** Takes four HexPoints and maps p1, p2, and p3 to dest unless
        p1, p2, and/or p3 are already occupied, in which case there
        are a number of special cases.
        - If dest is occupied by black, p2 is viewed as dead and
        ignored.  p1 is mirrored as most other cells are, and p3 is
        mirrored to the spot above or below it, just outside of the
        pattern of four.
        - If dest is occupied by white and any of the two other spaces
        are occupied, white plays the last spot.
        - If dest is occupied by white and if two or more of p1, p2,
        p3 remain unoccupied then p1 and p2 are mirrored and p3 is
        mapped to the spot above or below it, just outside of the
        pattern of four.
        - Finally, if white occupies any of p1, p2, or p3, then p1 and
        dest are mirrored as most of the other cells are, p2 is
        ignored (as it is either dead for black or occupied by white)
        and p3 is mapped to the spot above or below it, just outside
        of the pattern of four.
    */
    void threeToOne(const StoneBoard& brd, 
                    HexPoint dest, HexPoint p1, HexPoint p2, HexPoint p3);
};

inline std::string HandicapPlayer::name() const
{
    return "handicap";
}

//----------------------------------------------------------------------------

#endif // HEXHANDICAPPLAYER_HPP
