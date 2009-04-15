//----------------------------------------------------------------------------
// $Id: PerfectPlayer.cpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#include "SgSystem.h"

#include "PerfectPlayer.hpp"
#include "BitsetIterator.hpp"

//----------------------------------------------------------------------------

PerfectPlayer::PerfectPlayer(Solver* solver)
    : UofAPlayer(),
      m_solver(solver)
{
    hex::log << hex::fine << "--- PerfectPlayer" << hex::endl;
}

PerfectPlayer::~PerfectPlayer()
{
}

//----------------------------------------------------------------------------

HexPoint PerfectPlayer::search(HexBoard& brd, 
                               const Game& UNUSED(game_state),
			       HexColor color, 
                               const bitset_t& UNUSED(consider), 
                               double UNUSED(time_remaining), 
                               double& score)
{
    bool need_to_run_solver = true;

    bitset_t proof;
    HexPoint move_to_play = INVALID_POINT;

    // check db 
    if (find_db_move(brd, color, move_to_play, proof, score)) 
        need_to_run_solver = false;

    // state not currently in db; solve it. 
    if (need_to_run_solver) {
       
        solve_new_state(brd, color, move_to_play, proof, score);

    }

    hex::log << hex::info << brd.printBitset(proof) << hex::endl;
    if (HexEvalUtil::IsWin(score)) {
        hex::log << "Win in " << HexEvalUtil::PlyToWin(score) << "." 
                 << hex::endl;
    } else {
        hex::log << "Loss in " << HexEvalUtil::PlyToLoss(score) << "." 
                 << hex::endl;
    }

    return move_to_play;
}

bool PerfectPlayer::find_db_move(StoneBoard& brd, HexColor color, 
                                   HexPoint& move_to_play, bitset_t& proof, 
                                   double& score) const
{
    if (!hex::settings.get_bool("perfect-use-db"))
        return false;

    // try to open the db
    SolverDB db;
    std::string filename = hex::settings.get("perfect-db-filename");
    if (!db.open(brd.width(), brd.height(), filename)) {
        hex::log << hex::info << "perfect: db '" << filename << "' "
                 << "does not exist!" << hex::endl;
        return false;
    }

    // bail if state doesn't exist in db
    SolvedState root_state;
    if (!db.get(brd, root_state)) {
        hex::log << hex::info << "perfect: state not in db." << hex::endl;
        return false;
    }
    bool winning = root_state.win;
    proof = root_state.proof;

    hex::log << hex::info 
             << "perfect: state in db; finding best move..."
             << hex::endl;

    // check all children to find the shortest win/longest loss
    HexPoint best_win  = INVALID_POINT;
    HexPoint best_loss = INVALID_POINT;
    int win_length  = 999999;
    int loss_length =-999999;

    bool found_child = false;
    for (BitsetIterator p(brd.getEmpty()); p; ++p) {
        brd.playMove(color, *p);
        
        SolvedState state;
        if (db.get(brd, state)) {
            found_child = true;
            if (state.win) {
                if (state.nummoves > loss_length) {
                    best_loss = *p;
                    loss_length = state.nummoves;
                }
            } else {
                if (state.nummoves < win_length) {
                    best_win = *p;
                    win_length = state.nummoves;
                }
            }
        }
        
        brd.undoMove(*p);
    }
    
    // if no child (ie, a db leaf state), we have to solve it by hand
    if (!found_child) {
        hex::log << hex::info << "perfect: db leaf." << hex::endl;
        return false;
    }

    if (winning) {
        HexAssert(best_win != INVALID_POINT);
        move_to_play = best_win;
        score = IMMEDIATE_WIN - win_length;
    } else {
        HexAssert(best_loss != INVALID_POINT);
        move_to_play = best_loss;
        score = IMMEDIATE_LOSS + loss_length;
    }

    return true;
}

void PerfectPlayer::solve_new_state(HexBoard& brd, HexColor color, 
                                      HexPoint& move_to_play, bitset_t& proof, 
                                      double& score) const
{
    hex::log << hex::info 
             << "perfect: state not in db; solving from scratch." 
             << hex::endl;
        
    Solver::SolutionSet solution;
    HexEval result = Solver::UNKNOWN;
    bool solve_without_db = true;

    /** @todo Make this an option? */
    m_solver->ClearTT();
    
    // solve the state; try to use the db if option is on
    if (hex::settings.get_bool("perfect-use-db")) {
        std::string filename = hex::settings.get("perfect-db-filename");
        
        SolverDB db;
        if (db.open(brd.width(), brd.height(), filename)) {
            int flags = m_solver->GetFlags();
            m_solver->SetFlags(flags | Solver::SOLVE_ROOT_AGAIN);
            result = m_solver->solve(brd, color, db, solution);
            m_solver->SetFlags(flags);
            solve_without_db = false;
        } else {
            hex::log << hex::info << "perfect: db '" << filename << "' "
                     << "does not exist!" << hex::endl;
        }
    }

    if (solve_without_db) {
        hex::log << "perfect: solving state without db..." << hex::endl;
        result = m_solver->solve(brd, color, solution);
    }

    HexAssert(result != Solver::UNKNOWN);
        
    /** @todo Propagate these values up the tree. This is
        necessary because finding an alternate win can change the
        moves_to_connection value in parent states. This is
        somewhat cosmetic. */

    proof = solution.proof;
    HexAssert(!solution.pv.empty());
    move_to_play = solution.pv[0];
    
    if (solution.result) {
        score = IMMEDIATE_WIN - solution.moves_to_connection;
    } else {
        score = IMMEDIATE_LOSS + solution.moves_to_connection;
    }
}

//----------------------------------------------------------------------------
