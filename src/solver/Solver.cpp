//----------------------------------------------------------------------------
// $Id: Solver.cpp 1715 2008-10-28 22:13:37Z broderic $
//----------------------------------------------------------------------------

/** @file 
    
    @todo Generalize to arbitrary group solver. 
          
*/

#include "Hex.hpp"
#include "HexProp.hpp"
#include "HexBoard.hpp"
#include "GraphUtils.hpp"
#include "Resistance.hpp"
#include "Solver.hpp"
#include "Time.hpp"
#include "BoardUtils.hpp"
#include "BitsetIterator.hpp"
#include "PlayerUtils.hpp"

#include <cmath>
#include <algorithm>
#include <boost/scoped_ptr.hpp>

//----------------------------------------------------------------------------

/** Performs various proof-checking diagnostics. */
#define VERIFY_PROOF_INTEGRITY    1

/** Output data each time we shrink a proof. */
#define OUTPUT_PROOF_SHRINKINGS   1

/** Display TT hits. */
#define OUTPUT_TT_HITS            1

/** Output extra debugging info to log if true. */
#define VERBOSE_LOG_MESSAGES      0

unsigned g_last_histogram_dump;

//----------------------------------------------------------------------------

Solver::Solver()
    : m_tt(0),
      m_db(0) 
{
    hex::log << hex::fine << "--- Solver" << hex::endl;
}

Solver::~Solver()
{
}

//----------------------------------------------------------------------------

void Solver::Initialize(const HexBoard& brd)
{
    g_last_histogram_dump = 0;

    m_aborted = false;

    m_start_time = HexGetTime();

    m_histogram = Histogram();
    m_statistics = GlobalStatistics();
    m_stoneboard.reset(new StoneBoard(brd));

    // save old settings, and set our own.
    hex::settings.Push();
    hex::settings.put_bool("vc-and-over-edge", false);
    hex::settings.put_int("vc-default-max-ors", 3);

    // Create new TT if one doesn't already exist.
    if (hex::settings.get_bool("solver-use-tt") && m_tt.get()==0) 
    {
        int bits = hex::settings.get_int("solver-tt-size");
        m_tt.reset( new TransTable<SolvedState>(bits) );
    }
}

void Solver::Cleanup()
{
    // restore old settings
    hex::settings.Pop();

    m_stoneboard.reset();
    if (m_db && m_delete_db_when_done) 
        delete m_db;
}

Solver::Result Solver::solve(HexBoard& brd, HexColor tomove, 
                             SolutionSet& solution,
                             int depth_limit, 
                             double time_limit)
{
    m_settings.use_db = false;
    m_settings.depth_limit = depth_limit;
    m_settings.time_limit = time_limit;

    m_db = 0;

    Initialize(brd);

    return run_solver(brd, tomove, solution);
}

Solver::Result Solver::solve(HexBoard& brd, HexColor tomove, 
                             SolverDB& db, 
                             SolutionSet& solution,
                             int depth_limit, 
                             double time_limit)
{
    m_settings.use_db = true;
    m_settings.depth_limit = depth_limit;
    m_settings.time_limit = time_limit;

    m_db = &db;
    m_delete_db_when_done = false;

    Initialize(brd);

    return run_solver(brd, tomove, solution);    

}

Solver::Result Solver::solve(HexBoard& brd, HexColor tomove, 
                             const std::string& filename, 
                             int numstones, int transtones, 
                             SolutionSet& solution,
                             int depth_limit, 
                             double time_limit)
{
    m_settings.use_db = true;
    m_settings.depth_limit = depth_limit;
    m_settings.time_limit = time_limit;

    m_db = new SolverDB();
    if (!m_db->open(brd.width(), brd.height(), numstones, 
                    transtones, filename))
    {
        hex::log << hex::warning << "Could not open db '" 
                 << filename << "'. Solver run aborted." << hex::endl;

        return UNKNOWN;
    }
    m_delete_db_when_done = true;

    Initialize(brd);

    return run_solver(brd, tomove, solution);
}

Solver::Result Solver::run_solver(HexBoard& brd, HexColor tomove,
                                  SolutionSet& solution)
{
    // Solver currently cannot handle permanently inferior cells.
    HexAssert(!hex::settings.get_bool("ice-find-permanently-inferior"));

    // Compute VCs/IC info for this state.
    brd.ComputeAll(tomove, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);

    // Solve it!
    m_completed.resize(100);
    MoveSequence variation;
    bool win = solve_state(brd, tomove, variation, solution);

    // AND the proof with empty cells on board since our working proof
    // contains played stones.
    solution.proof &= brd.getEmpty();

    m_end_time = HexGetTime();

    Cleanup();

    if (m_aborted) return Solver::UNKNOWN;

    return (win) ? Solver::WIN : Solver::LOSS;
}

//----------------------------------------------------------------------------

bitset_t 
Solver::DefaultProofForWinner(const HexBoard& brd, HexColor winner) const
{
    return (brd.getColor(winner) | brd.getEmpty()) - brd.getDead();
}

bool Solver::CheckDB(const HexBoard& brd, HexColor toplay, 
                     SolvedState& state) const
{
    if (m_settings.use_db && m_db->get(*m_stoneboard, state)) {
        hex::log << hex::fine << "DB[" << m_stoneboard->numStones()
                 << "] hit: "<< ((state.win)?"Win":"Loss") << ", "
                 << state.numstates << " states."
                 << hex::endl;

        HexColor winner = (state.win) ? toplay : !toplay;
        state.proof = DefaultProofForWinner(brd, winner);

        m_histogram.tthits[m_stoneboard->numStones()]++;
        return true;
    }
    return false;
}

