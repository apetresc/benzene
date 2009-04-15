//----------------------------------------------------------------------------
// $Id: HtpHexEngine.hpp 1677 2008-09-19 21:05:39Z broderic $
//----------------------------------------------------------------------------

#ifndef HTPHEXENGINE_H
#define HTPHEXENGINE_H

#include "GtpEngine.h"
#include "HexProgram.hpp"
#include "Game.hpp"

class SgNode;

//----------------------------------------------------------------------------

typedef GtpCommand HtpCommand;
typedef GtpFailure HtpFailure;

//----------------------------------------------------------------------------

/**
 * Implements the basic htp commands any Hex engine is required to
 * support.
 */
class HtpHexEngine: public GtpEngine
{
public:

    HtpHexEngine(std::istream& in, std::ostream& out, 
                 HexProgram &program);
    
    ~HtpHexEngine();

    /** @name Command Callbacks */
    // @{
    // The callback functions are documented in the cpp file

    virtual void CmdName(HtpCommand&);
    virtual void CmdVersion(HtpCommand&);
    virtual void CmdListSettings(HtpCommand&);
    virtual void CmdPlay(HtpCommand&);
    virtual void CmdGenMove(HtpCommand& cmd);
    virtual void CmdUndo(HtpCommand& cmd);
    virtual void CmdNewGame(HtpCommand& cmd);
    virtual void CmdShowboard(HtpCommand&);
    virtual void CmdBoardID(HtpCommand&);
    virtual void CmdTimeLeft(HtpCommand&);
    virtual void CmdFinalScore(HtpCommand& cmd);
    virtual void CmdAllLegalMoves(HtpCommand& cmd);
    virtual void CmdLoadSgf(HtpCommand& cmd);

    // @} // @name

    virtual void Ponder();
    virtual void InitPonder();
    virtual void StopPonder();

    virtual void Interrupt();

protected:

    HexColor ColorArg(const HtpCommand& cmd, std::size_t number) const;

    HexPoint MoveArg(const HtpCommand& cmd, std::size_t number) const;

    void PrintBitsetToHTP(HtpCommand& cmd, const bitset_t& bs) const;

    void SetTimeRemaining(HexColor color, double time);

    void ReduceTimeRemaining(HexColor color, double time);

    double TimeRemaining(HexColor color) const;
            
    /** Plays a move. */
    virtual void Play(HexColor color, HexPoint move);

    /** Creates a new game on a board with given dimensions. */
    virtual void NewGame(int width, int height);

    /** Generates a move. */
    virtual HexPoint GenMove(HexColor color, double time_remaining);

    void SetPosition(const SgNode* node);

    StoneBoard m_board;
    Game* m_game;
    HexProgram& m_program;
    double m_timeRemaining[BLACK_AND_WHITE];

private:
    void RegisterCmd(const std::string& name,
                     GtpCallback<HtpHexEngine>::Method method);
};

//----------------------------------------------------------------------------

#endif // HTPHEXENGINE_H
