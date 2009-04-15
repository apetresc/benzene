//----------------------------------------------------------------------------
// $Id: HtpHexEngine.cpp 1699 2008-09-24 23:55:56Z broderic $
//----------------------------------------------------------------------------

#include "SgSystem.h"

#include <cmath>
#include <iomanip>
#include <sstream>
#include <limits>
#include <time.h>
#include <signal.h>
#include <iostream>

#include "SgGameReader.h"
#include "SgNode.h"
#include "SgTimer.h"

#include "HexSgUtil.hpp"
#include "HtpHexEngine.hpp"
#include "GroupBoard.hpp"
#include "BitsetIterator.hpp"
#include "BoardUtils.hpp"
#include "Time.hpp"

//----------------------------------------------------------------------------

HtpHexEngine::HtpHexEngine(std::istream& in, 
                           std::ostream& out, 
                           HexProgram &program)
    : GtpEngine(in, out),
      m_board(hex::settings.get_int("game-default-boardsize")),
      m_game(new Game(m_board)),
      m_program(program)
{
    RegisterCmd("name", &HtpHexEngine::CmdName);
    RegisterCmd("version", &HtpHexEngine::CmdVersion);
    RegisterCmd("list_settings", &HtpHexEngine::CmdListSettings);
    RegisterCmd("play", &HtpHexEngine::CmdPlay);
    RegisterCmd("genmove", &HtpHexEngine::CmdGenMove);
    RegisterCmd("undo", &HtpHexEngine::CmdUndo);
    RegisterCmd("boardsize", &HtpHexEngine::CmdNewGame);
    RegisterCmd("showboard", &HtpHexEngine::CmdShowboard);
    RegisterCmd("board_id", &HtpHexEngine::CmdBoardID);
    RegisterCmd("time_left", &HtpHexEngine::CmdTimeLeft);
    RegisterCmd("final_score", &HtpHexEngine::CmdFinalScore);
    RegisterCmd("loadsgf", &HtpHexEngine::CmdLoadSgf);
    RegisterCmd("all_legal_moves", &HtpHexEngine::CmdAllLegalMoves);

    NewGame(m_board.width(), m_board.height());
}

HtpHexEngine::~HtpHexEngine()
{
    delete m_game;
}

//----------------------------------------------------------------------------

void HtpHexEngine::RegisterCmd(const std::string& name,
                               GtpCallback<HtpHexEngine>::Method method)
{
    Register(name, new GtpCallback<HtpHexEngine>(this, method));
}


HexColor HtpHexEngine::ColorArg(const HtpCommand& cmd, std::size_t number) const
{
    std::string value = cmd.ArgToLower(number);
    if (value == "e" || value == "empty")
            return EMPTY;
    if (value == "b" || value == "black")
	    return BLACK;
    if (value == "w" || value == "white")
	    return WHITE;
    throw HtpFailure() << "argument " << (number + 1) << " must be color";
}

HexPoint HtpHexEngine::MoveArg(const HtpCommand& cmd, std::size_t number) const
{
    return HexPointUtil::fromString(cmd.ArgToLower(number));
}

void HtpHexEngine::Play(HexColor color, HexPoint move)
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
    }
}

HexPoint HtpHexEngine::GenMove(HexColor UNUSED(color), 
                               double UNUSED(time_remaining))
{
    return BoardUtils::RandomEmptyCell(m_game->Board());
}
 
void HtpHexEngine::NewGame(int width, int height)
{
    if (width != m_game->Board().width() || 
        height != m_game->Board().height()) 
    {
        delete m_game;
        m_board = StoneBoard(width, height);
        m_game = new Game(m_board);
    } else {
        m_game->NewGame();
    }

    SetTimeRemaining(BLACK, hex::settings.get_double("game-total-time"));
    SetTimeRemaining(WHITE, hex::settings.get_double("game-total-time"));
}

void HtpHexEngine::SetTimeRemaining(HexColor color, double time)
{
    m_timeRemaining[color] = time;
}

void HtpHexEngine::ReduceTimeRemaining(HexColor color, double time)
{
    m_timeRemaining[color] -= time;
}

double HtpHexEngine::TimeRemaining(HexColor color) const
{
    return m_timeRemaining[color];
}

void HtpHexEngine::PrintBitsetToHTP(HtpCommand& cmd, const bitset_t& bs) const
{
    int c=0;
    for (BitsetIterator i(bs); i; ++i) {
        cmd << " " << HexPointUtil::toString(*i);
        if ((++c % 10) == 0) cmd << "\n";
    }
}

////////////////////////////////////////////////////////////////////////
// Commands
////////////////////////////////////////////////////////////////////////

void HtpHexEngine::CmdName(HtpCommand& cmd)
{
    cmd << m_program.getName();
}

void HtpHexEngine::CmdVersion(HtpCommand& cmd)
{
    cmd << m_program.getVersion() << "." << m_program.getBuild();
}

/** Lists all hex::settings and the values. */
void HtpHexEngine::CmdListSettings(HtpCommand& cmd)
{
    const Settings::SettingsMap& lst = hex::settings.GetSettings();
    Settings::SettingsMap::const_iterator si = lst.begin();
    cmd << '\n';
    for (; si != lst.end(); ++si) {
        std::string name = si->first;
        cmd << name << " = " << si->second << '\n';
    }
}

/** Starts new game with the given board size. */
void HtpHexEngine::CmdNewGame(HtpCommand& cmd)
{
    cmd.CheckNuArg(2);
    NewGame(cmd.IntArg(0,1,14), cmd.IntArg(1,1,14));
}