bool Solver::CheckTT(const HexBoard& brd, HexColor toplay, 
                     SolvedState& state) const
{
    if (hex::settings.get_bool("solver-use-tt") 
        && m_tt->get(brd.Hash(), state)) 
    {
    
#if OUTPUT_TT_HITS
        hex::log << hex::fine
                 << "TT [" << state.numstones << "] "
                 << HashUtil::toString(state.hash) << " "
                 << state.numstates << " " 
                 << ((state.win) ? "Win" : "Loss")
                 << brd
                 << hex::endl;
#endif

#if CHECK_HASH_COLLISION
        state.CheckCollision(brd.Hash(), 
                             m_stoneboard->getBlack(), 
                             m_stoneboard->getWhite());
#endif

        HexColor winner = (state.win) ? toplay : !toplay;
        state.proof = DefaultProofForWinner(brd, winner);
  
        m_histogram.tthits[m_stoneboard->numStones()]++;
        return true;
    }
    return false;
}

bool Solver::CheckTransposition(const HexBoard& brd, HexColor toplay, 
                                SolvedState& state) const
{
    if (m_settings.use_db && m_stoneboard->numStones() <= m_db->maxstones())
        return CheckDB(brd, toplay, state);

    return CheckTT(brd, toplay, state);    
}

void Solver::StoreInDB(const SolvedState& state)
{
    if (!m_settings.use_db)
        return;

    int numstones = m_stoneboard->numStones();
    int numwritten = m_db->put(*m_stoneboard, state);
    if (numwritten && numstones == m_db->maxstones()) {
        hex::log << hex::info << "Stored DB[" << numstones << "] result: "
                 << m_stoneboard->printBitset(state.proof & 
                                              m_stoneboard->getEmpty())
                 << hex::endl
                 << ((state.win)?"Win":"Loss") << ", " 
                 << state.numstates << " states."
                 << hex::endl
                 << "Wrote " << numwritten << " transpositions."
                 << hex::endl
                 << "====================" 
                 << hex::endl;
    }
}

void Solver::StoreInTT(const SolvedState& state)
{
    if (!hex::settings.get_bool("solver-use-tt"))
        return;

    hex::log << hex::fine
             << "Storing proof in " << HashUtil::toString(state.hash) 
             << "(win " << state.win << ")"             
             << m_stoneboard->printBitset(state.proof) << hex::endl;

    m_tt->put(state);
}

void Solver::StoreState(const SolvedState& state) 
{
    if (m_settings.use_db && m_stoneboard->numStones() <= m_db->maxstones()) {
        StoreInDB(state);
    } else {
        StoreInTT(state);
    }
}

void Solver::ClearTT()
{
    if (m_tt && hex::settings.get_bool("solver-use-tt")) {
        m_tt->clear();
    }
}

//----------------------------------------------------------------------------

bool Solver::HandleTerminalNode(const HexBoard& brd, HexColor color,
                                SolvedState& state) const
{
    bitset_t proof;
    int numstones = m_stoneboard->numStones();

    if (SolverUtil::isWinningState(brd, color, proof)) {

        state.win = true;
        state.nummoves = 0;
        state.numstates = 1;
        state.proof = proof;
        m_histogram.terminal[numstones]++;
        return true;

    } else if (SolverUtil::isLosingState(brd, color, proof)) {

        state.win = false;
        state.nummoves = 0;
        state.numstates = 1;
        state.proof = proof;
        m_histogram.terminal[numstones]++;
        return true;

    } 
    return false;
}

bool Solver::HandleLeafNode(const HexBoard& brd, HexColor color, 
                            SolvedState& state, bool root_node) const
{
    if (HandleTerminalNode(brd, color, state))
        return true;

    // skip the transposition check if the flag is set and we are at
    // the root.
    if (root_node && m_settings.flags & SOLVE_ROOT_AGAIN)
        return false;

    return CheckTransposition(brd, color, state);
}

//----------------------------------------------------------------------------

bool Solver::solve_state(HexBoard& brd, HexColor color, 
                         MoveSequence& variation,
                         SolutionSet& solution)
{
    // Handle timelimit
    if (m_settings.time_limit > 0) {
        double elapsed_time = HexGetTime() - m_start_time;
        if (elapsed_time > m_settings.time_limit) {
            m_aborted = true;
            return false;
        }
    }

    // Check for VC/DB/TT states
    {
        SolvedState state;
        if (HandleLeafNode(brd, color, state, variation.empty())) {
            
            solution.stats.explored_states = 1;
            solution.stats.minimal_explored = 1;
            solution.stats.total_states += state.numstates;
            
            solution.pv.clear();
            solution.moves_to_connection = state.nummoves;
            solution.proof = state.proof;
            
            return state.win;
        }
    }

    // Solve decompositions if they exist, otherwise solve the state
    // normally.
    bool winning_state = false;
    {
        HexPoint group;
        bitset_t captured;
        if (hex::settings.get_bool("solver-use-decompositions") 
            && BoardUtils::FindSplittingDecomposition(brd, !color, group, 
                                                      captured))
        {
            winning_state = solve_decomposition(brd, color, variation, 
                                                solution, group);
        } else {
            winning_state = solve_interior_state(brd, color, variation, 
                                                 solution);
        }
    }

    // Shrink, verify, and store proof in DB/TT.
    handle_proof(brd, color, variation, winning_state, solution);

    // Dump histogram every 1M moves
    if ((m_statistics.played / 1000000) > (g_last_histogram_dump)) {
        hex::log << hex::info << m_histogram.Dump() << hex::endl;
        g_last_histogram_dump = m_statistics.played / 1000000;
    }

    return winning_state;
}

