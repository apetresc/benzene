//----------------------------------------------------------------------------
// $Id: EndgameCheck.hpp 1877 2009-01-29 00:57:27Z broderic $
//----------------------------------------------------------------------------

#ifndef ENDGAMECHECK_HPP
#define ENDGAMECHECK_HPP

#include "BenzenePlayer.hpp"

//----------------------------------------------------------------------------

/** Handles VC endgames and prunes the moves to consider to
    the set return by PlayerUtils::MovesToConsider(). 
*/
class EndgameCheck : public BenzenePlayerFunctionality
{
public:

    /** Extends the given player. */
    EndgameCheck(BenzenePlayer* player);

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

    bool Enabled() const;

    void SetEnabled(bool enable);

private:

    bool m_enabled;

};

inline bool EndgameCheck::Enabled() const
{
    return m_enabled;
}

inline void EndgameCheck::SetEnabled(bool enable)
{
    m_enabled = enable;
}

//----------------------------------------------------------------------------

#endif // ENDGAMECHECK_HPP
