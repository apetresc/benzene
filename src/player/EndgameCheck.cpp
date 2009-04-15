//----------------------------------------------------------------------------
// $Id: EndgameCheck.cpp 1877 2009-01-29 00:57:27Z broderic $
//----------------------------------------------------------------------------

#include "EndgameCheck.hpp"
#include "PlayerUtils.hpp"

//----------------------------------------------------------------------------

EndgameCheck::EndgameCheck(BenzenePlayer* player)
    : BenzenePlayerFunctionality(player),
      m_enabled(true)
{
}

EndgameCheck::~EndgameCheck()
{
}

HexPoint EndgameCheck::pre_search(HexBoard& brd, const Game& game_state,
                                  HexColor color, bitset_t& consider,
                                  double time_remaining, double& score)
{
    if (m_enabled)
    {
        if (PlayerUtils::IsDeterminedState(brd, color, score)) 
        {
            return PlayerUtils::PlayDeterminedState(brd, color);
        } 
        else 
        {
            consider = PlayerUtils::MovesToConsider(brd, color);
            HexAssert(consider.any());
        }

        score = 0;

        if (consider.count() == 1) {
            HexPoint move = static_cast<HexPoint>
                (BitsetUtil::FindSetBit(consider));
            hex::log << hex::info << "Mustplay is singleton!" << hex::endl;
            return move;
        }
    }

    return m_player->pre_search(brd, game_state, color, consider,
				time_remaining, score);
}

//----------------------------------------------------------------------------