bool Solver::solve_decomposition(HexBoard& brd, HexColor color, 
                                 MoveSequence& variation,
                                 SolutionSet& solution,
                                 HexPoint group)
{
    solution.stats.decompositions++;

    hex::log << hex::fine;
    hex::log << "FOUND DECOMPOSITION FOR " << !color << hex::endl;
    hex::log << "Group: "<< group << hex::endl;
    hex::log << brd << hex::endl;

    // compute the carriers for each side 
    PointToBitset nbs;
    brd.ComputeDigraph(!color, nbs);
    bitset_t stopset = nbs[group];

    bitset_t carrier[2];
    carrier[0] = 
        GraphUtils::BFS(HexPointUtil::colorEdge1(!color), nbs, stopset);
    carrier[1] = 
        GraphUtils::BFS(HexPointUtil::colorEdge2(!color), nbs, stopset);

    if ((carrier[0] & carrier[1]).any()) {
        hex::log << "Side0:" << brd.printBitset(carrier[0]) << hex::endl;
        hex::log << "Side1:" << brd.printBitset(carrier[1]) << hex::endl;
        HexAssert(false);
    }
        
    // solve each side
    SolvedState state;
    SolutionSet dsolution[2];
    for (int s=0; s<2; ++s) {
        hex::log << hex::fine << "----------- Side" << s << ":" 
                 << brd.printBitset(carrier[s]) << hex::endl;

        bool win = false;
        brd.PlayStones(!color, carrier[s^1] & brd.getCells(), color);

        // check if new stones caused terminal state; if not, solve it
        if (HandleTerminalNode(brd, color, state)) {
            win = state.win;

            dsolution[s].stats.expanded_states = 0;
            dsolution[s].stats.explored_states = 1;
            dsolution[s].stats.minimal_explored = 1;
            dsolution[s].stats.total_states = 1;

            dsolution[s].proof = state.proof;
            dsolution[s].moves_to_connection = state.nummoves;
            dsolution[s].pv.clear();

        } else {

            win = solve_interior_state(brd, color, variation, dsolution[s]);

        }
        brd.UndoMove();
        
        // abort if we won this side
        if (win) {
            hex::log << hex::fine;
            hex::log << "##### WON SIDE " << s << " #####" << hex::endl;
            hex::log << brd.printBitset(dsolution[s].proof) << hex::endl;
            hex::log << "explored_states: " 
                     << dsolution[s].stats.explored_states << hex::endl;
            
            solution.pv = dsolution[s].pv;
            solution.proof = dsolution[s].proof;
            solution.moves_to_connection = dsolution[s].moves_to_connection;
            solution.stats += dsolution[s].stats;
            solution.stats.decompositions_won += 
                dsolution[s].stats.decompositions_won + 1;
            return true;
        } 
    }
        
    // combine the two losing proofs
    solution.pv = dsolution[0].pv;
    solution.pv.insert(solution.pv.end(), 
                       dsolution[1].pv.begin(), 
                       dsolution[1].pv.end());

    solution.moves_to_connection = 
        dsolution[0].moves_to_connection + 
        dsolution[1].moves_to_connection;
    
    solution.proof = 
        (dsolution[0].proof & carrier[0]) | 
        (dsolution[1].proof & carrier[1]) |
        brd.getColor(!color);
    solution.proof = solution.proof - brd.getDead();

    int s0 = (int)dsolution[0].stats.explored_states;
    int s1 = (int)dsolution[1].stats.explored_states;
    
    hex::log << hex::fine;
    hex::log << "##### LOST BOTH SIDES! #####" << hex::endl;
    hex::log << "Side0: " << s0 << " explored." << hex::endl;
    hex::log << "Side1: " << s1 << " explored." << hex::endl;
    hex::log << "Saved: " << (s0*s1) - (s0+s1) << hex::endl;
    hex::log << brd.printBitset(solution.proof) << hex::endl;

    return false;
}

