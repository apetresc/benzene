//----------------------------------------------------------------------------
// $Id: HtpSixEngine.cpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#include "SgSystem.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <limits>
#include <time.h>
#include <signal.h>
#include <iostream>

#include "HexSgUtil.hpp"
#include "HtpSixEngine.hpp"

#include "hexboard.h"
#include "hexgame.h"

//----------------------------------------------------------------------------

HtpSixEngine::HtpSixEngine(std::istream& in, 
                           std::ostream& out, 
                           Six& program)
    : HtpHexEngine(in, out, program),
      m_sixboard(new HexBoard(hex::settings.get_int("game-default-boardsize"),
                              hex::settings.get_int("game-default-boardsize"))),
      m_sixgame(new HexGame(*m_sixboard, 
                            HEX_MARK_VERT, 
                            hex::settings.get_bool("game-allow-swap"))),
      m_sixplayer(new SixPlayer(GetSixSkillLevel())),
      m_six(program)
{
    RegisterCmd("score_for_last_move", &HtpSixEngine::CmdScoreForLastMove);
    
    RegisterCmd("vc-build", &HtpSixEngine::CmdVCBuild);
    RegisterCmd("vc-connected-to", &HtpSixEngine::CmdGetCellsConnectedTo);
    RegisterCmd("vc-between-cells", &HtpSixEngine::CmdGetVCsBetween);
}

HtpSixEngine::~HtpSixEngine()
{
}

//----------------------------------------------------------------------------

void HtpSixEngine::RegisterCmd(const std::string& name,
                               GtpCallback<HtpSixEngine>::Method method)
{
    Register(name, new GtpCallback<HtpSixEngine>(this, method));
}

//----------------------------------------------------------------------------

SixPlayer::Level HtpSixEngine::GetSixSkillLevel()
{
    std::string skill = hex::settings.get("skill-level");
    if (skill == "beginner")
        return SixPlayer::BEGINNER;
    if (skill == "intermediate")
        return SixPlayer::INTERMEDIATE;
    if (skill == "advanced")
        return SixPlayer::ADVANCED;
    return SixPlayer::EXPERT;
}

HexMark HtpSixEngine::SixColor(HexColor color)
{
    if (color == BLACK)
        return HEX_MARK_VERT;
    if (color == WHITE)
        return HEX_MARK_HORI;
    return HEX_MARK_EMPTY;
}

HexField HtpSixEngine::HexPointToSixPoint(HexPoint move)
{
    if (move == NORTH)
        return HexBoard::TOP_EDGE;
    else if (move == SOUTH)
        return HexBoard::BOTTOM_EDGE;
    else if (move == WEST)
        return HexBoard::LEFT_EDGE;
    else if (move == EAST)
        return HexBoard::RIGHT_EDGE;

    int x, y;
    HexPointUtil::pointToCoords(move, x, y);
    return m_sixboard->coords2Field(x, y);
}

HexMove HtpSixEngine::SixMove(HexColor color, HexPoint move)
{
    HexMark mark = SixColor(color);

    if (HexPointUtil::isSwap(move)) 
        return HexMove::createSwap(mark);
    if (move == RESIGN)
        return HexMove::createResign(mark);

    return HexMove(mark, HexPointToSixPoint(move));
}

HexPoint HtpSixEngine::SixFieldToHexPoint(HexField field)
{
   if (field == HexBoard::TOP_EDGE)
        return NORTH;
    else if (field == HexBoard::BOTTOM_EDGE)
        return SOUTH;
    else if (field == HexBoard::LEFT_EDGE)
        return WEST;
    else if (field == HexBoard::RIGHT_EDGE)
        return EAST;

    int x, y;
    m_sixboard->field2Coords(field, &x, &y);
    return HexPointUtil::coordsToPoint(x, y);
}

HexPoint HtpSixEngine::SixMoveToHexPoint(HexMove move)
{
    if (move.isSwap())
        return SWAP_PIECES;
    if (move.isResign())
        return RESIGN;

    return SixFieldToHexPoint(move.field());
 }

void HtpSixEngine::Play(HexColor color, HexPoint move)
{
    bool illegal = false;
    std::string reason = "";

    // do nothing if a resign move
    if (move == RESIGN)
        return;

    Game::ReturnType result = m_game->PlayMove(color, move);
    if (result == Game::INVALID_MOVE) {
        illegal = true;
        reason = " (invalid)";
    } else if (result == Game::OCCUPIED_CELL) {
        illegal = true;
        reason = " (occupied)";
    }
    
    if (illegal) {
        throw HtpFailure() << "illegal move: " << ' '
                           << HexColorUtil::toString(color) << ' ' 
                           << HexPointUtil::toString(move) << reason;
    } else {
        
        int milliseconds = 0;
        m_sixgame->play(SixMove(color, move), milliseconds);

    }
}

bool HtpSixEngine::VcArg(const std::string& arg)
{
    if (arg == "1") return true;
    if (arg == "semi") return true;
    return false;
}

void HtpSixEngine::NewGame(int width, int height)
{
    HtpHexEngine::NewGame(width, height);

    m_sixboard = new HexBoard(width, height);
    m_sixgame = new HexGame(*m_sixboard, 
                            HEX_MARK_VERT, 
                            hex::settings.get_bool("game-allow-swap"));
    m_sixplayer = new SixPlayer(GetSixSkillLevel());

}

