//----------------------------------------------------------------------------
/** @file
 */
//----------------------------------------------------------------------------

#include "SgSystem.h"

#include "BitsetIterator.hpp"
#include "MoHexEngine.hpp"
#include "MoHexPlayer.hpp"

//----------------------------------------------------------------------------

namespace 
{
} // namespace

//----------------------------------------------------------------------------

MoHexEngine::MoHexEngine(std::istream& in, std::ostream& out,
                         int boardsize, BenzenePlayer& player)
    : BenzeneHtpEngine(in, out, boardsize, player),
      m_bookBuilder(*GetInstanceOf<MoHexPlayer>(&player))
{
    RegisterCmd("book-expand", &MoHexEngine::CmdBookExpand);
    RegisterCmd("book-priorities", &MoHexEngine::CmdBookPriorities);
    RegisterCmd("param_mohex", &MoHexEngine::MoHexParam);
    RegisterCmd("param_book", &MoHexEngine::CmdParamBook);
}

MoHexEngine::~MoHexEngine()
{
}

//----------------------------------------------------------------------------

void MoHexEngine::RegisterCmd(const std::string& name,
                              GtpCallback<MoHexEngine>::Method method)
{
    Register(name, new GtpCallback<MoHexEngine>(this, method));
}

//----------------------------------------------------------------------------

void MoHexEngine::MoHexParam(HtpCommand& cmd)
{
    MoHexPlayer* mohex = GetInstanceOf<MoHexPlayer>(&m_player);
    if (!mohex)
        throw HtpFailure("No MoHex instance!");

    HexUctSearch& search = mohex->Search();

    if (cmd.NuArg() == 0) 
    {
        cmd << '\n'
            << "[bool] backup_ice_info "
            << mohex->BackupIceInfo() << '\n'
            << "[bool] lock_free " 
            << search.LockFree() << '\n'
            << "[bool] use_livegfx "
            << search.LiveGfx() << '\n'
            << "[bool] use_rave "
            << search.Rave() << '\n'
            << "[string] bias_term "
            << search.BiasTermConstant() << '\n'
            << "[string] expand_threshold "
            << search.ExpandThreshold() << '\n'
            << "[string] livegfx_interval "
            << search.LiveGfxInterval() << '\n'
            << "[string] max_games "
            << mohex->MaxGames() << '\n'
            << "[string] max_nodes "
            << search.MaxNodes() << '\n'
            << "[string] max_time "
            << mohex->MaxTime() << '\n'
            << "[string] num_threads "
            << search.NumberThreads() << '\n'
            << "[string] playout_update_radius "
            << search.PlayoutUpdateRadius() << '\n'
            << "[string] rave_weight_final "
            << search.RaveWeightFinal() << '\n'
            << "[string] rave_weight_initial "
            << search.RaveWeightInitial() << '\n'
            << "[string] tree_update_radius " 
            << search.TreeUpdateRadius() << '\n';
    }
    else if (cmd.NuArg() == 2)
    {
        std::string name = cmd.Arg(0);
        if (name == "backup_ice_info")
            mohex->SetBackupIceInfo(cmd.BoolArg(1));
        else if (name == "lock_free")
            search.SetLockFree(cmd.BoolArg(1));
        else if (name == "use_livegfx")
            search.SetLiveGfx(cmd.BoolArg(1));
        else if (name == "use_rave")
            search.SetRave(cmd.BoolArg(1));
        else if (name == "bias_term")
            search.SetBiasTermConstant(cmd.FloatArg(1));
        else if (name == "expand_threshold")
            search.SetExpandThreshold(cmd.IntArg(1, 0));
        else if (name == "livegfx_interval")
            search.SetLiveGfxInterval(cmd.IntArg(1, 0));
        else if (name == "max_games")
            mohex->SetMaxGames(cmd.IntArg(1, 0));
        else if (name == "max_time")
            mohex->SetMaxTime(cmd.FloatArg(1));
        else if (name == "max_nodes")
            search.SetMaxNodes(cmd.IntArg(1, 0));
        else if (name == "num_threads")
            search.SetNumberThreads(cmd.IntArg(1, 0));
        else if (name == "playout_update_radius")
            search.SetPlayoutUpdateRadius(cmd.IntArg(1, 0));
        else if (name == "rave_weight_final")
            search.SetRaveWeightFinal(cmd.IntArg(1, 0));
        else if (name == "rave_weight_initial")
            search.SetRaveWeightInitial(cmd.IntArg(1, 0));
        else if (name == "tree_update_radius")
            search.SetTreeUpdateRadius(cmd.IntArg(1, 0));
    }
    else 
        throw HtpFailure("Expected 0 or 2 arguments");

#if 0
    if (hex::settings.get_bool("uct-enable-init")) {
	search.SetPriorKnowledge(new HexUctPriorKnowledgeFactory(config_dir));
	search.SetPriorInit(SG_UCTPRIORINIT_BOTH);
    }
#endif
    
}

