//----------------------------------------------------------------------------
// $Id: HtpUofAEngine.hpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#ifndef HTPUOFAENGINE_H
#define HTPUOFAENGINE_H

#include "HtpHexEngine.hpp"
#include "OpeningBook.hpp"
#include "PatternBoard.hpp"
#include "UofAProgram.hpp"
#include "UofAPlayer.hpp"
#include "Game.hpp"

//----------------------------------------------------------------------------

/** 
    HTP engine supporting extra commands for Wolve and MoHex. 
*/
class HtpUofAEngine: public HtpHexEngine
{
public:

    HtpUofAEngine(std::istream& in, std::ostream& out, 
                  UofAProgram &program);
    
    ~HtpUofAEngine();

    /** @name Command Callbacks */
    // @{
    // The callback functions are documented in the cpp file

    virtual void CmdScoreForLastMove(HtpCommand& cmd);
    virtual void CmdRegGenMove(HtpCommand& cmd);

    virtual void CmdGetAbsorbGroup(HtpCommand& cmd);
    
    virtual void CmdBookOpen(HtpCommand& cmd);
    virtual void CmdBookExpand(HtpCommand& cmd);
    virtual void CmdBookMainLineDepth(HtpCommand& cmd);
    virtual void CmdBookTreeSize(HtpCommand& cmd);
    virtual void CmdBookScores(HtpCommand& cmd);
    virtual void CmdBookPriorities(HtpCommand& cmd);

    virtual void CmdOnShortestPaths(HtpCommand& cmd);
    virtual void CmdOnShortestVCPaths(HtpCommand& cmd);
    
    virtual void CmdComputeInferior(HtpCommand& cmd);
    virtual void CmdComputeFillin(HtpCommand& cmd);
    virtual void CmdComputeVulnerable(HtpCommand& cmd);
    virtual void CmdComputeDominated(HtpCommand& cmd);
    virtual void CmdFindCombDecomp(HtpCommand& cmd);
    virtual void CmdFindSplitDecomp(HtpCommand& cmd);
    virtual void CmdEncodePattern(HtpCommand& cmd);

    virtual void CmdResetVcs(HtpCommand& cmd);
    virtual void CmdGetVCsBetween(HtpCommand& cmd);
    virtual void CmdGetCellsConnectedTo(HtpCommand& cmd);
    virtual void CmdGetMustPlay(HtpCommand& cmd);
    virtual void CmdVCIntersection(HtpCommand& cmd);
    virtual void CmdVCUnion(HtpCommand& cmd);
    virtual void CmdVCMaintain(HtpCommand& cmd);

    virtual void CmdBuildStatic(HtpCommand& cmd);
    virtual void CmdBuildIncremental(HtpCommand& cmd);
    virtual void CmdUndoIncremental(HtpCommand& cmd);

    virtual void CmdDumpVcs(HtpCommand& cmd);

    virtual void CmdEvalTwoDist(HtpCommand& cmd);
    virtual void CmdEvalResist(HtpCommand& cmd);
    virtual void CmdEvalResistDelta(HtpCommand& cmd);
    virtual void CmdResistTest(HtpCommand& cmd);
    virtual void CmdEvalInfluence(HtpCommand& cmd);
    virtual void CmdResistCompareRotation(HtpCommand& cmd);

    virtual void CmdSolveState(HtpCommand& cmd);
    virtual void CmdSolverClearTT(HtpCommand& cmd);
    virtual void CmdSolverFindWinning(HtpCommand& cmd);

    void CmdDBOpen(HtpCommand& cmd);
    void CmdDBClose(HtpCommand& cmd);
    void CmdDBGet(HtpCommand& cmd);

    virtual void CmdMiscDebug(HtpCommand& cmd);

    // @} // @name

    virtual void Ponder();
    virtual void InitPonder();
    virtual void StopPonder();

    virtual void Interrupt();

protected:

    VC::Type VCTypeArg(const HtpCommand& cmd, std::size_t number) const;

    void PrintVC(HtpCommand& cmd, const VC& vc, HexColor color) const;
    
    virtual HexPoint GenMove(HexColor color, double time_remaining);

    /** Returns a pointer to the work board. */
    HexBoard* Board();

    /** Frees the Board. This will force a re-allocation the next time
        SyncBoard() is called. */
    void DeleteBoard();

    /** Syncs our HexBoard with the given StoneBoard. Deletes old
        board and allocates new one if sizes are different.
    
        Sets the settings returned by BoardFlags() in the board.

        @return Returns a pointer to the sync'd board. 
    */
    HexBoard* SyncBoard(const StoneBoard& board);

private:

    void RegisterCmd(const std::string& name,
                     GtpCallback<HtpUofAEngine>::Method method);

    bool StateMatchesBook(const StoneBoard& brd);




    UofAProgram& m_uofa_program;
    UofAPlayer* Player();

    HexBoard* m_brd;

    OpeningBook* m_book;

    SolverDB* m_db;

    double m_score_for_last_move;
};

inline HexBoard* HtpUofAEngine::Board()
{
    return m_brd;
}

inline UofAPlayer* HtpUofAEngine::Player()
{
    return dynamic_cast<UofAPlayer*>(m_uofa_program.Player());
}

//----------------------------------------------------------------------------

#endif // HTPUOFENGINE_H
