//----------------------------------------------------------------------------
// $Id: BenzeneHtpEngine.hpp 1878 2009-01-29 03:20:00Z broderic $
//----------------------------------------------------------------------------

#ifndef BENZENEHTPENGINE_H
#define BENZENEHTPENGINE_H

#include "HexHtpEngine.hpp"
#include "BenzenePlayer.hpp"
#include "Solver.hpp"

class OpeningBook;

//----------------------------------------------------------------------------

struct HexEnvironment
{
    HexEnvironment(int width, int height);

    void NewGame(int width, int height);

    HexBoard& SyncBoard(const StoneBoard& brd);

    ICEngine ice;

    ConnectionBuilderParam buildParam;

    boost::scoped_ptr<HexBoard> brd;
    
};

inline HexEnvironment::HexEnvironment(int width, int height)
    : brd(0)
{
    brd.reset(new HexBoard(width, height, ice, buildParam));
}

//----------------------------------------------------------------------------

/** HTP engine with commands for stuff common to all UofA Hex
    players. */
class BenzeneHtpEngine: public HexHtpEngine
{
public:

    BenzeneHtpEngine(std::istream& in, std::ostream& out, int boardsize, 
                  BenzenePlayer& player);
    
    ~BenzeneHtpEngine();

    /** @name Command Callbacks */
    // @{
    // The callback functions are documented in the cpp file

    virtual void CmdRegGenMove(HtpCommand& cmd);

    virtual void CmdGetAbsorbGroup(HtpCommand& cmd);
    
    virtual void CmdBookOpen(HtpCommand& cmd);
    virtual void CmdBookExpand(HtpCommand& cmd);
    virtual void CmdBookMainLineDepth(HtpCommand& cmd);
    virtual void CmdBookTreeSize(HtpCommand& cmd);
    virtual void CmdBookScores(HtpCommand& cmd);
    virtual void CmdBookPriorities(HtpCommand& cmd);
   
    virtual void CmdComputeInferior(HtpCommand& cmd);
    virtual void CmdComputeFillin(HtpCommand& cmd);
    virtual void CmdComputeVulnerable(HtpCommand& cmd);
    virtual void CmdComputeDominated(HtpCommand& cmd);
    virtual void CmdFindCombDecomp(HtpCommand& cmd);
    virtual void CmdFindSplitDecomp(HtpCommand& cmd);
    virtual void CmdEncodePattern(HtpCommand& cmd);

    virtual void CmdParamPlayerICE(HtpCommand& cmd);
    virtual void CmdParamPlayerVC(HtpCommand& cmd);
    virtual void CmdParamPlayerBoard(HtpCommand& cmd);
    virtual void CmdParamPlayer(HtpCommand& cmd);
    virtual void CmdParamSolverICE(HtpCommand& cmd);
    virtual void CmdParamSolverVC(HtpCommand& cmd);
    virtual void CmdParamSolverBoard(HtpCommand& cmd);
    virtual void CmdParamSolver(HtpCommand& cmd);

    virtual void CmdGetVCsBetween(HtpCommand& cmd);
    virtual void CmdGetCellsConnectedTo(HtpCommand& cmd);
    virtual void CmdGetMustPlay(HtpCommand& cmd);
    virtual void CmdVCIntersection(HtpCommand& cmd);
    virtual void CmdVCUnion(HtpCommand& cmd);
    virtual void CmdVCMaintain(HtpCommand& cmd);

    virtual void CmdBuildStatic(HtpCommand& cmd);
    virtual void CmdBuildIncremental(HtpCommand& cmd);
    virtual void CmdUndoIncremental(HtpCommand& cmd);

    virtual void CmdEvalTwoDist(HtpCommand& cmd);
    virtual void CmdEvalResist(HtpCommand& cmd);
    virtual void CmdEvalResistDelta(HtpCommand& cmd);
    virtual void CmdEvalInfluence(HtpCommand& cmd);

    virtual void CmdSolveState(HtpCommand& cmd);
    virtual void CmdSolverClearTT(HtpCommand& cmd);
    virtual void CmdSolverFindWinning(HtpCommand& cmd);

    void CmdDBOpen(HtpCommand& cmd);
    void CmdDBClose(HtpCommand& cmd);
    void CmdDBGet(HtpCommand& cmd);

    virtual void CmdMiscDebug(HtpCommand& cmd);

    // @} // @name

protected:

    VC::Type VCTypeArg(const HtpCommand& cmd, std::size_t number) const;

    void PrintVC(HtpCommand& cmd, const VC& vc, HexColor color) const;

    virtual void NewGame(int width, int height);
    
    virtual HexPoint GenMove(HexColor color, double time_remaining);

    /** Searches through the player decorators to find an instance
        of type T. Returns 0 on failure. */
    template<typename T> T* GetInstanceOf(BenzenePlayer* player);

    void ParamPlayer(BenzenePlayer* player, HtpCommand& cmd);

    //-----------------------------------------------------------------------

    BenzenePlayer& m_player;

    /** Player's environment. */
    HexEnvironment m_pe;

    /** Solver's environment. */
    HexEnvironment m_se;

    boost::scoped_ptr<Solver> m_solver;

    boost::scoped_ptr<SolverTT> m_solver_tt;

    OpeningBook* m_book;

    SolverDB* m_db;

private:

    void RegisterCmd(const std::string& name,
                     GtpCallback<BenzeneHtpEngine>::Method method);

    bool StateMatchesBook(const StoneBoard& brd);

};

template<typename T> T* BenzeneHtpEngine::GetInstanceOf(BenzenePlayer* player)
{
    T* obj = dynamic_cast<T*>(player);
    if (obj)
        return obj;
    BenzenePlayerFunctionality* func 
        = dynamic_cast<BenzenePlayerFunctionality*>(player);
    if (func)
        return GetInstanceOf<T>(func->PlayerExtending());
    return 0;
}

//----------------------------------------------------------------------------

#endif // BENZENEHTPENGINE_H
