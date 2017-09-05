//----------------------------------------------------------------------------
/** @file MoHexEngine.cpp */
//----------------------------------------------------------------------------

#include "SgSystem.h"

#include "BitsetIterator.hpp"
#include "MoHexEngine.hpp"
#include "MoHexPlayer.hpp"
#include "PlayAndSolve.hpp"
#include "SwapCheck.hpp"

using namespace benzene;

//----------------------------------------------------------------------------

namespace {

//----------------------------------------------------------------------------

std::string KnowledgeThresholdToString(const std::vector<SgUctValue>& t)
{
    if (t.empty())
        return "0";
    std::ostringstream os;
    os << '\"';
    for (std::size_t i = 0; i < t.size(); ++i)
    {
        if (i > 0) 
            os << ' ';
        os << t[i];
    }
    os << '\"';
    return os.str();
}

std::vector<SgUctValue> KnowledgeThresholdFromString(const std::string& val)
{
    std::vector<SgUctValue> v;
    std::istringstream is(val);
    SgUctValue t;
    while (is >> t)
        v.push_back(t);
    if (v.size() == 1 && v[0] == 0)
        v.clear();
    return v;
}

SgUctMoveSelect MoveSelectArg(const HtpCommand& cmd, size_t number)
{
    std::string arg = cmd.ArgToLower(number);
    if (arg == "value")
        return SG_UCTMOVESELECT_VALUE;
    if (arg == "count")
        return SG_UCTMOVESELECT_COUNT;
    if (arg == "bound")
        return SG_UCTMOVESELECT_BOUND;
    if (arg == "estimate")
        return SG_UCTMOVESELECT_ESTIMATE;
    throw HtpFailure() << "unknown move select argument \"" << arg << '"';
}

std::string MoveSelectToString(SgUctMoveSelect moveSelect)
{
    switch (moveSelect)
    {
    case SG_UCTMOVESELECT_VALUE:
        return "value";
    case SG_UCTMOVESELECT_COUNT:
        return "count";
    case SG_UCTMOVESELECT_BOUND:
        return "bound";
    case SG_UCTMOVESELECT_ESTIMATE:
        return "estimate";
    default:
        return "?";
    }
}

//----------------------------------------------------------------------------

} // namespace

//----------------------------------------------------------------------------

