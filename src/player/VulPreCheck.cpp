
#include "VulPreCheck.hpp"

//----------------------------------------------------------------------------

VulPreCheck::VulPreCheck(BenzenePlayer* player)
    : BenzenePlayerFunctionality(player)
{
}

VulPreCheck::~VulPreCheck()
{
}

HexPoint VulPreCheck::pre_search(HexBoard& brd, const Game& game_state,
				 HexColor color, bitset_t& consider,
				 double time_remaining, double& score)
{
    hex::log << hex::warning << "Performing vulnerable pre-check..."
	     << hex::endl;
    
    if (!game_state.History().empty()) {
	// Setup the board as it was prior to the opponent's last move.
	GameHistory gh = game_state.History();
	PatternBoard b(brd.width(), brd.height());
	b.startNewGame();
	for (uint i=0; i+1<gh.size(); i++) {
	    HexPoint p = gh[i].point();
	    HexColor c = gh[i].color();
	    
	    /** If we've killed this opponent stone, give it to ourselves.
		This often helps to find more vulnerable opponent moves. */
	    if (m_killedOpptStones.test(p)) {
		HexAssert(c == !color);
		c = !c;
	    }
	    b.playMove(c, p);
	}
	b.update();
	hex::log << hex::warning << "Board before last move:"
		 << b << hex::endl;
	
	// Check if last move played (by opponent) was vulnerable.
	HexPoint lastCell = gh.back().point();
	bitset_t lastMoveOnly;
	lastMoveOnly.set(lastCell);
	hex::log << hex::warning << "Last move on this board:"
		 << b.printBitset(lastMoveOnly) << hex::endl;
	HexAssert(gh.back().color() == !color);
	InferiorCells inf;
	brd.ICE().FindVulnerable(b, !color, lastMoveOnly, inf);
	hex::log << hex::warning << "Inferior cells:"
		 << inf.GuiOutput() << hex::endl;
	
	// If it was, simply return the killer.
	if (inf.Vulnerable().test(lastCell)) {
	    hex::log << hex::warning
		     << "Opponent's last move was vulnerable - killing it!"
		     << hex::endl;
	    std::set<VulnerableKiller> killers = inf.Killers(lastCell);
	    HexAssert(!killers.empty());
	    
	    /** If opponent's last move can be made unconditionally dead,
		this is preferable since we can treat it as such in the
		future, thereby finding more opponent vulnerable cells. */
	    std::set<VulnerableKiller>::iterator i;
	    for (i=killers.begin(); i!=killers.end(); ++i) {
		if (i->carrier().none()) {
		    m_killedOpptStones.set(lastCell);
		    return i->killer();
		}
	    }
	    
	    // Otherwise, just kill it any which way.
	    return killers.begin()->killer();
	}
    }
    
    return m_player->pre_search(brd, game_state, color, consider,
				time_remaining, score);
}

//----------------------------------------------------------------------------
