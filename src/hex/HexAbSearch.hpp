//----------------------------------------------------------------------------
// $Id: HexAbSearch.hpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#ifndef HEXABSEARCH_HPP
#define HEXABSEARCH_HPP

#include "Hex.hpp"
#include "HexEval.hpp"
#include "SearchedState.hpp"
#include "TransTable.hpp"

//----------------------------------------------------------------------------

class HexBoard;

/** Base Alpha-Beta search class. */
class HexAbSearch
{
public:
    
    /** Constructor. */
    explicit HexAbSearch(HexBoard& board, HexColor color);

    /** Destructor. */
    virtual ~HexAbSearch();

    //------------------------------------------------------------------------

    /** Sets the transposition table to be used during search. */
    void SetTT(TransTable<SearchedState>* tt);

    /** Writes progress of search in guifx format after each root move
        completes. Off by default. */
    bool GuiFx() const;

    /** Sets whether guifx output should be dumped. 
        @see GuiFX(). */
    void SetGuiFx(bool flag);

    //------------------------------------------------------------------------

    /** Starts the AlphaBeta search. 
        @param plywidth  depth of the search set to plywidth.size()
                         and plywidth[j] top moves are explored.
        @param depths_to_search successive depths to search (like in ID).
        @param timelimit amount of time to finish search.
        @param PV        the principal variation will be stored here. 
        @returns         the evaluation of the PV.  
    */
    HexEval Search(const std::vector<int>& plywidth, 
                   const std::vector<int>& depths_to_search,
                   int timelimit,
                   std::vector<HexPoint>& PV);
   
    /** Returns the scores for each move from the root. */
    const std::vector<HexMoveValue>& RootMoveScores() const;

    //------------------------------------------------------------------------

    /** Evaluates leaf position. */
    virtual HexEval Evaluate()=0;

    /** Generates moves for this position.  Moves will be played
        in the returned order. */
    virtual void GenerateMoves(std::vector<HexPoint>& moves)=0;

    /** Plays the given move. */
    virtual void ExecuteMove(HexPoint move)=0;

    /** Undoes the given move. */
    virtual void UndoMove(HexPoint move)=0;

    /** Hook function called upon entering new position. 
        Default implementation does nothing. */
    virtual void EnteredNewState();

    /** Hook function called at the very start of the search. 
        Default implementation does nothing. */
    virtual void OnStartSearch();
    
    /** Hook function called after the search has completed. 
        Default implementation does nothing. */
    virtual void OnSearchComplete();

    /** Hook function called after a states moves have been searched. 
        Default implementation does nothing. */
    virtual void AfterStateSearched();

    //------------------------------------------------------------------------

    /** Output stats from search. */
    std::string DumpStats();

protected:

    /** The board we are playing on. */
    HexBoard& m_brd;

    /** Color of player to move next. */
    HexColor m_toplay;

    /** Transposition table to use during search, if any. */
    TransTable<SearchedState>* m_tt;

    /** @see GuiFx(). */
    bool m_use_guifx;
        
    /** Number of moves from the root. */
    int m_current_depth;

    /** Sequences of moves to the current state. */
    MoveSequence m_sequence;

    /** Value of root state: only valid after search is completed. */
    HexEval m_value;
    
    /** Principal variation from the root: only valid after 
        search is completed. */
    std::vector<HexPoint> m_pv;

    /** If current state exists in TT, but TT state was not deep
        enough, this will hold the best move for that state; otherwise
        it will be INVALID_MOVE.  Could be used in GenerateMoves() to
        improve move ordering when using iterative deepening. */
    HexPoint m_tt_bestmove;
    bool m_tt_info_available;

private:
    void ClearStats();

    std::string DumpPV(HexEval value, std::vector<HexPoint>& pv);

    void UpdatePV(int current_depth, HexEval value, std::vector<HexPoint>& cv);

    HexEval CheckTerminalState();

    HexEval search_state(const std::vector<int>& plywidth,
                         int depth, 
			 HexEval alpha, HexEval beta,
                         std::vector<HexPoint>& cv);

    /** Search statistics. */
    int m_numstates;
    int m_numleafs;
    int m_numterminal;
    int m_numinternal;
    int m_mustplay_branches;
    int m_total_branches;
    int m_visited_branches;
    int m_cuts;
    int m_tt_hits;
    int m_tt_cuts;
    double m_elapsed_time;

    /** Evaluations for each move from the root state. */
    std::vector<HexMoveValue> m_eval;

};

inline void HexAbSearch::SetTT(TransTable<SearchedState>* tt)
{
    m_tt = tt;
}

inline void HexAbSearch::SetGuiFx(bool flag)
{
    m_use_guifx = flag;
}

inline bool HexAbSearch::GuiFx() const
{
    return m_use_guifx;
}

inline const std::vector<HexMoveValue>& HexAbSearch::RootMoveScores() const
{
    return m_eval;
}

//----------------------------------------------------------------------------

#endif // HEXABSEARCH_HPP