HexPoint HtpSixEngine::GenMove(HexColor color, 
                               double UNUSED(time_remaining))
{
    HexMark mark = SixColor(color);

    // check if the game is over and resign if so
    if (m_sixgame->board().winner() != HEX_MARK_EMPTY) {
        return RESIGN;
    }

    // get six to generate a move
    m_sixplayer->init(m_sixgame, mark);
    
    std::pair<bool, HexMove> ret;
    ret = m_sixplayer->play();
    HexPoint point = SixMoveToHexPoint(ret.second);

    return point;
}

////////////////////////////////////////////////////////////////////////
// Commands
////////////////////////////////////////////////////////////////////////

/** Undo the last move. */
void HtpSixEngine::CmdUndo(HtpCommand& cmd)
{
    std::cerr << "six::undo" << std::endl;
    cmd.CheckNuArg(0);
    m_game->UndoMove();
    m_sixgame->back();
}

/** Returns score of last generated move--always zero for now. */
void HtpSixEngine::CmdScoreForLastMove(HtpCommand& cmd)
{
    cmd << 0;
}

//----------------------------------------------------------------------------

int g_doOrs;

void HtpSixEngine::CmdVCBuild(HtpCommand& cmd)
{

    static Poi<Connector::DualBatchLimiter> limiter =
        Poi<Connector::DualBatchLimiter>
        (new Connector::SoftLimiter(MAXINT, MAXINT, 50, MAXINT));
    
   
    for (BWIterator it; it; ++it) {
        HexMark mark = SixColor(*it);

        m_con[*it] = new Connector(limiter, 4, true, false);
        m_con[*it]->init(m_sixgame->board(), mark, false);
        m_con[*it]->calc();

    }
}

void HtpSixEngine::CmdGetCellsConnectedTo(HtpCommand& cmd)
{
    HexPoint p1 = MoveArg(cmd, 0);
    HexColor color = ColorArg(cmd, 1);
    bool semis = VcArg(cmd.ArgToLower(2));

    HexField sp1 = HexPointToSixPoint(p1);
    
    const Grouping& g = m_con[color]->grouping();
    Poi<Group> pg1 = g(sp1);
    const Group* g1 = &(*pg1);

    const Connector::DualBatchMap& dbm = m_con[color]->connections();
    Connector::DualBatchMap::const_iterator cur = dbm.begin();
    Connector::DualBatchMap::const_iterator end = dbm.end();

    bitset_t ret;
    while (cur != end) {
        GroupPair pair = cur->first;
        if (pair.minGroup() == g1 || pair.maxGroup() == g1) {
            Group* g2 = (pair.minGroup() == g1) 
                ? pair.maxGroup() : pair.minGroup();
               
            DualBatch db = cur->second;
            const Batch& batch = (semis) ? db.semiBatch() : db.connBatch();

            if (batch.size()) {
                const vector<HexField>& fields = g2->fields();
                for (int i=0; i<fields.size(); ++i) {
                    ret.set(SixFieldToHexPoint(fields[i]));
                }
            }
        }
        cur++;
    }

    PrintBitsetToHTP(cmd, ret);
}

void HtpSixEngine::CmdGetVCsBetween(HtpCommand& cmd)
{
    cmd.CheckNuArg(4);
    HexPoint p1 = MoveArg(cmd, 0);
    HexPoint p2 = MoveArg(cmd, 1);
    HexColor color = ColorArg(cmd, 2);
    bool semis = VcArg(cmd.ArgToLower(3));

    const Grouping& g = m_con[color]->grouping();
        
    HexField sp1 = HexPointToSixPoint(p1);
    HexField sp2 = HexPointToSixPoint(p2);
    Poi<Group> pg1 = g(sp1);
    Poi<Group> pg2 = g(sp2);
    const Group* g1 = &(*pg1);
    const Group* g2 = &(*pg2);

    const Connector::DualBatchMap& dbm = m_con[color]->connections();
    Connector::DualBatchMap::const_iterator cur = dbm.begin();
    Connector::DualBatchMap::const_iterator end = dbm.end();

    bitset_t ret;
    while (cur != end) {
        GroupPair pair = cur->first;
        if ((pair.minGroup() == g1 && pair.maxGroup() == g2) ||
            (pair.minGroup() == g2 && pair.maxGroup() == g1)) 
        {
            DualBatch db = cur->second;
            const Batch& batch = (semis) ? db.semiBatch() : db.connBatch();
            
            Batch::Iterator curc = batch.begin();
            Batch::Iterator endc = batch.end();
            while (curc != endc) {
                Carrier car = *curc;
                const vector<HexField>& fields = car.fields();

                cmd << "\n";
                cmd << HexPointUtil::toString(p1) << " ";
                cmd << HexPointUtil::toString(p2) << " ";
                cmd << HexColorUtil::toString(color) << " ";
                cmd << cmd.ArgToLower(3) << " ";
                cmd << "all ";
                cmd << " 0 ";
                
                cmd << "[";
                for (int i=0; i<fields.size(); ++i) {
                    cmd << " " << HexPointUtil::toString(SixFieldToHexPoint(fields[i]));
                }
                
                cmd << " ]";

                ++curc;
            }
            break;
        }
        cur++;
    }

    cmd << "\n";
}

//----------------------------------------------------------------------------
