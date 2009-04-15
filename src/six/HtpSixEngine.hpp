//----------------------------------------------------------------------------
// $Id: HtpSixEngine.hpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#ifndef HTPSIXENGINE_H
#define HTPSIXENGINE_H

#include "HtpHexEngine.hpp"
#include "Six.hpp"

#include "sixplayer.h"
#include "hexboard.h"
#include "hexgame.h"
#include "hexmark.h"


//----------------------------------------------------------------------------

class HtpSixEngine: public HtpHexEngine
{
public:

    HtpSixEngine(std::istream& in, std::ostream& out, Six& program);
    
    ~HtpSixEngine();

    /** @name Command Callbacks */
    // @{
    // The callback functions are documented in the cpp file

    virtual void CmdUndo(HtpCommand& cmd);
    virtual void CmdScoreForLastMove(HtpCommand& cmd);

    virtual void CmdVCBuild(HtpCommand& cmd);
    virtual void CmdGetCellsConnectedTo(HtpCommand& cmd);
    virtual void CmdGetVCsBetween(HtpCommand& cmd);
    
    // @} // @name

protected:

    /** Generates a move. */
    HexPoint GenMove(HexColor color, double time_reminaing);
    
    /** Determine Six's level of play. */
    SixPlayer::Level GetSixSkillLevel();
    
    /** Convert to Six colors and moves. */
    HexMark SixColor(HexColor color);
    HexMove SixMove(HexColor color, HexPoint move);
    HexPoint SixFieldToHexPoint(HexField field);
    HexField HexPointToSixPoint(HexPoint move);
    HexPoint SixMoveToHexPoint(HexMove move);

    bool VcArg(const std::string& arg);

    /** Plays a move.  */
    virtual void Play(HexColor color, HexPoint move);
    virtual void NewGame(int width, int height);

    HexBoard* m_sixboard;
    HexGame* m_sixgame;
    SixPlayer* m_sixplayer;

    Six& m_six;

    Connector* m_con[BLACK_AND_WHITE];

    static const int DEFAULT_BOARDSIZE = 11;

private:
    void RegisterCmd(const std::string& name,
                     GtpCallback<HtpSixEngine>::Method method);
};

//----------------------------------------------------------------------------

#endif // HTPHEXENGINE_H
