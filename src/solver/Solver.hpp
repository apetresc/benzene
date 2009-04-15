//----------------------------------------------------------------------------
// $Id: Solver.hpp 1715 2008-10-28 22:13:37Z broderic $
//----------------------------------------------------------------------------

#ifndef SOLVER_H
#define SOLVER_H

#include <boost/scoped_ptr.hpp>

#include "Hex.hpp"
#include "HexBoard.hpp"
#include "VC.hpp"
#include "TransTable.hpp"
#include "SolvedState.hpp"
#include "SolverDB.hpp"
#include "HexEval.hpp"

//----------------------------------------------------------------------

/** 
    Determines the winner of a gamestate. 
    
    Solver uses a mustplay driven depth-first search to determine the
    winner in the given state.  A transposition table and a database
    of solved positions are also used to reduce the amount of work
    needed.
    
    Parameters
    
    @li "solver-use-decompositions": if true, in gamestates that can 
    be decomposed into separate components, each component is solved
    separately and the proofs are combined if necessary. 

    @li "solver-use-tt": flag storing whether to use a transposition
    table or not.
    
    @li "solver-tt-size": number of bits to use for indexing in the
    transposition table.  TT will hold 2^n states, where n is the
    number of bits.

    @li "solver-clear-tt-for-db-state": clears the tt

    @li "solver-use-move-ordering": flag storing whether to order
    moves at all.

    @li "solver-order-from-center": if true, moves near center of
    board get higher priority than moves near the edge of the board.

    @li "solver-order-with-mustplay": if true, each move is played and
    the size of the resulting mustplay is stored.  Moves are ordered
    in increasing order of mustplay.  This is a very, very, expensive
    move ordering, since the vcs and inferior cells must be updated
    for every possible move in every possible state.  However, the
    move ordering is usually very good.  For example, it is probably
    not possible to solve 7x7 without using this heuristic.

    @li "solver-order-with-resist": if true, resistance score is used
    to break ties instead of distance from the center of the board.

    @li "solver-shrink-proof": if true, ice is used to shrink proofs.

    @li "solver-progress-depth": number of ply from the root to print
    the current variation.  Only for debugging purposes.

    @li "solver-update-depth": depth at which the current position is
    dumped to the log.  
*/
class Solver 
{
public:

    //------------------------------------------------------------------------

    /** Constructor. */
    Solver();

    /** Destructor. */
    virtual ~Solver();

    //------------------------------------------------------------------------

    /** Return type for solve(): player to move wins, player to move
     loses, unknown result (timelimit or depth limit reached). */
    typedef enum { WIN, LOSS, UNKNOWN } Result;

    /** Stats for a branch of the search tree. */
    struct BranchStatistics
    {
        /** Total states in tree if no DB and no TT. */
        unsigned total_states;

        /** States actually visited; includes leafs, tt and db hits. */
        unsigned explored_states;

        /** Expanded nodes; non leaf, non tt and db hit states. */
        unsigned expanded_states;

        /** Number of expanded nodes assuming perfect move ordering 
            (assuming the same set of winning moves). */
        unsigned minimal_explored;
        
        /** Decompositions found; if black is to move, it must be a
            decomposition for white. */
        unsigned decompositions;

        /** Decompositions where the player to move won. */
        unsigned decompositions_won;

        /** Total number of moves to consider in expanded states. 
            Includes moves that are later pruned (by mustplay or
            from skipping due to finding a win). */
        unsigned moves_to_consider;

        /** Number of expanded states that had winning moves. */
        unsigned winning_expanded;

        /** Number of branches tried before win was found. */
        unsigned branches_to_win;

        /** States pruned by mustplay pruning. */
        unsigned pruned;

        /** Number of proofs that were successfully shrunk. */
        unsigned shrunk;
        
        /** Total number of cells removed in all successful proof
            shrinkings. */
        unsigned cells_removed;

        BranchStatistics() 
            : total_states(0), explored_states(0), 
              expanded_states(0), minimal_explored(0), 
              decompositions(0), decompositions_won(0), 
              moves_to_consider(0), winning_expanded(0), branches_to_win(0),
              pruned(0), shrunk(0), cells_removed(0)
        { }

        void operator+=(const BranchStatistics& o)
        {
            total_states += o.total_states;
            explored_states += o.explored_states;
            expanded_states += o.expanded_states;
            minimal_explored += o.minimal_explored;
            decompositions += o.decompositions;
            decompositions_won += o.decompositions_won;
            moves_to_consider += o.moves_to_consider;
            winning_expanded += o.winning_expanded;
            branches_to_win += o.branches_to_win;
            pruned += o.pruned;
            shrunk += o.shrunk;
            cells_removed += o.cells_removed;
        }
    };

