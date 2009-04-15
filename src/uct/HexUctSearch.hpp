//----------------------------------------------------------------------------
// $Id: HexUctSearch.hpp 1470 2008-06-17 22:04:32Z broderic $ 
//----------------------------------------------------------------------------

#ifndef HEXUCTSEARCH_H
#define HEXUCTSEARCH_H

#include "SgBlackWhite.h"
#include "SgPoint.h"
#include "SgUctSearch.h"

#include "HexUctState.hpp"
#include "PatternBoard.hpp"

class SgNode;

//----------------------------------------------------------------------------

class HexUctSharedPolicy;

/** Creates threads. */
class HexThreadStateFactory : public SgUctThreadStateFactory
{
public:
    HexThreadStateFactory(HexUctSharedPolicy* shared_policy);

    ~HexThreadStateFactory();

    SgUctThreadState* Create(std::size_t threadId,
                             const SgUctSearch& search);
private:

    HexUctSharedPolicy* m_shared_policy;
};

//----------------------------------------------------------------------------

/** Monte-Carlo search using UCT for Hex. */
class HexUctSearch : public SgUctSearch
{
public:
    /** Constructor.
        @param bd The board
        @param tomove The color of the player to move. 
        @param factory 
    */
    HexUctSearch(PatternBoard& bd, 
                 const HexUctInitialData* data, 
		 SgUctThreadStateFactory* factory,
		 int maxMoves = 0);
    
    /** Destructor. */
    ~HexUctSearch();    
    
    /** @name Pure virtual functions of SgUctSearch */
    // @{

    std::string MoveString(SgMove move) const;

    float UnknownEval() const;

    float InverseEval(float eval) const;

    // @} // @name

    /** @name Virtual functions of SgUctSearch */
    // @{

    virtual void OnSearchIteration(std::size_t gameNumber, int threadId,
                                   const SgUctGameInfo& info);

    void OnStartSearch();

    // @} // @name

    /** @name Hex-specific functions */
    // @{

    PatternBoard& Board();

    const PatternBoard& Board() const;

    const HexUctInitialData* InitialData() const;

    /** @see SetKeepGames()
        @throws SgException if KeepGames() was false at last invocation of
        StartSearch()
    */
    void SaveGames(const std::string& fileName) const;

    /** @see HexUctUtil::SaveTree() */
    void SaveTree(std::ostream& out) const;

    //FIXME: Phil commented these out just because, but don't seem to exist?!?
    //void SetToPlay(HexColor toPlay);
    //HexColor ToPlay() const;

    // @} // @name

    /** @name Hex-specific parameters */
    // @{

    /** Keep a SGF tree of all games.
        This is reset in OnStartSearch() and can be saved with SaveGames().
    */
    void SetKeepGames(bool enable);

    /** @see SetKeepGames() */
    bool KeepGames() const;

    /** Enable outputting of live graphics commands for GoGui.
        Outputs LiveGfx commands for GoGui to the debug stream every
        n games.
        @see GoGuiGfx(), SetLiveGfxInterval()
    */
    void SetLiveGfx(bool enable);

    /** @see SetLiveGfx() */
    bool LiveGfx() const;

    /** Set interval for outputtingof live graphics commands for GoGui.
        Default is every 5000 games.
        @see SetLiveGfx()
    */
    void SetLiveGfxInterval(int interval);

    /** @see SetLiveGfxInterval() */
    int LiveGfxInterval() const;
    
    // @} // @name

protected:

    /** @see SetKeepGames() */
    bool m_keepGames;

    /** @see SetLiveGfx() */
    bool m_liveGfx;

    /** @see SetLiveGfxInterval() */
    int m_liveGfxInterval;

    //----------------------------------------------------------------------

    /** Board, nothing is done to this board. Threads make their own
        copies of this board. */
    PatternBoard& m_bd;

   
    /** Data for first few play of the game. Shared amoung threads. */
    const HexUctInitialData* m_initial_data;

    //----------------------------------------------------------------------

    /** @see SetKeepGames() */
    SgNode* m_root;
    
    /** Not implemented */
    HexUctSearch(const HexUctSearch& search);

    /** Not implemented */
    HexUctSearch& operator=(const HexUctSearch& search);

    //void AppendGame(const std::vector<SgMove>& sequence);
};

inline PatternBoard& HexUctSearch::Board()
{
    return m_bd;
}

inline const PatternBoard& HexUctSearch::Board() const
{
    return m_bd;
}

inline bool HexUctSearch::KeepGames() const
{
    return m_keepGames;
}

inline bool HexUctSearch::LiveGfx() const
{
    return m_liveGfx;
}

inline int HexUctSearch::LiveGfxInterval() const
{
    return m_liveGfxInterval;
}

inline void HexUctSearch::SetKeepGames(bool enable)
{
    m_keepGames = enable;
}

inline void HexUctSearch::SetLiveGfx(bool enable)
{
    m_liveGfx = enable;
}

inline void HexUctSearch::SetLiveGfxInterval(int interval)
{
    SG_ASSERT(interval > 0);
    m_liveGfxInterval = interval;
}

inline const HexUctInitialData* HexUctSearch::InitialData() const
{
    return m_initial_data;
}

//----------------------------------------------------------------------------

#endif // HEXUCTSEARCH_H
