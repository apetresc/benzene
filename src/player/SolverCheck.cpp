//----------------------------------------------------------------------------
// $Id$
//----------------------------------------------------------------------------

#include "SgSystem.h"
#include "SgTimer.h"

#include <boost/scoped_ptr.hpp>

#include "Time.hpp"
#include "Solver.hpp"
#include "SolverCheck.hpp"
#include "PlayerUtils.hpp"

//----------------------------------------------------------------------------

SolverCheck::SolverCheck(UofAPlayer* player)
    : UofAPlayerFunctionality(player)
{
}

SolverCheck::~SolverCheck()
{
}

HexPoint SolverCheck::pre_search(HexBoard& brd, const Game& game_state,
				 HexColor color, bitset_t& consider,
				 double time_remaining, double& score)
{
    if (hex::settings.get_bool("player-use-solver-check") && 
        hex::settings.get_int("player-solver-check-threshold")
        < (int)game_state.History().size())
    {
        int timelimit = hex::settings.get_int("player-solver-check-timelimit");
        hex::log << hex::info << "SolverCheck: Trying to solve in " 
		 << FormattedTime(timelimit) << "." << hex::endl;

	boost::scoped_ptr<HexBoard> bd(new HexBoard(brd.width(), 
						    brd.height(),
						    brd.ICE()));
        bd->startNewGame();
        bd->setColor(BLACK, brd.getBlack());
        bd->setColor(WHITE, brd.getWhite());
	bd->setPlayed(brd.getPlayed());
	
        SgTimer timer;
	Solver solver;
	Solver::Result result;
	Solver::SolutionSet solution;

	timer.Start();
	result = solver.solve(*bd, color, solution, 
			      Solver::NO_DEPTH_LIMIT, timelimit);
        timer.Stop();
      
        if (result == Solver::WIN && !solution.pv.empty()) {

	  hex::log << hex::info << "******* FOUND WIN!!! ******" 
		   << hex::endl 
		   << "PV: " << HexPointUtil::ToPointListString(solution.pv)
		   << hex::endl
		   << "Elapsed: " << timer.GetTime()
		   << hex::endl;
	  return solution.pv[0];

        } else if (result == Solver::LOSS) {

	  hex::log << hex::info << "** Found loss!! **" << hex::endl;

	}

	hex::log << hex::info << "No win found." << hex::endl;
        time_remaining -= timer.GetTime();
    }

    return m_player->pre_search(brd, game_state, color, consider,
				time_remaining, score);
}

//----------------------------------------------------------------------------