//--------------------------------------------------------------------------
// Internal state
//--------------------------------------------------------------------------
bool Solver::solve_interior_state(HexBoard& brd, HexColor color, 
                                  MoveSequence& variation,
                                  SolutionSet& solution)
{
    int depth = variation.size();
    std::string space(2*depth, ' ');
    int numstones = m_stoneboard->numStones();

    // Print some output for debugging/tracing purposes
    hex::log << hex::fine;
    hex::log << SolverUtil::PrintVariation(variation) << hex::endl;
    hex::log << brd << hex::endl;

    //
    // Set initial proof for this state to be the union of all
    // opponent winning semis.  We need to do this because we use the
    // semis to restrict the search (ie, the mustplay).
    // Proof will also include all opponent stones. 
    //
    // Basically, we are assuming the opponent will win from this state;
    // if we win instead, we use the proof generated from that state,
    // not this one. 
    //
    solution.proof = SolverUtil::InitialProof(brd, color);

    // Get the moves to consider
    bitset_t mustplay = SolverUtil::MovesToConsider(brd, color, solution.proof);
    hex::log << hex::fine << "mustplay: [" 
             << HexPointUtil::ToPointListString(mustplay) 
             << " ]" << hex::endl;

    if (depth == hex::settings.get_int("solver-update-depth")) {
        hex::log << hex::info;
        hex::log << "Solving position:" << hex::endl;
        hex::log << *m_stoneboard << hex::endl;

        // output progress for the gui
        if (hex::settings.get_bool("solver-use-guifx"))
        {
            std::ostringstream os;
            os << "gogui-gfx:\n";
            os << "solver\n";
            os << "VAR";
            HexColor toplay = (variation.size()&1) ? !color : color;
            for (std::size_t i=0; i<variation.size(); ++i) 
            {
                os << " " << ((toplay == BLACK) ? "B" : "W");
                os << " " << variation[i];
                toplay = !toplay;
            }
            os << "\n";
            os << "LABEL ";
            const InferiorCells& inf = brd.getInferiorCells();
            os << inf.GuiOutput();
            os << BoardUtils::GuiDumpOutsideConsiderSet(brd, mustplay, 
                                                        inf.All());
            os << "\n";
            os << "TEXT";
            for (int i=0; i<depth; ++i) 
            {
                os << " " << m_completed[i].first 
                   << "/" << m_completed[i].second;
            }
            os << "\n";
            os << "\n";
            std::cout << os.str();
            std::cout.flush();
        }
    } 

    // If mustplay is empty then this is a losing state.
    if (mustplay.none()) {
        hex::log << hex::fine << "Empty reduced mustplay." << hex::endl
                 << brd.printBitset(solution.proof) << hex::endl;

        m_histogram.terminal[numstones]++;

        solution.stats.total_states = 1;
        solution.stats.explored_states = 1;
        solution.stats.minimal_explored = 1;

        solution.result = LOSS;
        solution.pv.clear();
        solution.moves_to_connection = 0;

        return false;  
    }

    bitset_t original_mustplay = mustplay;

    // Clear the transposition table so we can reproduce the
    // proof under this db state at a later time.
    if (m_settings.use_db 
        && hex::settings.get_bool("solver-clear-tt-for-db-state")
        && numstones == m_db->maxstones())
    {
        ClearTT();
    }
     
    solution.stats.total_states = 1;
    solution.stats.explored_states = 1;
    solution.stats.minimal_explored = 1;
    solution.stats.expanded_states = 1;
    solution.stats.moves_to_consider = mustplay.count();
    m_histogram.states[numstones]++;

    // 
    // Order moves in the mustplay.
    //
    // @note If we want to find all winning moves then 
    // we need to stop OrderMoves() from aborting on a win.
    //
    // @note OrderMoves() will handle VC/DB/TT hits, and remove them
    // from consideration.  It is possible that there are no moves, in
    // which case we fall through the loop below with no problem (the
    // state is a loss).
    //
    solution.moves_to_connection = -1;
    std::vector<HexMoveValue> moves;
    bool winning_state = OrderMoves(brd, color, mustplay, 
                                    solution, moves);

    //----------------------------------------------------------------------
    // Expand all moves in mustplay that were not leaf states.
    //----------------------------------------------------------------------
    u64 states_under_losing = 0;
    bool made_it_through = false;

    for (unsigned index=0; 
         !winning_state && index<moves.size(); 
         ++index) 
    {
        HexPoint cell = moves[index].point();
        
        // Output a rough progress indicator as an 'info' level log message.
        if (depth < hex::settings.get_int("solver-progress-depth")) {
            hex::log << hex::info << space
                     << (index+1) << "/" << moves.size()
                     << ": (" << color
                     << ", " << cell << ")"
                     << " " << m_statistics.played 
                     << " " << FormattedTime(HexGetTime() - m_start_time);

            if (!mustplay.test(cell))
                hex::log << " " << "*pruned*";

            hex::log << hex::endl;
        }

        // note the level of completion
        m_completed[depth] = std::make_pair(index, moves.size());

        // skip moves that were pruned by the proofs of previous moves
        if (!mustplay.test(cell)) {
            solution.stats.pruned++;
            continue;
        }

	made_it_through = true;
        SolutionSet child_solution;
        PlayMove(brd, cell, color);
        variation.push_back(cell);

        bool win = !solve_state(brd, !color, variation, child_solution);

        variation.pop_back();
        UndoMove(brd, cell);

        solution.stats += child_solution.stats;

        if (win) {

            // Win: copy proof over, copy pv, abort!
            winning_state = true;
            solution.proof = child_solution.proof;

            solution.pv.clear();
            solution.pv.push_back(cell);
            solution.pv.insert(solution.pv.end(), child_solution.pv.begin(), 
                               child_solution.pv.end());

            solution.moves_to_connection = 
                child_solution.moves_to_connection + 1;

            // set minimal tree-size explicitly to be child's minimal size
            // plus 1.
            solution.stats.minimal_explored = 
                child_solution.stats.minimal_explored + 1;

            solution.stats.winning_expanded++;
            solution.stats.branches_to_win += index+1;

            m_histogram.winning[numstones]++;
            m_histogram.size_of_winning_states[numstones] 
                += child_solution.stats.explored_states;

            m_histogram.branches[numstones] += index+1;
            m_histogram.states_under_losing[numstones] += states_under_losing;
            m_histogram.mustplay[numstones] += original_mustplay.count();

	    if (solution.moves_to_connection == -1) {
		hex::log << hex::info 
			 << "child_solution.moves_to_connection == " 
			 << child_solution.moves_to_connection << hex::endl;
	    }
	    HexAssert(solution.moves_to_connection != -1);	    

        } else {

            // Loss: add returned proof to current proof. Prune
            // mustplay by proof.  Maintain PV to longest loss.

            mustplay &= child_solution.proof;
            solution.proof |= child_solution.proof;
            states_under_losing += child_solution.stats.explored_states;

            m_histogram.size_of_losing_states[numstones] 
                += child_solution.stats.explored_states;

            if (child_solution.moves_to_connection + 1 > 
                solution.moves_to_connection) 
            {
                solution.moves_to_connection = 
                    child_solution.moves_to_connection + 1;

                solution.pv.clear();
                solution.pv.push_back(cell);
                solution.pv.insert(solution.pv.end(), 
                                   child_solution.pv.begin(), 
                                   child_solution.pv.end());
            }
	    if (solution.moves_to_connection == -1) {
		hex::log << hex::info 
			 << "child_solution.moves_to_connection == " 
			 << child_solution.moves_to_connection << hex::endl;
	    }
	    HexAssert(solution.moves_to_connection != -1);
        }
    }

    if (solution.moves_to_connection == -1) {
	hex::log << hex::info << "moves_to_connection == -1 and "
		 << "made_it_through = " << made_it_through << hex::endl;
    }
    HexAssert(solution.moves_to_connection != -1);

    return winning_state;
}

