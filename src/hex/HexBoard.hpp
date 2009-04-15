//----------------------------------------------------------------------------
// $Id: HexBoard.hpp 1699 2008-09-24 23:55:56Z broderic $
//----------------------------------------------------------------------------

#ifndef HEXBOARD_H
#define HEXBOARD_H

#include <boost/scoped_ptr.hpp>

#include "Connections.hpp"
#include "ICEngine.hpp"
#include "PatternBoard.hpp"
#include "VCPattern.hpp"

//----------------------------------------------------------------------------

/** Combines GroupBoard, PatternBoard, and Connections into a board
    that handles all updates automatically.
  
    @todo Document me!
*/
class HexBoard : public PatternBoard
{
public:
    
    /** Creates a square board. */
    HexBoard(int size, const ICEngine& ice);

    /** Creates a rectangular board. */
    HexBoard(int width, int size, const ICEngine& ice);

    /** Destructor. */
    ~HexBoard();

    //-----------------------------------------------------------------------

    /** VCs are computed; default is on. */
    static const int USE_VCS = 1;

    /** Inferior cells are computed; default is on. */
    static const int USE_ICE = 2;

    /** VC Decompositions are computed and filled. */
    static const int USE_DECOMPOSITIONS = 4;

    struct Settings 
    {
        int flags;

        Settings() : flags(USE_VCS | USE_ICE | USE_DECOMPOSITIONS) { };
    };

    void SetFlags(int flags);
    int GetFlags() const;

    //-----------------------------------------------------------------------

    /** Returns the vc mustplay for color (ie, the intersection of all
        of the other color's winning semi-connections). */
    bitset_t getMustplay(HexColor color) const;

    //-----------------------------------------------------------------------

    /** Controls how to deal with endgames caused by fill-in. */
    typedef enum 
    { 
        /** All of the winning player's fill-in is removed if the fill-in
            creates a solid winning chain. */
        REMOVE_WINNING_FILLIN,

        /** Winning fill-in is not removed. */
        DO_NOT_REMOVE_WINNING_FILLIN 

    } EndgameFillin;

    /** Clears history.  Computes dead/vcs for current state. */
    virtual void ComputeAll(HexColor color, 
                            EndgameFillin endgame_mode,
                            int max_ors = Connections::DEFAULT);

    /** Stores old state on stack, plays move to board, updates
        ics/vcs.  Hash is modified by the move.  Allows ice info to
        be backed-up. */
    virtual void PlayMove(HexColor color, HexPoint cell,
                          int max_ors = Connections::DEFAULT);
    
    /** Stores old state on stack, plays set of stones, updates
        ics/vcs. HASH IS NOT MODIFIED! No ice info will be backed up,
        but this set of moves can be reverted with a single call to
        UndoMove(). */
    virtual void PlayStones(HexColor color, const bitset_t& played,
                            HexColor color_to_move,
                            int max_ors = Connections::DEFAULT);
        
    /** Adds stones for color to board with color_to_move about to
        play next; added stones must be a subset of the empty cells.
        Does not affect the hash of this state. State is not pushed
        onto stack, so a call to UndoMove() will undo these changes
        along with the last changes that changed the stack. */
    virtual void AddStones(HexColor color, const bitset_t& played,
                           HexColor color_to_move, 
                           EndgameFillin endgame_mode, 
                           int max_ors = Connections::DEFAULT);    

    /** Reverts to last state stored on the stack, restoring all state
        info. If the option is on, also backs up inferior cell
        info. */
    virtual void UndoMove();

    //-----------------------------------------------------------------------

    /** Returns domination arcs added from backing-up. */
    const std::set<HexPointPair>& GetBackedUp() const;

    /** Adds domination arcs in dom to set of inferior cells for this
        state. */
    void AddDominationArcs(const std::set<HexPointPair>& dom);

    //-----------------------------------------------------------------------

    /** Returns the set of dead cells on the board. This is the union
        of all cells found dead previously during the history of moves
        since the last ComputeAll() call.  */
    bitset_t getDead() const;
    
    /** Returns the set of inferior cell. */
    const InferiorCells& getInferiorCells() const;

    /** Returns the Inferior Cell Engine the board is using. */
    const ICEngine& ICE() const;

    /** Returns the connection set for color. */
    const Connections& Cons(HexColor color) const;

private:

    void init();

    void ComputeInferiorCells(HexColor color_to_move, 
                              EndgameFillin endgame_mode);

    void BuildVCs(int max_ors);
    void BuildVCs(bitset_t added[BLACK_AND_WHITE], int max_ors,
                  bool mark_the_log = true);

    void RevertVCs();

    /** In non-terminal states, checks for combinatorial decomposition
        with a vc using FindCombinatorialDecomposition(). Plays the carrier
	using AddStones(). Loops until no more decompositions are found. */
    void HandleVCDecomposition(HexColor color_to_move, 
                               EndgameFillin endgame_mode,
                               int max_ors);

    void ClearHistory();
    void PushHistory(HexColor color, HexPoint cell);
    void PopHistory();

    //-----------------------------------------------------------------------

    struct History
    {
        /** Saved board state. */
        StoneBoard board;

        /** The inferior cell data for this state. */
        InferiorCells inf;

        /** Domination arcs added from backing-up. */
        std::set<HexPointPair> backedup;

        /** Color to play from this state. */
        HexColor to_play;
        
        /** Move last played from this state. */
        HexPoint last_played;

        History(const StoneBoard& b, const InferiorCells& i, 
                const std::set<HexPointPair>& back, HexColor tp, HexPoint lp)
            : board(b), inf(i), backedup(back), to_play(tp), last_played(lp) 
        { }
    };

    //-----------------------------------------------------------------------

    /** ICEngine used to compute inferior cells. */
    const ICEngine& m_ice;

    /** Connection sets for black and white. */
    boost::scoped_ptr<Connections> m_cons[BLACK_AND_WHITE];

    /** The vc changelogs for both black and white. */
    ChangeLog<VC> m_log[BLACK_AND_WHITE];

    /** Current settings for this board. */
    Settings m_settings;

    /** History stack. */
    std::vector<History> m_history;

    /** The set of inferior cells for the current boardstate. */
    InferiorCells m_inf;

    /** Domination arcs added from backing-up ic info from moves
        played from this state. */
    std::set<HexPointPair> m_backedup;
};

inline bitset_t HexBoard::getDead() const
{
    return m_inf.Dead();
}

inline const InferiorCells& HexBoard::getInferiorCells() const
{
    return m_inf;
}

inline const std::set<HexPointPair>& HexBoard::GetBackedUp() const
{
    return m_backedup;
}

inline const ICEngine& HexBoard::ICE() const
{
    return m_ice;
}

inline const Connections& HexBoard::Cons(HexColor color) const
{
    return *m_cons[color].get();
}

inline void HexBoard::SetFlags(int flags)
{
    m_settings.flags = flags;
}

inline int HexBoard::GetFlags() const
{
    return m_settings.flags;
}

//----------------------------------------------------------------------------

#endif // HEXBOARD_H
