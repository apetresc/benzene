//----------------------------------------------------------------------------
// $Id: UofAPlayer.cpp 1536 2008-07-09 22:47:27Z broderic $
//----------------------------------------------------------------------------

#include "UofAPlayer.hpp"
#include "PlayerUtils.hpp"
#include "BoardUtils.cpp"

//----------------------------------------------------------------------------

UofAPlayer::UofAPlayer()
    : HexPlayer()
{
}

UofAPlayer::~UofAPlayer()
{
}

//----------------------------------------------------------------------------

HexPoint UofAPlayer::genmove(HexBoard& brd, 
                             const Game& game_state, HexColor color,
                             double time_remaining, double& score)
{
    HexPoint move = INVALID_POINT;
    bitset_t consider;

    move = init_search(brd, color, consider, time_remaining, score);
    if (move != INVALID_POINT)
        return move;

    //----------------------------------------------------------------------

    /** @bug Subtract time spent to here from time_remaining! */
    move = pre_search(brd, game_state, color, consider, time_remaining, score);
    if (move != INVALID_POINT) 
        return move;

    //----------------------------------------------------------------------

    hex::log << hex::info << "Best move cannot be determined,"
             << " must search state." << hex::endl;
    /** @bug Subtract time spent to here from time_remaining! */
    move = search(brd, game_state, color, consider, time_remaining, score);

    //----------------------------------------------------------------------

    hex::log << hex::info << "Applying post search heuristics..." << hex::endl;
    /** @bug Subtract time spent to here from time_remaining! */
    return post_search(move, brd, color, time_remaining, score);
}

HexPoint UofAPlayer::init_search(HexBoard& brd, 
                                 HexColor color, 
                                 bitset_t& consider, 
                                 double UNUSED(time_remaining), 
                                 double& score)
{
    // set the board flags before doing anything
    brd.SetFlags(BoardFlags());
    
    // resign if the game is already over
    brd.absorb();
    if (brd.isGameOver()) {
        score = IMMEDIATE_LOSS;
        return RESIGN;
    }

    // compute vcs/ice and set moves to consider to all empty cells
    brd.ComputeAll(color, HexBoard::REMOVE_WINNING_FILLIN);
    consider = brd.getEmpty();
    score = 0;

    hex::log << hex::info << brd << hex::endl;

    return INVALID_POINT;
}

HexPoint UofAPlayer::pre_search(HexBoard& UNUSED(brd), 
                                const Game& UNUSED(game_state),
				HexColor UNUSED(color), 
                                bitset_t& UNUSED(consider),
				double UNUSED(time_remaining), 
                                double& UNUSED(score))
{
    return INVALID_POINT;
}

HexPoint UofAPlayer::search(HexBoard& brd, 
                            const Game& UNUSED(game_state),
                            HexColor UNUSED(color), 
                            const bitset_t& UNUSED(consider),
                            double UNUSED(time_remaining), 
                            double& UNUSED(score))
{
    HexAssert(false); // Defense against Phil's stupidity
    return BoardUtils::RandomEmptyCell(brd);
}
    
HexPoint UofAPlayer::post_search(HexPoint move, 
                                 HexBoard& UNUSED(brd), 
                                 HexColor UNUSED(color), 
                                 double UNUSED(time_remaining), 
                                 double& UNUSED(score))
{
    return move;
}

//----------------------------------------------------------------------------

int UofAPlayer::BoardFlags() const
{
    int flags = HexBoard::USE_ICE | HexBoard::USE_VCS;

    if (hex::settings.get_bool("global-use-decompositions")) 
        flags |= HexBoard::USE_DECOMPOSITIONS;

    return flags;
}

//----------------------------------------------------------------------------