void MoHexEngine::CmdParamBook(HtpCommand& cmd)
{
    if (cmd.NuArg() == 0)
    {
        cmd << '\n'
            << "[bool] use_widening " 
            << m_bookBuilder.UseWidening() << '\n'
            << "[string] alpha "
            << m_bookBuilder.Alpha() << '\n'
            << "[string] expand_width "
            << m_bookBuilder.ExpandWidth() << '\n'
            << "[string] expand_threshold " 
            << m_bookBuilder.ExpandThreshold() << '\n'
            << "[string] num_threads " 
            << m_bookBuilder.NumThreads() << '\n';
    }
    else if (cmd.NuArg() == 2)
    {
        std::string name = cmd.Arg(0);
        if (name == "alpha")
            m_bookBuilder.SetAlpha(cmd.FloatArg(1));
        else if (name == "expand_width")
            m_bookBuilder.SetExpandWidth(cmd.SizeTypeArg(1, 1));
        else if (name == "expand_threshold")
            m_bookBuilder.SetExpandThreshold(cmd.SizeTypeArg(1, 1));
        else if (name == "num_threads")
            m_bookBuilder.SetNumThreads(cmd.SizeTypeArg(1));
        else if (name == "use_widening")
            m_bookBuilder.SetUseWidening(cmd.BoolArg(1));
        else
            throw HtpFailure() << "unknown parameter: " << name;
    }
}

//----------------------------------------------------------------------------

/** Expands the current node in the current opening book.
    "book-expand [iterations]"
*/
void MoHexEngine::CmdBookExpand(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    int iterations = cmd.IntArg(0, 1);
    HexBoard& brd = m_pe.SyncBoard(m_game->Board());

    if (!StateMatchesBook(brd))
        return;

    m_bookBuilder.Expand(*m_book, brd, iterations);
}

void MoHexEngine::CmdBookPriorities(HtpCommand& cmd)
{
    HexBoard& brd = m_pe.SyncBoard(m_game->Board());
    HexColor color = brd.WhoseTurn();

    if (!StateMatchesBook(brd))
        return;

    OpeningBookNode parent;
    if (!m_book->GetNode(brd, parent))
        return;

    for (BitsetIterator p(brd.getEmpty()); p; ++p) 
    {
        brd.playMove(color, *p);
        OpeningBookNode succ;
        if (m_book->GetNode(brd, succ))
        {
            cmd << " " << *p;
            float priority = OpeningBookUtil::ComputePriority(brd, parent, 
                                               succ, m_bookBuilder.Alpha());
            float value = OpeningBook::InverseEval(succ.m_value);
            if (HexEvalUtil::IsWin(value))
                cmd << " W";
            else if (HexEvalUtil::IsLoss(value))
                cmd << " L";
            else
                cmd << " " << std::fixed << std::setprecision(1) << priority;
        }
        brd.undoMove(*p);
    }
}

//----------------------------------------------------------------------------
// Pondering

#if GTPENGINE_PONDER

void MoHexEngine::InitPonder()
{
}

void MoHexEngine::Ponder()
{
}

void MoHexEngine::StopPonder()
{
}

#endif // GTPENGINE_PONDER

//----------------------------------------------------------------------------