void Solver::handle_proof(const HexBoard& brd, HexColor color, 
                          const MoveSequence& variation,
                          bool winning_state, 
                          SolutionSet& solution)
{
    // do nothing if we aborted the search
    if (m_aborted) return;

    HexColor winner = (winning_state) ? color : !color;
    HexColor loser = !winner;

    // Verify loser's stones do not intersect proof
    if ((brd.getColor(loser) & solution.proof).any()) {
        hex::log << hex::warning;
        hex::log << color << " to play." << hex::endl;
        hex::log << loser << " loses." << hex::endl;
        hex::log << "Losing stones hit proof:" << hex::endl;
        hex::log << brd.printBitset(solution.proof) << hex::endl;
        hex::log << brd << hex::endl;
        hex::log << SolverUtil::PrintVariation(variation) << hex::endl;
        HexAssert(false);
    }

    // Verify dead cells do not intersect proof
    if ((brd.getDead() & solution.proof).any()) {
        hex::log << hex::warning;
        hex::log << color << " to play." << hex::endl;
        hex::log << loser << " loses." << hex::endl;
        hex::log << "Dead cells hit proof:" << hex::endl;
        hex::log << brd.printBitset(solution.proof) << hex::endl;
        hex::log << brd << hex::endl;
        hex::log << SolverUtil::PrintVariation(variation) << hex::endl;
        HexAssert(false);
    }

    // Shrink proof.
    bitset_t old_proof = solution.proof;
    if (hex::settings.get_bool("solver-shrink-proof")) {
        SolverUtil::ShrinkProof(solution.proof, *m_stoneboard, loser, brd.ICE());
        
        bitset_t pruned;
        pruned  = BoardUtils::ReachableOnBitset(brd, solution.proof, 
                                 EMPTY_BITSET,
                                 HexPointUtil::colorEdge1(winner));
        pruned &= BoardUtils::ReachableOnBitset(brd, solution.proof, 
                                 EMPTY_BITSET,
                                 HexPointUtil::colorEdge2(winner));
        solution.proof = pruned;

        if (solution.proof.count() < old_proof.count()) {
            solution.stats.shrunk++;
            solution.stats.cells_removed 
                += old_proof.count() - solution.proof.count();
        }
    }
    
#if VERIFY_PROOF_INTEGRITY
    // Verify proof touches both of winner's edges.
    if (!BoardUtils::ConnectedOnBitset(brd, solution.proof, 
                                       HexPointUtil::colorEdge1(winner),
                                       HexPointUtil::colorEdge2(winner)))
    {
        hex::log << hex::warning;
        hex::log << "Proof does not touch both edges!" << hex::endl;
        hex::log << brd.printBitset(solution.proof) << hex::endl;
        
        hex::log << "Original proof:" << hex::endl;
        hex::log << brd.printBitset(old_proof) << hex::endl;
        hex::log << brd << hex::endl;
        hex::log << color << " to play." << hex::endl;
    
        hex::log << SolverUtil::PrintVariation(variation) << hex::endl;
        
        abort();
    }
#endif    

    // Store move in DB/TT
    bitset_t winners_stones = 
        m_stoneboard->getColor(winner) & solution.proof;

    StoreState(SolvedState(m_stoneboard->numStones(), brd.Hash(), 
                           winning_state, solution.stats.total_states, 
                           solution.moves_to_connection, 
                           solution.proof, winners_stones, 
                           m_stoneboard->getBlack(), 
                           m_stoneboard->getWhite()));
}

//----------------------------------------------------------------------------

void Solver::PlayMove(HexBoard& brd, HexPoint cell, HexColor color) 
{
    m_statistics.played++;
    m_stoneboard->playMove(color, cell);
    brd.PlayMove(color, cell);
}

void Solver::UndoMove(HexBoard& brd, HexPoint cell)
{
    m_stoneboard->undoMove(cell);
    brd.UndoMove();
}

//----------------------------------------------------------------------------