    //------------------------------------------------------------------------

    /** Contains all relevant data for a solution to a state. */
    struct SolutionSet
    {
        /** @todo not currently used! */
        Result result;

        bitset_t proof;
        int moves_to_connection;
        MoveSequence pv;
        BranchStatistics stats;

        SolutionSet()
	  : moves_to_connection(0) {};
    };
    
    //------------------------------------------------------------------------

    static const int NO_DEPTH_LIMIT    = -1;
    static const double NO_TIME_LIMIT  = -1.0;
    
    static const int SOLVE_ROOT_AGAIN = 1;

    /** User controllable settings. */
    struct Settings
    {
        int flags;

        bool use_db;
        int depth_limit;
        double time_limit;

        Settings() : flags(0) { };
    };

    /** Sets the flags for the next solver run. */
    void SetFlags(int flags);

    /** Gets the current flags. */
    int GetFlags() const;
   
    //------------------------------------------------------------------------    

    /** @name Methods to solve a given boardstate. 
        
        All Return WIN/LOSS if color to play wins/loses; otherwise
        UNKNOWN. 
    */

    // @{

    /** Solves state with no db. */
    Result solve(HexBoard& board, HexColor toplay, SolutionSet& solution,
                 int depth_limit = NO_DEPTH_LIMIT, 
                 double time_imit = NO_TIME_LIMIT);

    /** Uses db in file filename; db is created if it does not
        currently exist. Numstones sets the maximum number of stones
        allowed in a db state; transtones sets the maximum number of
        stones in states stored with proof transpositions. */
    Result solve(HexBoard& board, HexColor toplay, 
                 const std::string& filename, 
                 int numstones, int transtones, 
                 SolutionSet& solution,
                 int depth_limit = NO_DEPTH_LIMIT, 
                 double time_imit = NO_TIME_LIMIT);

    /** Solves state using the supplied db. */
    Result solve(HexBoard& board, HexColor toplay, 
                 SolverDB& db,
                 SolutionSet& solution, 
                 int depth_limit = NO_DEPTH_LIMIT, 
                 double time_imit = NO_TIME_LIMIT);

    // @} // @name

    /** Clears the TT. */
    void ClearTT();

    /** Dumps the stats on # of states, branching factors, etc, for
        the last run. */
    void DumpStats(const SolutionSet& solution) const;

private:

    //------------------------------------------------------------------------
    
    /** Globabl statistics for the current solver run. */
    struct GlobalStatistics
    {
        /** Times HexBoard::PlayMove() was called. */
        unsigned played;

        GlobalStatistics()
            : played(0)
        { }
    };

    //------------------------------------------------------------------------

    /** Stats for the entire search tree broken down by level. */
    struct Histogram
    {
        /** Map of # of stones to a counter. */
        typedef std::map<int, u64> StatsMap;

        /** Terminal states encountered at each depth. */
        StatsMap terminal;
        
        /** Internal states encountered at each depth. */
        StatsMap states;

        /** Winning states encountered at each depth. */
        StatsMap winning;

        StatsMap size_of_winning_states;
        
        StatsMap size_of_losing_states;

        /** Branches taken to find winning move at each depth. */
        StatsMap branches;

        /** Size of original mustplay in winning states. */
        StatsMap mustplay;

        /** States under losing moves before winning move. */
        StatsMap states_under_losing;

        /** DB/TT hits at each depth. */
        StatsMap tthits;

        /** Dumps histogram to a string. */
        std::string Dump();
    };

    //------------------------------------------------------------------------

    /** Plays the move; updates the board.  */
    void PlayMove(HexBoard& brd, HexPoint cell, HexColor color);
    
    /** Takes back the move played. */
    void UndoMove(HexBoard& brd, HexPoint cell);

    //------------------------------------------------------------------------

    void Initialize(const HexBoard&);

    void Cleanup();

    /** Returns true if state is in DB or TT.  Checks DB first, then TT. 
        If return is true, info is stored in state. */
    bool CheckTransposition(const HexBoard& brd, HexColor toplay, 
                            SolvedState& state) const;

    /** Stores the solved state in the TT or DB. */
    void StoreState(const SolvedState& state);

    bitset_t DefaultProofForWinner(const HexBoard& brd, 
                                   HexColor winner) const;

    bool CheckDB(const HexBoard& brd, HexColor toplay, 
                 SolvedState& state) const;

    bool CheckTT(const HexBoard& brd, HexColor toplay, 
                 SolvedState& state) const;

    void StoreInTT(const SolvedState& state);

    void StoreInDB(const SolvedState& state);

    //------------------------------------------------------------------------

