//----------------------------------------------------------------------------
// $Id: WolvePlayer.hpp 1669 2008-09-16 23:30:36Z broderic $
//----------------------------------------------------------------------------

#ifndef WOLVEPLAYER_HPP
#define WOLVEPLAYER_HPP

#include <boost/scoped_ptr.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

#include "UofAPlayer.hpp"
#include "TwoDistance.hpp"
#include "HexAbSearch.hpp"
#include "Resistance.hpp"

//----------------------------------------------------------------------------

/** Hex player using AbEngine to generate moves. */
class WolvePlayer : public UofAPlayer
{
public:
    
    /** Creates a player. */
    explicit WolvePlayer();
    
    /** Destructor. */
    virtual ~WolvePlayer();
  
    /** Returns "wolve". */
    std::string name() const;

protected:

    /** Generates a move in the given gamestate using alphabeta. */
    virtual HexPoint search(HexBoard& brd, const Game& game_state,
			    HexColor color, const bitset_t& consider, 
                            double time_remaining, double& score);
};

inline std::string WolvePlayer::name() const
{
    return "wolve";
}

//----------------------------------------------------------------------------

typedef enum 
{
    /** Tells thread to quit. */
    WOLVE_THREAD_QUIT=0,

    /** Tells thread to play a move. */
    WOLVE_THREAD_PLAY,

    /** Tells thread to undo last move played. */
    WOLVE_THREAD_UNDO

} WolveThreadAction;

/** Plays/Takesback moves. */
struct WolveWorkThread
{
    WolveWorkThread(HexBoard& brd, 
                    boost::barrier& start,
                    boost::barrier& completed,
                    const WolveThreadAction& action,
                    const HexPoint& move,
                    const HexColor& color);

    void operator()();

    HexBoard& m_brd;
    boost::barrier& m_start;
    boost::barrier& m_completed;
    const WolveThreadAction& m_action;
    const HexPoint& m_move;
    const HexColor& m_color;
};

//----------------------------------------------------------------------------

/** 

*/
struct VariationInfo
{
    VariationInfo()
        : hash(0), depth(-1)
    { }
    
    VariationInfo(hash_t h, int d, const bitset_t& con)
        : hash(h), depth(d), consider(con)
    { }

    ~VariationInfo();
    
    bool Initialized() const;

    hash_t Hash() const;

    void CheckCollision(const VariationInfo& other) const;

    bool ReplaceWith(const VariationInfo& other) const;

    //------------------------------------------------------------------------

    /** Hash for this variation. */
    hash_t hash;
   
    /** Depth state was searched. */
    int depth;

    /** Moves to consider from this variation. */
    bitset_t consider;
};

inline VariationInfo::~VariationInfo()
{
}

inline bool VariationInfo::Initialized() const
{
    return (depth != -1);
}

inline hash_t VariationInfo::Hash() const
{
    return hash;
}

inline 
void VariationInfo::CheckCollision(const VariationInfo& UNUSED(other)) const
{
    /** @todo Check for hash variation collisions. */
}

inline bool VariationInfo::ReplaceWith(const VariationInfo& other) const
{
    /** @todo check for better bounds/scores? */

    // replace this state only with a deeper state
    return (other.depth > depth);
}

//-------------------------------------------------------------------------- 

/** Performs an AlphaBeta search with two boards in parallel. */
class WolveSearch : public HexAbSearch
{
public:
    explicit WolveSearch(HexBoard& board, HexColor color, 
                         const bitset_t& rootMTC);

    virtual ~WolveSearch();

    /** Virtual methods from HexAbSearch. */
    virtual HexEval Evaluate();
    virtual void GenerateMoves(std::vector<HexPoint>& moves);
    virtual void ExecuteMove(HexPoint move);
    virtual void UndoMove(HexPoint move);
    virtual void EnteredNewState();
    virtual void OnStartSearch();
    virtual void OnSearchComplete();
    virtual void AfterStateSearched();

private:

    /** Copy the board to the fill-in board. */
    void SetupNonFillinBoard();

    /** Computes the evaluation on the no_fillin_board (if it exists)
        using the ConductanceGraph from m_brd. */
    void ComputeResistance(Resistance& resist);

    /** Board with no fill-in. Used for circuit evaluation since 
        fill-in reduces the amount of flow. */
    boost::scoped_ptr<HexBoard> m_no_fillin_board;

    /** Consider sets for each depth. */
    std::vector<bitset_t> m_consider;

    /** Moves to consider in root state. */
    bitset_t m_rootMTC;

    /** Variation TT. */
    TransTable<VariationInfo> m_varTT;

    //---------------------------------------------------------------------- 
    
    void StartThreads();

    void StopThreads();

    boost::barrier m_start_threads;
    boost::barrier m_threads_completed;
    boost::scoped_ptr<boost::thread> m_thread[2];

    WolveThreadAction m_threadAction;
    HexPoint m_threadMove;
    HexColor m_threadColor;
};

//----------------------------------------------------------------------------

#endif // WOLVEPLAYER_HPP