bool Solver::OrderMoves(HexBoard& brd, HexColor color, bitset_t& mustplay, 
                        SolutionSet& solution,
                        std::vector<HexMoveValue>& moves)
{        
    hex::log << hex::fine;
    hex::log << "OrderMoves" << hex::endl;

    moves.clear();

    boost::scoped_ptr<Resistance> resist;
    bool with_ordering = hex::settings.get_bool("solver-use-move-ordering");
    bool with_resist = hex::settings.get_bool("solver-order-with-resist");
    bool with_center = hex::settings.get_bool("solver-order-from-center");
    bool with_mustplay = hex::settings.get_bool("solver-order-with-mustplay");
    HexColor other = !color;

    // intersection of proofs for all losing moves
    bitset_t proof_intersection;
    proof_intersection.set();

    // union of proofs for all losing moves
    bitset_t proof_union;
    
    if (with_resist && with_ordering) {
        resist.reset(new Resistance());
        resist->Evaluate(brd);
    }
    
    /** @todo The TT/DB checks should be done as a single 1-ply sweep
        that only modifies the board hash, since computing the VCs for
        these states is pointless.
    */

    bool found_win = false;
    for (BitsetIterator it(mustplay); !found_win && it; ++it) {
        bool skip_this_move = false;
        double score = 0.0;

        if (!mustplay.test(*it)) 
            continue;

        if (with_ordering) {
            double mustplay_size = 0.0;
            double fromcenter = 0.0;
            double rscore = 0.0;
            double tiebreaker = 0.0;
            bool exact_score = false;
            bool winning_semi_exists = false;

            // 
            // Do mustplay move-ordering.  This entails playing each
            // move, computing the vcs, storing the mustplay size,
            // then undoing the move. This gives pretty good move
            // ordering: 7x7 is not solvable without this method.
            // However, it is very expensive!
            // 
            // We try to reduce the number of PlayMove/UndoMove pairs
            // we perform by checking the VC/DB/TT here, instead of in
            // solve_state().  Any move leading to a VC/DB/TT hit is removed
            // from the mustplay and handled as it would be in
            // solve_state().
            //
            if (with_mustplay) {

                PlayMove(brd, *it, color);

                SolvedState state;
                if (HandleLeafNode(brd, other, state, false)) {
                    exact_score = true;

                    solution.stats.explored_states += 1;
                    solution.stats.minimal_explored++;
                    solution.stats.total_states += state.numstates;

                    if (!state.win) {

                        found_win = true;
                        moves.clear();
                        
                        // this state plus the child winning state
                        // (which is a leaf).
                        solution.stats.minimal_explored = 2;

                        solution.proof = state.proof;
                        solution.moves_to_connection = state.nummoves+1;
                        solution.pv.clear();
                        solution.pv.push_back(*it);
                        
                    } else {

                        skip_this_move = true;
                        if (state.nummoves+1 > solution.moves_to_connection) {
                            solution.moves_to_connection = state.nummoves+1;
                            solution.pv.clear();
                            solution.pv.push_back(*it);
                        }

                        // prune the mustplay with the proof
                        proof_intersection &= state.proof;
                        proof_union |= state.proof;
                    }

                } else {
                   
                    // Not a leaf node. 
                    // Do we force a mustplay on our opponent?
                    HexPoint edge1 = HexPointUtil::colorEdge1(color);
                    HexPoint edge2 = HexPointUtil::colorEdge2(color);
                    if (brd.Cons(color).doesVCExist(edge1, edge2, VC::SEMI)) {
                        winning_semi_exists = true;
                    }
                    bitset_t mp = brd.getMustplay(other);
                    mustplay_size = mp.count();
                } 
                
                UndoMove(brd, *it);

            } // end of mustplay move ordering

            // Perform move ordering 
            if (!exact_score) {

                if (with_center) {
                    fromcenter += SolverUtil::DistanceFromCenter(brd, *it);
                }

                if (with_resist) {
                    rscore = resist->Score(*it);
                    HexAssert(rscore < 100.0);
                }
                
                tiebreaker = (with_resist) ? 100.0 - rscore : fromcenter;
                
                if (winning_semi_exists) {
                    score = 1000.0*mustplay_size + tiebreaker;
                } else {
                    score = 1000000.0*tiebreaker;
                }
            }
        }

        if (!skip_this_move) 
            moves.push_back(HexMoveValue(*it, score));

    }

    /** @note 'sort' is not stable, so multiple runs can produce
        different move orders in the same state unless stable_sort is
        used. */
    stable_sort(moves.begin(), moves.end());
    HexAssert(!found_win || moves.size()==1);

    // for a win: nothing to do
    if (found_win) {
        hex::log << hex::fine
                 << "Found winning move; aborted ordering." 
                 << hex::endl;
    } 

    // for a loss: recompute mustplay because backed-up ice info
    // could shrink it.  Then prune with the intersection of all 
    // losing proofs, and add in the union of all losing proofs
    // to the current proof. 
    else {

        if (hex::settings.get_bool("solver-backup-ice-info")) {
            bitset_t new_initial_proof = SolverUtil::InitialProof(brd, color);
            bitset_t new_mustplay = 
                SolverUtil::MovesToConsider(brd, color, new_initial_proof);
            HexAssert(BitsetUtil::IsSubsetOf(new_mustplay, mustplay));
            
            if (new_mustplay.count() < mustplay.count()) {
                
                hex::log << hex::fine
                         << "Pruned mustplay with backing-up info."
                         << brd.printBitset(mustplay)
                         << brd.printBitset(new_mustplay) << hex::endl;

                mustplay = new_mustplay;
                solution.proof = new_initial_proof;
            }
        }

        mustplay &= proof_intersection;
        solution.proof |= proof_union;
    }

#if VERBOSE_LOG_MESSAGES
    hex::log << hex::fine;
    hex::log << "Ordered list of moves: " << hex::endl;
    for (unsigned i=0; i<moves.size(); i++) {
        hex::log << " [" << moves[i].point();
        hex::log << ", " << moves[i].value() << "]";
    }
    hex::log << hex::endl;
#endif

    return found_win;
}

//----------------------------------------------------------------------------

// Stats output