    /** Returns true if current state is a terminal node (win/loss),
        or a DB/TT hit.  If so, info is stored in state.  If root_node
        is true and SOLVE_ROOT_AGAIN is set, then no transpositions
        are checked. */
    bool HandleLeafNode(const HexBoard& brd, HexColor color, 
                        SolvedState& state, bool root_node) const;

    bool HandleTerminalNode(const HexBoard& brd, HexColor color, 
                            SolvedState& state) const;

    //------------------------------------------------------------------------

    /** Orders the moves in mustplay using several heuristics. 
        Aborts move ordering early if it finds a TT win: winning
        move is put to the front. 

        Will shrink the mustplay if it encounters TT losses: losing
        moves are not added to the list of sorted moves.

        Returns true if it found a TT win, false otherwise. 
    */
    bool OrderMoves(HexBoard& brd, HexColor color, bitset_t& mustplay, 
                    SolutionSet& solution, 
                    std::vector<HexMoveValue>& moves);

    /** Plays the move and returns the opponent's mustplay
        size. Stores whether there is a winning semi in
        winning_semi_exists. */
    int MustPlaySize(HexBoard& brd, HexColor color,  HexPoint cell,
                     bool& state_in_db, 
                     bool& winning_semi_exists) const;

    //------------------------------------------------------------------------

    /** Helper for solve(). */
    Result run_solver(HexBoard& brd, HexColor tomove, SolutionSet& solution);

    /** Solves the current state in brd for the color to move. Handles
        decompositions if option is turned on. */
    bool solve_state(HexBoard& brd, HexColor tomove,
                     MoveSequence& variation, 
                     SolutionSet& solution);

    /** Solves each side of the decompsosition; combines proofs if
        necessary. */
    bool solve_decomposition(HexBoard& brd, HexColor color, 
                             MoveSequence& variation,
                             SolutionSet& solution,
                             HexPoint group);

    /** Does the recursive mustplay search; calls solve_state() on
        child states. */
    bool solve_interior_state(HexBoard& brd, HexColor color, 
                              MoveSequence& variation,
                              SolutionSet& solution);

    /** Shrinks/verifies proof; stores in tt/db. */
    void handle_proof(const HexBoard& brd, HexColor color, 
                      const MoveSequence& variation,
                      bool winning_state, SolutionSet& solution);

    //------------------------------------------------------------------------

    /** Transposition table. */
    boost::scoped_ptr< TransTable<SolvedState> > m_tt;

    /** Database of solved positions.
        
        @note m_db cannot be a smart pointer because we may be passing
        it in from the outside in one of the solve() methods. 
    */
    SolverDB* m_db;
    bool m_delete_db_when_done;

    double m_start_time, m_end_time;

    std::vector<std::pair<int, int> > m_completed;

    bool m_aborted;

    Settings m_settings;
    mutable Histogram m_histogram;
    mutable GlobalStatistics m_statistics;

    /** Board with no fillin. */
    boost::scoped_ptr<StoneBoard> m_stoneboard;
};

//----------------------------------------------------------------------------

inline void Solver::SetFlags(int flags)
{
    m_settings.flags = flags;
}

inline int Solver::GetFlags() const
{
    return m_settings.flags;
}

//----------------------------------------------------------------------------

/** Methods in Solver that do not need Solver's private data. */
/** @todo Refactor some of these out? */
namespace SolverUtil 
{
    /** Prints the variation; for debugging purposes. */
    std::string PrintVariation(const MoveSequence& variation);
    
    /** Computes distance from the center of the board. */
    int DistanceFromCenter(const ConstBoard& brd, HexPoint p);

    /** Determines if this is a winning state. If so, proof is set to
        the winning proof. */
    bool isWinningState(const HexBoard& brd, HexColor color, 
                        bitset_t& proof);

    /** Determines if the given board state is losing for color. If
        so, proof is set. */
    bool isLosingState(const HexBoard& brd, HexColor color, bitset_t& proof);

    /** Computes the moves to consider for this state. */
    bitset_t MovesToConsider(const HexBoard& brd, HexColor color,
                             bitset_t& proof);

    /** Computes the union of opponent winning semis. */
    bitset_t MustplayCarrier(const HexBoard& brd, HexColor color);

    /** Returns the original losing proof for this state; ie, the
        union of the opponent's winning semi-connections. */
    bitset_t InitialProof(const HexBoard& brd, HexColor color);

    /** Gives all cells outside of the proof to loser, computes fillin
        using ice, removes any cell in proof that is filled-in. */
    void ShrinkProof(bitset_t& proof, 
                     const StoneBoard& board, HexColor loser, 
                     const ICEngine& ice);

}

//----------------------------------------------------------------------------

#endif // SOLVER_H