/** Plays a move. */
void HtpHexEngine::CmdPlay(HtpCommand& cmd)
{
    cmd.CheckNuArg(2);
    Play(ColorArg(cmd, 0), MoveArg(cmd, 1));
}

/** Generates a move and handles time remaining. If engine exceeds
    time remaining, returns RESIGN. */
void HtpHexEngine::CmdGenMove(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor color = ColorArg(cmd, 0);

    SgTime::SetDefaultMode(SG_TIME_REAL);
    SgTimer timer;
    timer.Start();
    HexPoint move = GenMove(color, TimeRemaining(color));
    timer.Stop();

    ReduceTimeRemaining(color, timer.GetTime());

    if (TimeRemaining(color) < 0) {
        hex::log << hex::warning << "**** FLAG DROPPED ****" << hex::endl;
        if (hex::settings.get_bool("player-resign-on-time")) {
            move = RESIGN;
        }
    }

    Play(color, move);
    cmd << HexPointUtil::toString(move);
}


/** Undo the last move. */
void HtpHexEngine::CmdUndo(HtpCommand& cmd)
{
    cmd.CheckNuArg(0);
    m_game->UndoMove();
}

/** Displays the board. */
void HtpHexEngine::CmdShowboard(HtpCommand& cmd)
{
    cmd << "\n";
    cmd << m_game->Board();
}

void HtpHexEngine::CmdBoardID(HtpCommand& cmd)
{
    cmd.CheckNuArg(0);
    cmd << m_game->Board().GetBoardIDString();
}

void HtpHexEngine::CmdTimeLeft(HtpCommand& cmd)
{
    cmd.CheckNuArgLessEqual(2);
    if (cmd.NuArg() == 0) {
        cmd << "Black: " << FormattedTime(TimeRemaining(BLACK)) << ", "
            << "White: " << FormattedTime(TimeRemaining(WHITE));
    }
    else if (cmd.NuArg() == 1) {
        HexColor color = ColorArg(cmd, 0);
        cmd << FormattedTime(TimeRemaining(color));
    } 
    else {
        HexColor color = ColorArg(cmd, 0);
        SetTimeRemaining(color, cmd.IntArg(1));
    }
}

/** Return a string with what we think the outcome of the game is.
 *          Black win:  B+ 
 *         White  win:  W+ 
 */
void HtpHexEngine::CmdFinalScore(HtpCommand& cmd)
{
    GroupBoard brd(m_game->Board());
    brd.absorb();
    HexColor winner = brd.getWinner();
    std::string ret = "cannot score";
    if (winner == BLACK)
        ret = "B+";
    else if (winner == WHITE)
        ret = "W+";
    cmd << ret;
}

/** Returns a list of all legal moves. */
void HtpHexEngine::CmdAllLegalMoves(HtpCommand& cmd)
{
    int c = 0;
    bitset_t legal = m_game->Board().getLegal();
    for (BitsetIterator i(legal); i; ++i) {
        cmd << " " << HexPointUtil::toString(*i);
        if ((++c % 10) == 0) cmd << "\n";
    }
}

/** @bug This won't work if we're overwriting previosly played stones! */
void HtpHexEngine::SetPosition(const SgNode* node)
{
    std::vector<HexPoint> black, white, empty;
    HexSgUtil::GetSetupPosition(node, m_game->Board().height(), 
                                black, white, empty);
    for (unsigned i=0; ; ++i) {
        bool bdone = (i >= black.size());
        bool wdone = (i >= white.size());
        if (!bdone) Play(BLACK, black[i]);
        if (!wdone) Play(WHITE, white[i]);
        if (bdone && wdone) break;
    }
}

void HtpHexEngine::CmdLoadSgf(HtpCommand& cmd)
{
    cmd.CheckNuArgLessEqual(2);
    std::string filename = cmd.Arg(0);
    int movenumber = 1024;
    if (cmd.NuArg() == 2) 
        movenumber = cmd.IntArg(1, 0);

    std::ifstream file(filename.c_str());
    if (!file) {
        throw HtpFailure() << "cannot load file";
        return;
    }

    SgGameReader sgreader(file, 11);
    SgNode* root = sgreader.ReadGame(); 
    if (root == 0) {
        throw HtpFailure() << "cannot load file";
        return;
    }
    sgreader.PrintWarnings(std::cerr);

    int size = root->GetIntProp(SG_PROP_SIZE);

    NewGame(size, size);

    const StoneBoard& brd = m_game->Board();

    if (HexSgUtil::NodeHasSetupInfo(root)) {
        hex::log << hex::warning << "Root has setup info!" << hex::endl;
        SetPosition(root);
    }

    // play movenumber moves; stop if we hit the end of the game
    SgNode* cur = root;
    for (int mn=0; mn<movenumber;) {
        cur = cur->NodeInDirection(SgNode::NEXT);
        if (!cur) break;

        if (HexSgUtil::NodeHasSetupInfo(cur)) {
            SetPosition(cur);
            continue;
        } else if (!cur->HasNodeMove()) {
            continue;
        }

        HexColor color = HexSgUtil::SgColorToHexColor(cur->NodePlayer());
        HexPoint point = HexSgUtil::SgPointToHexPoint(cur->NodeMove(), 
                                                      brd.height());
        Play(color, point);
        mn++;
    }
}

//----------------------------------------------------------------------------
// Interrupt/Pondering

void HtpHexEngine::InitPonder()
{
}

void HtpHexEngine::Ponder()
{
}

void HtpHexEngine::StopPonder()
{
}

void HtpHexEngine::Interrupt()
{
}

//----------------------------------------------------------------------------