std::string Solver::Histogram::Dump()
{
    std::ostringstream os;
    os << std::endl;
    os << "Histogram" << std::endl;
    os << "                         States             ";
    os << "                      Branch Info                    ";
    os << "                                      TT/DB                ";
    os << std::endl;
    os << std::setw(3) << "#" << " "
       << std::setw(12) << "Terminal"
       << std::setw(12) << "Internal"
       << std::setw(12) << "Int. Win"
       << std::setw(12) << "Win Pct"

       << std::setw(12) << "Sz Winning"
       << std::setw(12) << "Sz Losing"
        
       << std::setw(12) << "To Win"
       << std::setw(12) << "Mustplay"
       << std::setw(12) << "U/Losing"
       << std::setw(12) << "Cost"
       << std::setw(12) << "Hits"
       << std::setw(12) << "Pct"
       << std::endl;

    for (int p=0; p<FIRST_INVALID; ++p) {
        if (!states[p] && !terminal[p]) 
            continue;

        double moves_to_find_winning = winning[p] ?
            (double)branches[p]/winning[p] : 0;
        
        double avg_states_under_losing = (branches[p]-winning[p])?
            ((double)states_under_losing[p]/(branches[p]-winning[p])):0;

        os << std::setw(3) << p << ":"
            
           << std::setw(12) << terminal[p] 

           << std::setw(12) << states[p]

           << std::setw(12) << winning[p]
            
           << std::setw(12) << std::fixed << std::setprecision(3) 
           << ((states[p])?((double)winning[p]*100.0/states[p]):0)

           << std::setw(12) << std::fixed << std::setprecision(1) 
           << ((winning[p])?((double)size_of_winning_states[p]/winning[p]):0)

           << std::setw(12) << std::fixed << std::setprecision(1) 
           << ((states[p]-winning[p])?((double)(size_of_losing_states[p]/(states[p] - winning[p]))):0)

           << std::setw(12) << std::fixed << std::setprecision(4)
           << moves_to_find_winning
            
           << std::setw(12) << std::fixed << std::setprecision(2)
           << ((winning[p])?((double)mustplay[p]/winning[p]):0)
            
           << std::setw(12) << std::fixed << std::setprecision(1)
           << avg_states_under_losing

           << std::setw(12) << std::fixed << std::setprecision(1)
           << fabs((moves_to_find_winning-1.0)*avg_states_under_losing*winning[p])

           << std::setw(12) << tthits[p]
            
           << std::setw(12) << std::fixed << std::setprecision(3)
           << ((states[p])?((double)tthits[p]*100.0/states[p]):0)
            
           << std::endl;
    }
    return os.str();
}

void Solver::DumpStats(const SolutionSet& solution) const
{
    double total_time = m_end_time - m_start_time;

    hex::log << hex::info << "\n"
             << "########################################\n"
             << "         Played: " << m_statistics.played << "\n"
             << "         Pruned: " << solution.stats.pruned << "\n"
             << "   Total States: " << solution.stats.total_states << "\n"
             << "Explored States: " << solution.stats.explored_states 
             << " (" << solution.stats.minimal_explored << ")" << "\n"
             << "Expanded States: " << solution.stats.expanded_states << "\n"
             << " Decompositions: " << solution.stats.decompositions << "\n"
             << "    Decomps won: " << solution.stats.decompositions_won << "\n"
             << "  Shrunk Proofs: " << solution.stats.shrunk << "\n"
             << "    Avg. Shrink: " 
             << ((double)solution.stats.cells_removed / solution.stats.shrunk) << "\n"
             << "  Branch Factor: " 
             << ((double)solution.stats.moves_to_consider/
                 solution.stats.expanded_states) << "\n"
             << "    To Find Win: "
             << ((double)solution.stats.branches_to_win/
                 solution.stats.winning_expanded) << "\n"
             << "########################################\n";

    if (m_settings.use_db && m_db) 
    {
        SolverDB::Statistics db_stats = m_db->stats();
        hex::log << "         DB Hit: " 
                 << db_stats.gets << " (" << db_stats.saved << ")" << "\n"
                 << "      DB Solved: " << db_stats.puts << "\n"
                 << "       DB Trans: " << db_stats.writes << "\n"
                 << "      DB Shrunk: " << db_stats.shrunk << "\n"
                 << "    Avg. Shrink: " 
                 << (double)db_stats.shrinkage/db_stats.shrunk << "\n"
                 << "########################################\n";
    }
   
    if (hex::settings.get_bool("solver-use-tt") && m_tt) 
    {
        hex::log << m_tt->stats();
        hex::log << "########################################\n";
    }
    
    hex::log << "States/sec: " << (solution.stats.explored_states/total_time) 
             << "\n"
             << "Played/sec: " << (m_statistics.played/total_time) << "\n"
             << "Total Time: " << FormattedTime(total_time) << "\n";
   
    //hex::log << ((win)?"Winning":"Losing") << " ";
    hex::log << "VC in " << solution.moves_to_connection << " moves" << "\n";
    hex::log << "PV:" << HexPointUtil::ToPointListString(solution.pv);
    hex::log << hex::endl;

    hex::log << hex::info << m_histogram.Dump() << hex::endl;
}

//----------------------------------------------------------------------------

// Debugging utilities

std::string SolverUtil::PrintVariation(const MoveSequence& variation)
{
    std::ostringstream os;
    os << "Variation: ";
    for (unsigned i=0; i<variation.size(); i++) 
        os << " " << variation[i];
    os << std::endl;
    return os.str();
}
 
// Move ordering utilities

int SolverUtil::DistanceFromCenter(const ConstBoard& brd, HexPoint cell)
{
    // Odd boards are easy
    if ((brd.width() & 1) && (brd.height() & 1))
        return brd.distance(brd.centerPoint(), cell);

    // Make sure we spiral nicely on boards with even
    // dimensions. Take the sum of the distance between
    // the two center cells on the main diagonal.
    return brd.distance(brd.centerPointRight(), cell) +
           brd.distance(brd.centerPointLeft(), cell);
}

// Misc. other utilities for playing moves, checking for lost states, etc. 

bool SolverUtil::isWinningState(const HexBoard& brd, HexColor color, 
                                bitset_t& proof)
{
    if (brd.isGameOver()) 
    {
        if (brd.getWinner() == color) 
        {
            // Surprisingly, this situation *can* happen: opponent plays a
            // move in the mustplay causing a sequence of presimplicial-pairs 
            // and captures that result in a win. 
            hex::log << hex::fine << "#### Solid chain win ####" << hex::endl;
            proof = brd.getColor(color) - brd.getDead();
            return true;
        }
    } 
    else 
    {
        VC v;
        if (brd.Cons(color).getSmallestVC(HexPointUtil::colorEdge1(color), 
                                          HexPointUtil::colorEdge2(color), 
                                          VC::SEMI, v)) 
        {
            hex::log << hex::fine << "VC win." << hex::endl;
            proof = (v.carrier() | brd.getColor(color)) - brd.getDead();
            return true;
        } 
    }
    return false;
}

