//----------------------------------------------------------------------------
// $Id: PerfectPlayer.hpp 1536 2008-07-09 22:47:27Z broderic $
//----------------------------------------------------------------------------

#ifndef PERFECTPLAYER_HPP
#define PERFECTPLAYER_HPP

#include "Solver.hpp"
#include "HexBoard.hpp"
#include "UofAPlayer.hpp"

//----------------------------------------------------------------------------

/** Player using Solver to generate moves. Works best on boards 7x7
    and smaller.

    PerfectPlayer can use a SolverDB; set "perfect-use-db" to "true"
    and "perfect-db-filename" to the name of the db. 
    
    If the db does not exist, the state will be solved without using a
    db. If the db does exist, and the current state is in the db, then
    all moves are checked and the best move is returned.  If the state
    is not in the db, it is solved and added.
*/
class PerfectPlayer : public UofAPlayer
{
public:

    explicit PerfectPlayer(Solver* solver);
    virtual ~PerfectPlayer();
    
    /** Returns "perfect". */
    std::string name() const;
    
protected:

    /** Generates a move in the given gamestate using Solver. */
    virtual HexPoint search(HexBoard& brd, const Game& game_state,
			    HexColor color, const bitset_t& consider,
                            double time_remaining, double& score);

    Solver* m_solver;

private:

    bool find_db_move(StoneBoard& brd, HexColor color, 
                      HexPoint& move_to_play, bitset_t& proof, 
                      double& score) const;

    void solve_new_state(HexBoard& brd, HexColor color, 
                         HexPoint& move_to_play, bitset_t& proof, 
                         double& score) const;
};

inline std::string PerfectPlayer::name() const
{
    return "perfect";
}

//----------------------------------------------------------------------------

#endif // HEXSOLVERPLAYER_HPP
