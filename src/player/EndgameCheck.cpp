//----------------------------------------------------------------------------
// $Id: EndgameCheck.cpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#include "EndgameCheck.hpp"
#include "PlayerUtils.hpp"

//----------------------------------------------------------------------------

EndgameCheck::EndgameCheck(UofAPlayer* player)
    : UofAPlayerFunctionality(player)
{
}

EndgameCheck::~EndgameCheck()
{
}

HexPoint EndgameCheck::pre_search(HexBoard& brd, const Game& game_state,
                                  HexColor color, bitset_t& consider,
                                  double time_remaining, double& score)
{
    if (hex::settings.get_bool("player-endgame-enabled"))
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