bool SolverUtil::isLosingState(const HexBoard& brd, HexColor color, 
                               bitset_t& proof)
{
    HexColor other = !color;
    if (brd.isGameOver()) 
    {
        if (brd.getWinner() == other) 
        {
            // This occurs very rarely, but definetly cannot be ruled out.
            hex::log << hex::fine << "#### Solid chain loss ####" << hex::endl;
            proof = brd.getColor(other) - brd.getDead();
            return true;
        } 
    } 
    else 
    {
        VC vc;
        HexPoint otheredge1 = HexPointUtil::colorEdge1(other);
        HexPoint otheredge2 = HexPointUtil::colorEdge2(other);
        if (brd.Cons(other).getSmallestVC(otheredge1, otheredge2, 
                                          VC::FULL, vc)) 
        {
            hex::log << hex::fine << "VC loss." << hex::endl;
            proof = (vc.carrier() | brd.getColor(other)) - brd.getDead();
            return true;
        } 
    }
    return false;
}

// Moves to consider calculation

bitset_t SolverUtil::MovesToConsider(const HexBoard& brd, HexColor color,
                                     bitset_t& proof)
{
    bitset_t ret = brd.getMustplay(color);
    if (ret.none()) 
    {
        hex::log << hex::warning;
        hex::log << "EMPTY MUSTPLAY!" << hex::endl;
        hex::log << brd << hex::endl;
    }
    HexAssert(ret.any());
    
    const InferiorCells& inf = brd.getInferiorCells();

    // take out the dead, dominated, and vulnerable
    ret = ret - inf.Dead();
    ret = ret - inf.Dominated();
    ret = ret - inf.Vulnerable();
    
    /** Must add vulnerable killers (and their carriers) to proof.
        
        @todo Currently, we just add the first killer: we should see
        if any killer is already in the proof, since then we wouldn't
        need to add one.
    */
    for (BitsetIterator p(inf.Vulnerable()); p; ++p) 
    {
        const std::set<VulnerableKiller>& killers = inf.Killers(*p);
        proof.set((*killers.begin()).killer());
        proof |= ((*killers.begin()).carrier());
    }

    return ret;
}

// Utilities on proofs

bitset_t SolverUtil::MustplayCarrier(const HexBoard& brd, HexColor color)
{
    HexPoint edge1 = HexPointUtil::colorEdge1(!color);
    HexPoint edge2 = HexPointUtil::colorEdge2(!color);
    const VCList& lst = brd.Cons(!color).getVCList(edge1, edge2, VC::SEMI);
    return (hex::settings.get_bool("vc-use-greedy-union")) ?
        lst.getGreedyUnion():
        lst.getUnion();
}

bitset_t SolverUtil::InitialProof(const HexBoard& brd, HexColor color)
{
    hex::log << hex::fine;
    hex::log << "mustplay-carrier:" << hex::endl;
    hex::log << brd.printBitset(MustplayCarrier(brd, color)) << hex::endl;

    bitset_t proof = 
        (MustplayCarrier(brd, color) | brd.getColor(!color)) - brd.getDead();

    hex::log << hex::fine;
    hex::log << "Initial mustplay-carrier:" << hex::endl;
    hex::log << brd.printBitset(proof) << hex::endl;

    if ((proof & brd.getColor(color)).any()) 
    {
        hex::log << hex::severe
                 << "Initial mustplay hits toPlay's stones!" << hex::endl 
                 << brd << hex::endl 
                 << brd.printBitset(proof) << hex::endl;
        HexAssert(false);
    }
    
    return proof;
}

void SolverUtil::ShrinkProof(bitset_t& proof, 
                             const StoneBoard& board, HexColor loser, 
                             const ICEngine& ice)
{
    static boost::scoped_ptr<PatternBoard> brd;

    // never been here before or boardsize changed since last call
    if (!brd.get() || 
        (brd->width() != board.width() || brd->height() != board.height()))
    {
        brd.reset(new PatternBoard(board.width(), board.height()));
    }
    brd->startNewGame();

    // give loser all cells outside proof
    bitset_t cells_outside_proof = (~proof & brd->getCells());
    brd->addColor(loser, cells_outside_proof);

    // give winner only his stones inside proof; 
    HexColor winner = !loser;
    brd->addColor(winner, board.getColor(winner) & board.getPlayed() & proof);
    brd->update();
    brd->absorb();

    // compute fillin and remove captured cells from the proof
    InferiorCells inf;
    ice.computeFillin(loser, *brd, inf, HexColorSetUtil::Only(loser));
    HexAssert(inf.Captured(winner).none());

    bitset_t filled = inf.Dead() | inf.Captured(loser);
    bitset_t shrunk_proof = proof - filled;

#if OUTPUT_PROOF_SHRINKINGS
    if (shrunk_proof.count() < proof.count()) 
    {
        hex::log << hex::fine;
        hex::log << "**********************" << hex::endl;
        hex::log << loser << " loses on: ";
        hex::log << board << hex::endl;
        hex::log << "Original proof: ";
        hex::log << board.printBitset(proof) << hex::endl;
        hex::log << "Shrunk (removed " 
                 << (proof.count() - shrunk_proof.count()) << " cells):";
        hex::log << brd->printBitset(shrunk_proof) << hex::endl;
        hex::log << *brd << hex::endl;
        hex::log << "**********************" << hex::endl;
    }
#endif

    proof = shrunk_proof;
}

//----------------------------------------------------------------------------