MoHexEngine::MoHexEngine(int boardsize, MoHexPlayer& player)
    : CommonHtpEngine(boardsize),
      m_player(player), 
      m_book(0),
      m_bookCheck(m_book),
      m_bookCommands(m_game, m_pe, m_book, m_bookCheck, m_player)
{
    m_bookCommands.Register(*this);
    RegisterCmd("param_mohex", &MoHexEngine::MoHexParam);
    RegisterCmd("param_mohex_policy", &MoHexEngine::MoHexPolicyParam);
    RegisterCmd("mohex-save-tree", &MoHexEngine::SaveTree);
    RegisterCmd("mohex-save-games", &MoHexEngine::SaveGames);
    RegisterCmd("mohex-get-pv", &MoHexEngine::GetPV);
    RegisterCmd("mohex-values", &MoHexEngine::Values);
    RegisterCmd("mohex-rave-values", &MoHexEngine::RaveValues);
    RegisterCmd("mohex-bounds", &MoHexEngine::Bounds);
    RegisterCmd("mohex-find-top-moves", &MoHexEngine::FindTopMoves);
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

double MoHexEngine::TimeForMove(HexColor color)
{
    if (m_player.UseTimeManagement())
        return m_game.TimeRemaining(color) * 0.08;
    return m_player.MaxTime();
}

HexPoint MoHexEngine::GenMove(HexColor color, bool useGameClock)
{
    SG_UNUSED(useGameClock);
    if (SwapCheck::PlaySwap(m_game, color))
        return SWAP_PIECES;
    HexPoint bookMove = m_bookCheck.BestMove(HexState(m_game.Board(), color));
    if (bookMove != INVALID_POINT)
        return bookMove;
    double maxTime = TimeForMove(color);
    return DoSearch(color, maxTime);
}

HexPoint MoHexEngine::DoSearch(HexColor color, double maxTime)
{
    HexState state(m_game.Board(), color);
    if (m_useParallelSolver)
    {
        PlayAndSolve ps(*m_pe.brd, *m_se.brd, m_player, m_dfpnSolver, 
                        m_dfpnPositions, m_game);
        return ps.GenMove(state, maxTime);
    }
    else
    {
        double score;
        return m_player.GenMove(state, m_game, m_pe.SyncBoard(m_game.Board()),
                                maxTime, score);
    }
}

//----------------------------------------------------------------------------

void MoHexEngine::CmdAnalyzeCommands(HtpCommand& cmd)
{
    CommonHtpEngine::CmdAnalyzeCommands(cmd);
    m_bookCommands.AddAnalyzeCommands(cmd);
    cmd << 
        "param/MoHex Param/param_mohex\n"
        "param/MoHex Policy Param/param_mohex_policy\n"
        "none/MoHex Save Tree/mohex-save-tree %w\n"
        "none/MoHex Save Games/mohex-save-games %w\n"
        "var/MoHex PV/mohex-get-pv %m\n"
        "pspairs/MoHex Values/mohex-values\n"
        "pspairs/MoHex Rave Values/mohex-rave-values\n"
        "pspairs/MoHex Bounds/mohex-bounds\n"
        "pspairs/MoHex Top Moves/mohex-find-top-moves %c\n";
}

void MoHexEngine::MoHexPolicyParam(HtpCommand& cmd)
{
    MoHexPlayoutPolicyConfig& config = m_player.SharedPolicy().Config();
    if (cmd.NuArg() == 0)
    {
        cmd << '\n'
            << "pattern_check_percent "
            << config.patternCheckPercent << '\n'
            << "pattern_heuristic "
            << config.patternHeuristic << '\n'
            << "response_heuristic "
            << config.responseHeuristic << '\n'
            << "response_threshold "
            << config.responseThreshold << '\n';
    }
    else if (cmd.NuArg() == 2)
    {
        std::string name = cmd.Arg(0);
        if (name == "pattern_check_percent")
            config.patternCheckPercent = cmd.ArgMinMax<int>(1, 0, 100);
        else if (name == "pattern_heuristic")
            config.patternHeuristic = cmd.Arg<bool>(1);
        else if (name == "response_heuristic")
            config.responseHeuristic = cmd.Arg<bool>(1);
        else if (name == "response_threshold")
            config.responseThreshold = cmd.ArgMin<std::size_t>(1, 0);
        else
            throw HtpFailure("Unknown option!");
    }
    else
        throw HtpFailure("Expected 0 or 2 arguments!");
}

void MoHexEngine::MoHexParam(HtpCommand& cmd)
{
    MoHexSearch& search = m_player.Search();
    if (cmd.NuArg() == 0) 
    {
        cmd << '\n'
            << "[bool] backup_ice_info "
            << m_player.BackupIceInfo() << '\n'
#if HAVE_GCC_ATOMIC_BUILTINS
            << "[bool] lock_free " 
            << search.LockFree() << '\n'
#endif
            << "[bool] keep_games "
            << search.KeepGames() << '\n'
            << "[bool] perform_pre_search " 
            << m_player.PerformPreSearch() << '\n'
            << "[bool] ponder "
            << m_player.Ponder() << '\n'
            << "[bool] reuse_subtree " 
            << m_player.ReuseSubtree() << '\n'
            << "[bool] search_singleton "
            << m_player.SearchSingleton() << '\n'
            << "[bool] use_livegfx "
            << search.LiveGfx() << '\n'
            << "[bool] use_parallel_solver "
            << m_useParallelSolver << '\n'
            << "[bool] use_rave "
            << search.Rave() << '\n'
            << "[bool] use_time_management "
            << m_player.UseTimeManagement() << '\n'
            << "[bool] weight_rave_updates "
            << search.WeightRaveUpdates() << '\n'
            << "[bool] virtual_loss "
            << search.VirtualLoss() << '\n'
            << "[string] bias_term "
            << search.BiasTermConstant() << '\n'
            << "[string] expand_threshold "
            << search.ExpandThreshold() << '\n'
            << "[string] fillin_map_bits "
            << search.FillinMapBits() << '\n'
            << "[string] knowledge_threshold "
            << KnowledgeThresholdToString(search.KnowledgeThreshold()) << '\n'
            << "[string] number_playouts_per_visit "
            << search.NumberPlayouts() << '\n'
            << "[string] max_games "
            << m_player.MaxGames() << '\n'
            << "[string] max_memory "
            << search.MaxNodes() * 2 * sizeof(SgUctNode) << '\n'
            << "[string] max_nodes "
            << search.MaxNodes() << '\n'
            << "[string] max_time "
            << m_player.MaxTime() << '\n'
            << "[string] move_select "
            << MoveSelectToString(search.MoveSelect()) << '\n'
#if HAVE_GCC_ATOMIC_BUILTINS
            << "[string] num_threads "
            << search.NumberThreads() << '\n'
#endif
            << "[string] playout_update_radius "
            << search.PlayoutUpdateRadius() << '\n'
            << "[string] randomize_rave_frequency "
            << search.RandomizeRaveFrequency() << '\n'
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
            m_player.SetBackupIceInfo(cmd.Arg<bool>(1));
#if HAVE_GCC_ATOMIC_BUILTINS
        else if (name == "lock_free")
            search.SetLockFree(cmd.Arg<bool>(1));
#endif
        else if (name == "keep_games")
            search.SetKeepGames(cmd.Arg<bool>(1));
        else if (name == "perform_pre_search")
            m_player.SetPerformPreSearch(cmd.Arg<bool>(1));
        else if (name == "ponder")
            m_player.SetPonder(cmd.Arg<bool>(1));
        else if (name == "use_livegfx")
            search.SetLiveGfx(cmd.Arg<bool>(1));
        else if (name == "use_rave")
            search.SetRave(cmd.Arg<bool>(1));
        else if (name == "randomize_rave_frequency")
            search.SetRandomizeRaveFrequency(cmd.ArgMin<int>(1, 0));
        else if (name == "reuse_subtree")
           m_player.SetReuseSubtree(cmd.Arg<bool>(1));
        else if (name == "bias_term")
            search.SetBiasTermConstant(cmd.Arg<float>(1));
        else if (name == "expand_threshold")
            search.SetExpandThreshold(cmd.ArgMin<int>(1, 0));
        else if (name == "knowledge_threshold")
            search.SetKnowledgeThreshold
                (KnowledgeThresholdFromString(cmd.Arg(1)));
        else if (name == "fillin_map_bits")
            search.SetFillinMapBits(cmd.ArgMin<int>(1, 1));
        else if (name == "max_games")
            m_player.SetMaxGames(cmd.ArgMin<int>(1, 1));
        else if (name == "max_memory")
            search.SetMaxNodes(cmd.ArgMin<std::size_t>(1, 1) 
                               / sizeof(SgUctNode) / 2);
        else if (name == "max_time")
            m_player.SetMaxTime(cmd.Arg<float>(1));
        else if (name == "max_nodes")
            search.SetMaxNodes(cmd.ArgMin<std::size_t>(1, 1));
        else if (name == "move_select")
            search.SetMoveSelect(MoveSelectArg(cmd, 1));
#if HAVE_GCC_ATOMIC_BUILTINS
        else if (name == "num_threads")
            search.SetNumberThreads(cmd.ArgMin<int>(1, 1));
#endif
        else if (name == "number_playouts_per_visit")
            search.SetNumberPlayouts(cmd.ArgMin<int>(1, 1));
        else if (name == "playout_update_radius")
            search.SetPlayoutUpdateRadius(cmd.ArgMin<int>(1, 0));
        else if (name == "rave_weight_final")
            search.SetRaveWeightFinal(cmd.ArgMin<float>(1, 0));
        else if (name == "rave_weight_initial")
            search.SetRaveWeightInitial(cmd.ArgMin<float>(1, 0));
        else if (name == "weight_rave_updates")
            search.SetWeightRaveUpdates(cmd.Arg<bool>(1));
        else if (name == "tree_update_radius")
            search.SetTreeUpdateRadius(cmd.ArgMin<int>(1, 0));
        else if (name == "search_singleton")
            m_player.SetSearchSingleton(cmd.Arg<bool>(1));
        else if (name == "use_parallel_solver")
            m_useParallelSolver = cmd.Arg<bool>(1);
        else if (name == "use_time_management")
            m_player.SetUseTimeManagement(cmd.Arg<bool>(1));
        else if (name == "virtual_loss")
            search.SetVirtualLoss(cmd.Arg<bool>(1));
        else
            throw HtpFailure() << "Unknown parameter: " << name;
    }
    else 
        throw HtpFailure("Expected 0 or 2 arguments");
}

/** Saves the search tree from the previous search to the specified
    file.  The optional second parameter sets the max depth to
    output. If not given, entire tree is saved.    
*/
void MoHexEngine::SaveTree(HtpCommand& cmd)
{
    MoHexSearch& search = m_player.Search();

    cmd.CheckNuArg(1);
    std::string filename = cmd.Arg(0);
    int maxDepth = -1;
    std::ofstream file(filename.c_str());
    if (!file)
        throw HtpFailure() << "Could not open '" << filename << "'";
    if (cmd.NuArg() == 2)
        maxDepth = cmd.ArgMin<int>(1, 0);
    search.SaveTree(file, maxDepth);
}

/** Saves games from last search to a SGF. */
void MoHexEngine::SaveGames(HtpCommand& cmd)
{
    MoHexSearch& search = m_player.Search();
    cmd.CheckNuArg(1);
    std::string filename = cmd.Arg(0);
    search.SaveGames(filename);
}

void MoHexEngine::Values(HtpCommand& cmd)
{
    MoHexSearch& search = m_player.Search();
    const SgUctTree& tree = search.Tree();
    for (SgUctChildIterator it(tree, tree.Root()); it; ++it)
    {
        const SgUctNode& child = *it;
        SgPoint p = child.Move();
        SgUctValue count = child.MoveCount();
        SgUctValue mean = 0.0;
        if (count > 0)
            mean = search.InverseEval(child.Mean());
        cmd << ' ' << static_cast<HexPoint>(p)
            << ' ' << std::fixed << std::setprecision(3) << mean
            << '@' << static_cast<std::size_t>(count);
    }
}

void MoHexEngine::RaveValues(HtpCommand& cmd)
{
    MoHexSearch& search = m_player.Search();
    const SgUctTree& tree = search.Tree();
    for (SgUctChildIterator it(tree, tree.Root()); it; ++it)
    {
        const SgUctNode& child = *it;
        SgPoint p = child.Move();
        if (p == SG_PASS || ! child.HasRaveValue())
            continue;
        cmd << ' ' << static_cast<HexPoint>(p)
            << ' ' << std::fixed << std::setprecision(3) << child.RaveValue();
    }
}

void MoHexEngine::GetPV(HtpCommand& cmd)
{
    MoHexSearch& search = m_player.Search();
    std::vector<SgMove> seq;
    search.FindBestSequence(seq);
    for (std::size_t i = 0; i < seq.size(); ++i)
        cmd << static_cast<HexPoint>(seq[i]) << ' ';
}

void MoHexEngine::Bounds(HtpCommand& cmd)
{
    MoHexSearch& search = m_player.Search();
    const SgUctTree& tree = search.Tree();
    for (SgUctChildIterator it(tree, tree.Root()); it; ++it)
    {
        const SgUctNode& child = *it;
        SgPoint p = child.Move();
        if (p == SG_PASS || ! child.HasRaveValue())
            continue;
        SgUctValue bound = search.GetBound(search.Rave(), tree.Root(), child);
        cmd << ' ' << static_cast<HexPoint>(p) 
            << ' ' << std::fixed << std::setprecision(3) << bound;
    }
}

void MoHexEngine::FindTopMoves(HtpCommand& cmd)
{
    HexColor color = HtpUtil::ColorArg(cmd, 0);
    int num = 5;
    if (cmd.NuArg() >= 2)
        num = cmd.ArgMin<int>(1, 1);
    HexState state(m_game.Board(), color);
    HexBoard& brd = m_pe.SyncBoard(m_game.Board());
    if (EndgameUtil::IsDeterminedState(brd, color))
        throw HtpFailure() << "State is determined.\n";
    bitset_t consider = EndgameUtil::MovesToConsider(brd, color);
    std::vector<HexPoint> moves;
    std::vector<double> scores;
    m_player.FindTopMoves(num, state, m_game, brd, consider, 
                          m_player.MaxTime(), moves, scores);
    for (std::size_t i = 0; i < moves.size(); ++i)
        cmd << ' ' << static_cast<HexPoint>(moves[i]) 
            << ' ' << (i + 1) 
            << '@' << std::fixed << std::setprecision(3) << scores[i];
}

//----------------------------------------------------------------------------
// Pondering

#if GTPENGINE_PONDER

void MoHexEngine::InitPonder()
{
    SgSetUserAbort(false);
}

void MoHexEngine::Ponder()
{
    if (!m_player.Ponder())
        return;
    if (!m_player.ReuseSubtree())
    {
        LogWarning() << "Pondering requires reuse_subtree.\n";
        return;
    }
    // Call genmove() after 0.2 seconds delay to avoid calls 
    // in very short intervals between received commands
    boost::xtime time;
    boost::xtime_get(&time, boost::TIME_UTC_);
    for (int i = 0; i < 200; ++i)
    {
        if (SgUserAbort())
            return;
        time.nsec += 1000000; // 1 msec
        boost::thread::sleep(time);
    }
    LogInfo() << "MoHexEngine::Ponder: start\n";
    // Search for at most 10 minutes
    // Force it to search even if root has a singleton consider set
    bool oldSingleton = m_player.SearchSingleton();
    m_player.SetSearchSingleton(true);
    DoSearch(m_game.Board().WhoseTurn(), 600);
    m_player.SetSearchSingleton(oldSingleton);
}

void MoHexEngine::StopPonder()
{
    SgSetUserAbort(true);
}

#endif // GTPENGINE_PONDER

//----------------------------------------------------------------------------

