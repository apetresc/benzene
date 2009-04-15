//----------------------------------------------------------------------------
// $Id: MoHexEngine.cpp 1899 2009-02-06 23:51:19Z broderic $
//----------------------------------------------------------------------------

#include "SgSystem.h"

#include "MoHexEngine.hpp"
#include "MoHexPlayer.hpp"

//----------------------------------------------------------------------------

namespace 
{
} // namespace

//----------------------------------------------------------------------------

MoHexEngine::MoHexEngine(std::istream& in, std::ostream& out,
                         int boardsize, BenzenePlayer& player)
    : BenzeneHtpEngine(in, out, boardsize, player)
{
    RegisterCmd("param_mohex", &MoHexEngine::MoHexParam);
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
