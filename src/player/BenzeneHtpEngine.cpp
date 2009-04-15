//----------------------------------------------------------------------------
// $Id: BenzeneHtpEngine.cpp 1878 2009-01-29 03:20:00Z broderic $
//----------------------------------------------------------------------------

#include "SgSystem.h"

#include "BoardUtils.hpp"
#include "BookCheck.hpp"
#include "BitsetIterator.hpp"
#include "Connections.hpp"
#include "EndgameCheck.hpp"
#include "GraphUtils.hpp"
#include "HandBookCheck.hpp"
#include "BenzeneHtpEngine.hpp"
#include "LadderCheck.hpp"
#include "OpeningBook.hpp"
#include "PlayerUtils.hpp"
#include "Resistance.hpp"
#include "Solver.hpp"
#include "SolverCheck.hpp"
#include "SolverDB.hpp"
#include "SwapCheck.hpp"
#include "TwoDistance.hpp"
#include "VCUtils.hpp"
#include "VulPreCheck.hpp"

#include <cmath>

//----------------------------------------------------------------------------

namespace {

//----------------------------------------------------------------------------

void ParamICE(ICEngine& ice, HtpCommand& cmd)
{
    if (cmd.NuArg() == 0)
    {
        cmd << '\n'
            << "[bool] backup_opponent_dead "
            << ice.BackupOpponentDead() << '\n'
            << "[bool] find_all_pattern_dominators "
            << ice.FindAllPatternDominators() << '\n'
            << "[bool] find_all_pattern_killers "
            << ice.FindAllPatternKillers() << '\n'
            << "[bool] find_permanently_inferior "
            << ice.FindPermanentlyInferior() << '\n'
            << "[bool] find_presimplicial_pairs " 
            << ice.FindPresimplicialPairs() << '\n'
            << "[bool] find_three_sided_dead_regions "
            << ice.FindThreeSidedDeadRegions() << '\n'
            << "[bool] iterative_dead_regions "
            << ice.IterativeDeadRegions() << '\n'
            << "[bool] use_hand_coded_patterns "
            << ice.UseHandCodedPatterns() << '\n';
    }
    else if (cmd.NuArg() == 2)
    {
        std::string name = cmd.Arg(0);
        if (name == "backup_opponent_dead")
            ice.SetBackupOpponentDead(cmd.BoolArg(1));
        else if (name == "find_all_pattern_dominators")
            ice.SetFindAllPatternDominators(cmd.BoolArg(1));
        else if (name == "find_all_pattern_killers")
            ice.SetFindAllPatternKillers(cmd.BoolArg(1));
        else if (name == "find_permanently_inferior")
            ice.SetFindPermanentlyInferior(cmd.BoolArg(1));
        else if (name == "find_presimplicial_pairs")
            ice.SetFindPresimplicialPairs(cmd.BoolArg(1));
        else if (name == "find_three_sided_dead_regions")
            ice.SetFindThreeSidedDeadRegions(cmd.BoolArg(1));
        else if (name == "iterative_dead_regions")
            ice.SetIterativeDeadRegions(cmd.BoolArg(1));
        else if (name == "use_hand_coded_patterns")
            ice.SetUseHandCodedPatterns(cmd.BoolArg(1));
        else
            throw HtpFailure() << "unknown parameter: " << name;
    }
}

void ParamVC(HexBoard& brd, HtpCommand& cmd)
{
    ConnectionBuilderParam& param = brd.Builder().Parameters();
    if (cmd.NuArg() == 0)
    {
        cmd << '\n'
            << "[bool] abort_on_winning_connection "
            << param.abort_on_winning_connection << '\n'
            << "[bool] and_over_edge "
            << param.and_over_edge << '\n'
            << "[bool] use_greedy_union "
            << param.use_greedy_union << '\n'
            << "[bool] use_patterns "
            << param.use_patterns << '\n'
            << "[bool] use_push_rule "
            << param.use_push_rule << '\n'
            << "[string] max_ors "
            << param.max_ors << '\n'
            << "[string] softlimit_full "
            << brd.Cons(BLACK).SoftLimit(VC::FULL) << '\n'
            << "[string] softlimit_semi "
            << brd.Cons(BLACK).SoftLimit(VC::SEMI) << '\n';
    }
    else if (cmd.NuArg() == 2)
    {
        std::string name = cmd.Arg(0);
        if (name == "abort_on_winning_connection")
            param.abort_on_winning_connection = cmd.BoolArg(1);
        else if (name == "and_over_edge")
            param.and_over_edge = cmd.BoolArg(1);
        else if (name == "use_greedy_union")
            param.use_greedy_union = cmd.BoolArg(1);
        else if (name == "use_patterns")
            param.use_patterns = cmd.BoolArg(1);
        else if (name == "use_push_rule")
            param.use_push_rule = cmd.BoolArg(1);
        else if (name == "max_ors")
            param.max_ors = cmd.IntArg(1);
        else if (name == "softlimit_full")
        {
            int limit = cmd.IntArg(1, 0);
            brd.Cons(BLACK).SetSoftLimit(VC::FULL, limit);
            brd.Cons(WHITE).SetSoftLimit(VC::FULL, limit);
        }
        else if (name == "softlimit_semi")
        {
            int limit = cmd.IntArg(1, 0);
            brd.Cons(BLACK).SetSoftLimit(VC::SEMI, limit);
            brd.Cons(WHITE).SetSoftLimit(VC::SEMI, limit);
        }
        else
            throw HtpFailure() << "unknown parameter: " << name;
    }    
}

void ParamBoard(HexBoard& brd, HtpCommand& cmd)
{
    if (cmd.NuArg() == 0)
    {
        cmd << '\n'
            << "[bool] backup_ice_info "
            << brd.BackupIceInfo() << '\n'
            << "[bool] use_decompositions "
            << brd.UseDecompositions() << '\n'
            << "[bool] use_ice "
            << brd.UseICE() << '\n'
            << "[bool] use_vcs "
            << brd.UseVCs() << '\n';
    }
    else if (cmd.NuArg() == 2)
    {
        std::string name = cmd.Arg(0);
        if (name == "backup_ice_info")
            brd.SetBackupIceInfo(cmd.BoolArg(1));
        else if (name == "use_decompositions")
            brd.SetUseDecompositions(cmd.BoolArg(1));
        else if (name == "use_ice")
            brd.SetUseICE(cmd.BoolArg(1));
        else if (name == "use_vcs")
            brd.SetUseVCs(cmd.BoolArg(1));
        else 
            throw HtpFailure() << "unknown parameter: " << name;
    }
}

//----------------------------------------------------------------------------

} // namespace

//----------------------------------------------------------------------------

void HexEnvironment::NewGame(int width, int height)
{
    if (brd->width() != width && brd->height() != height)
    {
        /** @todo Make board resizable? Until then, make sure all
            HexBoard parameters are copied here! */
        bool use_vcs = brd->UseVCs();
        bool use_ice = brd->UseICE();
        bool use_dec = brd->UseDecompositions();
        bool backup  = brd->BackupIceInfo();
        brd.reset(new HexBoard(width, height, ice, buildParam));
        brd->SetUseVCs(use_vcs);
        brd->SetUseICE(use_ice);
        brd->SetUseDecompositions(use_dec);
        brd->SetBackupIceInfo(backup);
    }
    brd->startNewGame();
}

HexBoard& HexEnvironment::SyncBoard(const StoneBoard& board)
{
    brd->startNewGame();
    brd->setColor(BLACK, board.getBlack());
    brd->setColor(WHITE, board.getWhite());
    brd->setPlayed(board.getPlayed());
    return *brd.get();
}

//----------------------------------------------------------------------------

BenzeneHtpEngine::BenzeneHtpEngine(std::istream& in, std::ostream& out,
                             int boardsize, BenzenePlayer& player)
    : HexHtpEngine(in, out, boardsize),
      m_player(player),
      m_pe(m_board.width(), m_board.height()),
      m_se(m_board.width(), m_board.height()),
      m_solver(new Solver()),
      m_solver_tt(new SolverTT(20)), // TT with 10^6 entries
      m_book(0),
      m_db(0)
{
    RegisterCmd("reg_genmove", &BenzeneHtpEngine::CmdRegGenMove);

    RegisterCmd("get_absorb_group", &BenzeneHtpEngine::CmdGetAbsorbGroup);

    RegisterCmd("book-open", &BenzeneHtpEngine::CmdBookOpen);
    RegisterCmd("book-expand", &BenzeneHtpEngine::CmdBookExpand);
    RegisterCmd("book-depths", &BenzeneHtpEngine::CmdBookMainLineDepth);
    RegisterCmd("book-sizes", &BenzeneHtpEngine::CmdBookTreeSize);
    RegisterCmd("book-scores", &BenzeneHtpEngine::CmdBookScores);
    RegisterCmd("book-priorities", &BenzeneHtpEngine::CmdBookPriorities);

    RegisterCmd("compute-inferior", &BenzeneHtpEngine::CmdComputeInferior);
    RegisterCmd("compute-fillin", &BenzeneHtpEngine::CmdComputeFillin);
    RegisterCmd("compute-vulnerable", &BenzeneHtpEngine::CmdComputeVulnerable);
    RegisterCmd("compute-dominated", &BenzeneHtpEngine::CmdComputeDominated);
    RegisterCmd("find-comb-decomp", &BenzeneHtpEngine::CmdFindCombDecomp);
    RegisterCmd("find-split-decomp", &BenzeneHtpEngine::CmdFindSplitDecomp);
    RegisterCmd("encode-pattern", &BenzeneHtpEngine::CmdEncodePattern);

    RegisterCmd("param_player_ice", &BenzeneHtpEngine::CmdParamPlayerICE);
    RegisterCmd("param_player_vc", &BenzeneHtpEngine::CmdParamPlayerVC);
    RegisterCmd("param_player_board", &BenzeneHtpEngine::CmdParamPlayerBoard);
    RegisterCmd("param_player", &BenzeneHtpEngine::CmdParamPlayer);
    RegisterCmd("param_solver_ice", &BenzeneHtpEngine::CmdParamSolverICE);
    RegisterCmd("param_solver_vc", &BenzeneHtpEngine::CmdParamSolverVC);
    RegisterCmd("param_solver_board", &BenzeneHtpEngine::CmdParamSolverBoard);
    RegisterCmd("param_solver", &BenzeneHtpEngine::CmdParamSolver);

    RegisterCmd("vc-between-cells", &BenzeneHtpEngine::CmdGetVCsBetween);
    RegisterCmd("vc-connected-to", &BenzeneHtpEngine::CmdGetCellsConnectedTo);
    RegisterCmd("vc-get-mustplay", &BenzeneHtpEngine::CmdGetMustPlay);
    RegisterCmd("vc-intersection", &BenzeneHtpEngine::CmdVCIntersection);
    RegisterCmd("vc-union", &BenzeneHtpEngine::CmdVCUnion);
    RegisterCmd("vc-maintain", &BenzeneHtpEngine::CmdVCMaintain);

    RegisterCmd("vc-build", &BenzeneHtpEngine::CmdBuildStatic);
    RegisterCmd("vc-build-incremental", &BenzeneHtpEngine::CmdBuildIncremental);
    RegisterCmd("vc-undo-incremental", &BenzeneHtpEngine::CmdUndoIncremental);

    RegisterCmd("eval-twod", &BenzeneHtpEngine::CmdEvalTwoDist);
    RegisterCmd("eval-resist", &BenzeneHtpEngine::CmdEvalResist);
    RegisterCmd("eval-resist-delta", &BenzeneHtpEngine::CmdEvalResistDelta);
    RegisterCmd("eval-influence", &BenzeneHtpEngine::CmdEvalInfluence);

    RegisterCmd("solve-state", &BenzeneHtpEngine::CmdSolveState);
    RegisterCmd("solver-clear-tt", &BenzeneHtpEngine::CmdSolverClearTT);
    RegisterCmd("solver-find-winning", &BenzeneHtpEngine::CmdSolverFindWinning);

    RegisterCmd("db-open", &BenzeneHtpEngine::CmdDBOpen);
    RegisterCmd("db-close", &BenzeneHtpEngine::CmdDBClose);
    RegisterCmd("db-get", &BenzeneHtpEngine::CmdDBGet);

    RegisterCmd("misc-debug", &BenzeneHtpEngine::CmdMiscDebug);

    // FIXME: remove these lines once StoneBoard is no longer inherited from
    m_pe.brd->startNewGame();
    m_se.brd->startNewGame();

    // Set some defaults
    m_se.buildParam.max_ors = 3;
    m_se.buildParam.and_over_edge = false;
    m_solver->SetTT(m_solver_tt.get());
}

BenzeneHtpEngine::~BenzeneHtpEngine()
{
}

//----------------------------------------------------------------------------

void BenzeneHtpEngine::RegisterCmd(const std::string& name,
                               GtpCallback<BenzeneHtpEngine>::Method method)
{
    Register(name, new GtpCallback<BenzeneHtpEngine>(this, method));
}

VC::Type
BenzeneHtpEngine::VCTypeArg(const HtpCommand& cmd, std::size_t number) const
{
    return VCTypeUtil::fromString(cmd.ArgToLower(number));
}

void BenzeneHtpEngine::PrintVC(HtpCommand& cmd, const VC& vc, 
                            HexColor color) const
{
    cmd << color << " " << vc << '\n';
}

void BenzeneHtpEngine::NewGame(int width, int height)
{
    HexHtpEngine::NewGame(width, height);

    m_pe.NewGame(width, height);
    m_se.NewGame(width, height);
}

/** Generates a move. */
HexPoint BenzeneHtpEngine::GenMove(HexColor color, double time_remaining)
{
    double score;
    return m_player.genmove(m_pe.SyncBoard(m_game->Board()), *m_game, 
                            color, time_remaining, score);
}

////////////////////////////////////////////////////////////////////////
// Commands
////////////////////////////////////////////////////////////////////////

/** Generates a move, but does not play it. */
void BenzeneHtpEngine::CmdRegGenMove(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    double score;
    HexPoint move = m_player.genmove(m_pe.SyncBoard(m_game->Board()),
                                     *m_game, ColorArg(cmd, 0),
                                     -1, score);
    cmd << HexPointUtil::toString(move);
}

/** Returns the set of stones this stone is part of. */
void BenzeneHtpEngine::CmdGetAbsorbGroup(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexPoint cell = MoveArg(cmd, 0);
    GroupBoard brd(m_game->Board().width(), m_game->Board().height());
    brd.setColor(BLACK, m_game->Board().getBlack());
    brd.setColor(WHITE, m_game->Board().getWhite());
    brd.absorb();

    if (brd.getColor(cell) == EMPTY)
        return;

    HexPoint captain = brd.getCaptain(cell);
    cmd << HexPointUtil::toString(captain);

    int c = 1;
    for (BoardIterator p(brd.EdgesAndInterior()); p; ++p) {
        if (*p == captain) continue;
        if (brd.getCaptain(*p) == captain) {
            cmd << " " << HexPointUtil::toString(*p);
            if ((++c % 10) ==  0) cmd << "\n";
        }
    }
}

//---------------------------------------------------------------------------

void BenzeneHtpEngine::ParamPlayer(BenzenePlayer* player, HtpCommand& cmd)
{
    BookCheck* book = GetInstanceOf<BookCheck>(player);
    EndgameCheck* endgame = GetInstanceOf<EndgameCheck>(player);
    HandBookCheck* handbook = GetInstanceOf<HandBookCheck>(player);
    LadderCheck* ladder = GetInstanceOf<LadderCheck>(player);
    SolverCheck* solver = GetInstanceOf<SolverCheck>(player);
    
    if (cmd.NuArg() == 0)
    {
        cmd << '\n';
        if (book) 
            cmd << "[bool] use_book "
                << book->Enabled() << '\n';
        if (endgame) 
            cmd << "[bool] use_endgame_check "
                << endgame->Enabled() << '\n';
        if (handbook)
            cmd << "[bool] use_handbook "
                << handbook->Enabled() << '\n';
        if (ladder)
            cmd << "[bool] use_ladder_check "
                << ladder->Enabled() << '\n';
        if (solver)
            cmd << "[bool] use_solver_check "
                << solver->Enabled() << '\n';
        if (book) 
        {
            cmd << "[string] book_depth_value_adjustment "
                << book->DepthValueAdjustment() << '\n'
                << "[string] book_max_depth "
                << book->MaxDepth() << '\n'
                << "[string] book_min_depth "
                << book->MinDepth() << '\n';
        }
    }
    else if (cmd.NuArg() == 2)
    {
        std::string name = cmd.Arg(0);

        if (book && name == "book_min_depth")
            book->SetMinDepth(cmd.IntArg(1, 0));
        else if (book && name == "book_max_depth")
            book->SetMaxDepth(cmd.IntArg(1, 0));
        else if (book && name == "book_depth_value_adjustment")
            book->SetDepthValueAdjustment(cmd.FloatArg(1));
	else if (book && name == "use_book")
	    book->SetEnabled(cmd.BoolArg(1));
        else if (endgame && name == "use_endgame_check") 
                endgame->SetEnabled(cmd.BoolArg(1));
        else if (handbook && name == "use_handbook")
            handbook->SetEnabled(cmd.BoolArg(1));
        else if (ladder && name == "use_ladder_check") 
            ladder->SetEnabled(cmd.BoolArg(1));
        else if (solver && name == "use_solver_check") 
            solver->SetEnabled(cmd.BoolArg(1));
    }
    else 
        throw HtpFailure("Expected 0 ore 2 arguments");
}

void BenzeneHtpEngine::CmdParamPlayerICE(HtpCommand& cmd)
{
    ParamICE(m_pe.ice, cmd);
}

void BenzeneHtpEngine::CmdParamPlayerVC(HtpCommand& cmd)
{
    ParamVC(*m_pe.brd, cmd);
}

void BenzeneHtpEngine::CmdParamPlayerBoard(HtpCommand& cmd)
{
    ParamBoard(*m_pe.brd, cmd);
}

void BenzeneHtpEngine::CmdParamPlayer(HtpCommand& cmd)
{
    ParamPlayer(&m_player, cmd);
}

void BenzeneHtpEngine::CmdParamSolverICE(HtpCommand& cmd)
{
    ParamICE(m_se.ice, cmd);    
}

void BenzeneHtpEngine::CmdParamSolverVC(HtpCommand& cmd)
{
    ParamVC(*m_se.brd, cmd);
}

void BenzeneHtpEngine::CmdParamSolverBoard(HtpCommand& cmd)
{
    ParamBoard(*m_se.brd, cmd);
}

void BenzeneHtpEngine::CmdParamSolver(HtpCommand& cmd)
{
    if (cmd.NuArg() == 0)
    {
        cmd << '\n'
            << "[bool] backup_ice_info "
            << m_solver->BackupIceInfo() << '\n'
            << "[bool] shrink_proofs "
            << m_solver->ShrinkProofs() << '\n'
            << "[bool] use_decompositions "
            << m_solver->UseDecompositions() << '\n'
            << "[bool] use_guifx " 
            << m_solver->UseGuiFx() << '\n'
            << "[string] move_ordering "
            << m_solver->MoveOrdering() << '\n' // FIXME: PRINT NICELY!!
            << "[string] progress_depth "
            << m_solver->ProgressDepth() << '\n'
            << "[string] update_depth "  
            << m_solver->UpdateDepth() << '\n';
    }
    else if (cmd.NuArg() == 2)
    {
        std::string name = cmd.Arg(0);
        if (name == "backup_ice_info")
            m_solver->SetBackupIceInfo(cmd.BoolArg(1));
        else if (name == "shrink_proofs")
            m_solver->SetShrinkProofs(cmd.BoolArg(1));
        else if (name == "use_decompositions")
            m_solver->SetUseDecompositions(cmd.BoolArg(1));
        else if (name == "use_guifx")
            m_solver->SetUseGuiFx(cmd.BoolArg(1));
        else if (name == "move_ordering")
            m_solver->SetMoveOrdering(cmd.IntArg(1,0,7));
        else if (name == "progress_depth")
            m_solver->SetProgressDepth(cmd.IntArg(1, 0));
        else if (name == "update_depth")
            m_solver->SetUpdateDepth(cmd.IntArg(1, 0));
        else
            throw HtpFailure() << "unknown parameter: " << name;
    }
}

//----------------------------------------------------------------------

/** Opens/Creates an opening book for the current boardsize.
    Usage: "book-expand [filename] {alpha}"
*/
void BenzeneHtpEngine::CmdBookOpen(HtpCommand& cmd)
{
    cmd.CheckNuArgLessEqual(2);
    std::string filename = cmd.Arg(0);
    double alpha = 15.0;
    if (cmd.NuArg() == 2) {
        alpha = cmd.FloatArg(1);
    }

    if (m_book) delete m_book;

    const StoneBoard& brd = m_game->Board();
    m_book = new OpeningBook(brd.width(), brd.height(), alpha, filename);
}

bool BenzeneHtpEngine::StateMatchesBook(const StoneBoard& board)
{
    if (!m_book) {
        throw HtpFailure() << "No open book.";
        return false;
    } else {
        OpeningBook::Settings settings = m_book->GetSettings();
        if (settings.board_width != board.width() ||
            settings.board_height != board.height()) {
            throw HtpFailure() << "Book is for different boardsize!";
            return false;
        }
    }
    return true;
}

/** Expands the current node in the current opening book.
    "book-expand [iterations]"
*/
void BenzeneHtpEngine::CmdBookExpand(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    int iterations = cmd.IntArg(0, 1);
    HexBoard& brd = m_pe.SyncBoard(m_game->Board());

    if (!StateMatchesBook(brd))
        return;

    m_book->Expand(brd, iterations);
}

void BenzeneHtpEngine::CmdBookMainLineDepth(HtpCommand& cmd)
{
    HexBoard& brd = m_pe.SyncBoard(m_game->Board());
    HexColor color = brd.WhoseTurn();

    if (!StateMatchesBook(brd))
        return;

    brd.ComputeAll(color, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);
    bitset_t consider = PlayerUtils::MovesToConsider(brd, color);
    for (BitsetIterator p(consider); p; ++p) {
        brd.playMove(color, *p);
        cmd << " " << HexPointUtil::toString(*p)
            << " " << m_book->GetMainLineDepth(brd, !color);
        brd.undoMove(*p);
    }
}

void BenzeneHtpEngine::CmdBookTreeSize(HtpCommand& cmd)
{
    HexBoard& brd = m_pe.SyncBoard(m_game->Board());
    HexColor color = brd.WhoseTurn();

    if (!StateMatchesBook(brd))
        return;

    brd.ComputeAll(color, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);
    bitset_t consider = PlayerUtils::MovesToConsider(brd, color);
    for (BitsetIterator p(consider); p; ++p) {
        brd.playMove(color, *p);
        cmd << " " << HexPointUtil::toString(*p)
            << " " << m_book->GetTreeSize(brd, !color);
        brd.undoMove(*p);
    }
}

void BenzeneHtpEngine::CmdBookScores(HtpCommand& cmd)
{
    HexBoard& brd = m_pe.SyncBoard(m_game->Board());
    HexColor color = brd.WhoseTurn();

    if (!StateMatchesBook(brd))
        return;

    brd.ComputeAll(color, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);
    bitset_t consider = PlayerUtils::MovesToConsider(brd, color);
    for (BitsetIterator p(consider); p; ++p) {
        brd.playMove(color, *p);
        OpeningBookNode node = m_book->GetNode(brd.Hash());

        if (!node.IsDummy()) {
            cmd << " " << HexPointUtil::toString(*p);
            // negated because we're looking at it from the parent state
            HexEval value = -node.getPropValue();
            if (HexEvalUtil::IsWin(value))
                cmd << " W";
            else if (HexEvalUtil::IsLoss(value))
                cmd << " L";
            else {
                cmd << " " << std::fixed << std::setprecision(3) << value;
            }
        }

        brd.undoMove(*p);
    }
}

void BenzeneHtpEngine::CmdBookPriorities(HtpCommand& cmd)
{
    HexBoard& brd = m_pe.SyncBoard(m_game->Board());
    HexColor color = brd.WhoseTurn();

    if (!StateMatchesBook(brd))
        return;

    brd.ComputeAll(color, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);
    float alpha = (m_book->GetSettings()).alpha;
    OpeningBookNode parent = m_book->GetNode(brd.Hash());
    HexAssert(!parent.IsDummy());

    bitset_t consider = PlayerUtils::MovesToConsider(brd, color);
    for (BitsetIterator p(consider); p; ++p) {
        brd.playMove(color, *p);
        OpeningBookNode succ = m_book->GetNode(brd.Hash());

        if (!succ.IsDummy()) {
            cmd << " " << HexPointUtil::toString(*p);
            // negated because we're looking at it from the parent state
            float value = parent.ComputeExpPriority(succ.getPropValue(), alpha,
						    succ.getExpPriority());
	    cmd << " " << std::fixed << std::setprecision(1) << value;
        }

        brd.undoMove(*p);
    }
}

//----------------------------------------------------------------------

/** Does inferior cell analysis. First argument is the color of the
    player. */
void BenzeneHtpEngine::CmdComputeInferior(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor color = ColorArg(cmd, 0);

    HexBoard& brd = m_pe.SyncBoard(m_game->Board());
    brd.update();
    brd.absorb();

    InferiorCells inf;
    m_pe.ice.ComputeInferiorCells(color, brd, inf);

    cmd << inf.GuiOutput();
    cmd << "\n";
}

/** Computes fillin for the given board. Color argument affects order
    for computing vulnerable/presimplicial pairs. */
void BenzeneHtpEngine::CmdComputeFillin(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor color = ColorArg(cmd, 0);

    HexBoard& brd = m_pe.SyncBoard(m_game->Board());
    brd.update();
    brd.absorb();

    InferiorCells inf;
    m_pe.ice.ComputeFillin(color, brd, inf);
    inf.ClearVulnerable();

    cmd << inf.GuiOutput();
    cmd << "\n";
}

/** Computes vulnerable cells on the current board for the given color. */
void BenzeneHtpEngine::CmdComputeVulnerable(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor col = ColorArg(cmd, 0);

    HexBoard& brd = m_pe.SyncBoard(m_game->Board());
    brd.update();
    brd.absorb();

    InferiorCells inf;
    m_pe.ice.FindVulnerable(brd, col, brd.getEmpty(), inf);

    cmd << inf.GuiOutput();
    cmd << "\n";
}

/** Computes dominated cells on the current board for the given color. */
void BenzeneHtpEngine::CmdComputeDominated(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor col = ColorArg(cmd, 0);

    HexBoard& brd = m_pe.SyncBoard(m_game->Board());
    brd.update();
    brd.absorb();

    InferiorCells inf;
    m_pe.ice.FindDominated(brd, col, brd.getEmpty(), inf);

    cmd << inf.GuiOutput();
    cmd << "\n";
}

// tries to find a combinatorial decomposition of the board state
void BenzeneHtpEngine::CmdFindCombDecomp(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor color = ColorArg(cmd, 0);

    HexBoard& brd = m_pe.SyncBoard(m_game->Board());
    brd.ComputeAll(BLACK, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);

    bitset_t capturedVC;
    if (BoardUtils::FindCombinatorialDecomposition(brd, color, capturedVC)) {
        hex::log << hex::info << "Found decomposition!" << hex::endl;
        PrintBitsetToHTP(cmd, capturedVC);
    }
}

void BenzeneHtpEngine::CmdFindSplitDecomp(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor color = ColorArg(cmd, 0);

    HexBoard& brd = m_pe.SyncBoard(m_game->Board());
    brd.ComputeAll(BLACK, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);
    HexPoint group;
    bitset_t capturedVC;
    if (BoardUtils::FindSplittingDecomposition(brd, color, group,
					       capturedVC)) {
        hex::log << hex::info << "Found split decomp: "
                 << HexPointUtil::toString(group) << "!"
                 << hex::endl;
        PrintBitsetToHTP(cmd, capturedVC);
    }
}

void BenzeneHtpEngine::CmdEncodePattern(HtpCommand& cmd)
{
    HexAssert(cmd.NuArg() > 0);

    //Build direction offset look-up matrix.
    int xoffset[Pattern::NUM_SLICES][32];
    int yoffset[Pattern::NUM_SLICES][32];
    for (int s=0; s<Pattern::NUM_SLICES; s++)
    {
        int fwd = s;
        int lft = (s + 2) % NUM_DIRECTIONS;
        int x1 = HexPointUtil::DeltaX(fwd);
        int y1 = HexPointUtil::DeltaY(fwd);
        for (int i=1, g=0; i<=Pattern::MAX_EXTENSION; i++)
        {
            int x2 = x1;
            int y2 = y1;
            for (int j=0; j<i; j++)
            {
                xoffset[s][g] = x2;
                yoffset[s][g] = y2;
                x2 += HexPointUtil::DeltaX(lft);
                y2 += HexPointUtil::DeltaY(lft);
                g++;
            }
            x1 += HexPointUtil::DeltaX(fwd);
            y1 += HexPointUtil::DeltaY(fwd);
        }
    }

    int pattOut[Pattern::NUM_SLICES * 5];
    memset(pattOut, 0, sizeof(pattOut));
    StoneBoard brd(m_game->Board());
    HexPoint center = MoveArg(cmd, 0);
    hex::log << hex::info << "Center of pattern: " << center
             << hex::endl << "Includes: ";
    int x1, y1, x2, y2;
    HexPointUtil::pointToCoords(center, x1, y1);
    uint i = 1;
    while (i < cmd.NuArg())
    {
        HexPoint p = MoveArg(cmd, i++);
        HexPointUtil::pointToCoords(p, x2, y2);
        x2 = x2 - x1;
        y2 = y2 - y1;
        int sliceNo;
        if (y2 > 0)
        {
            if ((x2 + y2) < 0)          // Point is in bottom of 4th slice
                sliceNo = 3;
            else if ((x2 < 0))          // Point is in 5th slice
                sliceNo = 4;
            else                        // point is in 6th slice
                sliceNo = 5;
        }
        else
        {
            if ((x2 + y2) > 0)          // Point is in 1st slice
                sliceNo = 0;
            else if (x2 > 0)            // Point is in 2nd slice
                sliceNo = 1;
            else if (x2 < 0 && y2 == 0) // Point is in upper part of 4th slice
                sliceNo = 3;
            else                        // Point is in 3rd slice
                sliceNo = 2;
        }
        int j = 0;
        while (j < 32 && (xoffset[sliceNo][j] != x2 ||
                          yoffset[sliceNo][j] != y2))
            j++;
        HexAssert(j != 32);
        pattOut[sliceNo*5] += (1 << j);

        if (brd.isBlack(p))
            pattOut[(sliceNo*5) + 1] += (1 << j);
        else if (brd.isWhite(p))
            pattOut[(sliceNo*5) + 2] += (1 << j);
        hex::log << hex::info << p << ":" << brd.getColor(p) << ", ";
    }
    hex::log << hex::info << hex::endl;
    
    std::string encPattStr = "d:";

    for (int k = 0; k < Pattern::NUM_SLICES; k++)
    {
        for (int l = 0; l < 4; l++)
        {
            std::stringstream out; //FIXME: Isn't there a better way??
           out << (pattOut[(k*5) + l]) << ",";
           encPattStr.append(out.str());
        }
           std::stringstream out;
           out << (pattOut[(k*5) + 4]) << ";";
           encPattStr.append(out.str());
    }
    hex::log << hex::info << encPattStr << hex::endl;
}

//----------------------------------------------------------------------------
// VC commands
//----------------------------------------------------------------------------

void BenzeneHtpEngine::CmdBuildStatic(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor color = ColorArg(cmd, 0);

    HexBoard& brd = m_pe.SyncBoard(m_game->Board());
    brd.ComputeAll(color, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);
    cmd << brd.getInferiorCells().GuiOutput();
    if (!PlayerUtils::IsDeterminedState(brd, color))
    {
        bitset_t consider = PlayerUtils::MovesToConsider(brd, color);
        cmd << BoardUtils::GuiDumpOutsideConsiderSet(brd, consider,
                                              brd.getInferiorCells().All());
    }
    cmd << "\n";
}

void BenzeneHtpEngine::CmdBuildIncremental(HtpCommand& cmd)
{
    cmd.CheckNuArgLessEqual(2);
    HexColor color = ColorArg(cmd, 0);
    HexPoint point = MoveArg(cmd, 1);

    HexBoard& brd = *m_pe.brd; // <-- NOTE: no call to SyncBoard()!
    brd.PlayMove(color, point);
    cmd << brd.getInferiorCells().GuiOutput();
    if (!PlayerUtils::IsDeterminedState(brd, color))
    {
        bitset_t consider = PlayerUtils::MovesToConsider(brd, color);
        cmd << BoardUtils::GuiDumpOutsideConsiderSet(brd, consider,
                                           brd.getInferiorCells().All());
    }

    cmd << "\n";
}

void BenzeneHtpEngine::CmdUndoIncremental(HtpCommand& UNUSED(cmd))
{
    m_pe.brd->UndoMove();
}

/** Returns a list of VCs between the given two cells.
    Format: "vc-between-cells x y c t", where x and y are the cells,
    c is the color of the player, and t is the type of connection
    (0-conn, 1-conn, etc). */
void BenzeneHtpEngine::CmdGetVCsBetween(HtpCommand& cmd)
{
    cmd.CheckNuArg(4);
    HexPoint from = MoveArg(cmd, 0);
    HexPoint to = MoveArg(cmd, 1);
    HexColor color = ColorArg(cmd, 2);
    VC::Type ctype = VCTypeArg(cmd, 3);

    HexBoard& brd = *m_pe.brd;
    HexPoint fcaptain = brd.getCaptain(from);
    HexPoint tcaptain = brd.getCaptain(to);

    std::vector<VC> vc;
    brd.Cons(color).VCs(fcaptain, tcaptain, ctype, vc);
    const VCList& lst = brd.Cons(color).GetList(ctype, fcaptain, tcaptain);

    cmd << "\n";

    unsigned i=0;
    for (; i<(unsigned)lst.softlimit() && i<vc.size(); i++) 
        PrintVC(cmd, vc.at(i), color);

    if (i >= vc.size())
        return;

    cmd << color << " ";
    cmd << HexPointUtil::toString(fcaptain) << " ";
    cmd << HexPointUtil::toString(tcaptain) << " ";
    cmd << "softlimit ----------------------";
    cmd << "\n";

    for (; i<vc.size(); i++)
        PrintVC(cmd, vc.at(i), color);
}


/** Returns a list of cells the given cell shares a vc with.
    Format: "vc-connected-to x c t", where x is the cell in question,
    c is the color of the player, and t is the type of vc. */
void BenzeneHtpEngine::CmdGetCellsConnectedTo(HtpCommand& cmd)
{
    cmd.CheckNuArg(3);
    HexPoint from = MoveArg(cmd, 0);
    HexColor color = ColorArg(cmd, 1);
    VC::Type ctype = VCTypeArg(cmd, 2);
    bitset_t pt = ConUtil::ConnectedTo(m_pe.brd->Cons(color), 
                                       *m_pe.brd, from, ctype);
    PrintBitsetToHTP(cmd, pt);
}

void BenzeneHtpEngine::CmdGetMustPlay(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor color = ColorArg(cmd, 0);
    bitset_t mustplay = m_pe.brd->getMustplay(color);
    InferiorCells inf(m_pe.brd->getInferiorCells());
    inf.ClearVulnerable();
    inf.ClearDominated();
    cmd << inf.GuiOutput();
    if (!PlayerUtils::IsDeterminedState(*m_pe.brd, color))
    {
        bitset_t consider = PlayerUtils::MovesToConsider(*m_pe.brd, color);
        cmd << BoardUtils::GuiDumpOutsideConsiderSet(*m_pe.brd, consider,
                                                     inf.All());
    }
}

void BenzeneHtpEngine::CmdVCIntersection(HtpCommand& cmd)
{
    cmd.CheckNuArg(4);
    HexPoint from = MoveArg(cmd, 0);
    HexPoint to = MoveArg(cmd, 1);
    HexColor color = ColorArg(cmd, 2);
    VC::Type ctype = VCTypeArg(cmd, 3);

    HexBoard& brd = *m_pe.brd;
    HexPoint fcaptain = brd.getCaptain(from);
    HexPoint tcaptain = brd.getCaptain(to);
    const VCList& lst = brd.Cons(color).GetList(ctype, fcaptain, tcaptain);
    bitset_t intersection = lst.hardIntersection();

    PrintBitsetToHTP(cmd, intersection);
}

void BenzeneHtpEngine::CmdVCUnion(HtpCommand& cmd)
{
    cmd.CheckNuArg(4);
    HexPoint from = MoveArg(cmd, 0);
    HexPoint to = MoveArg(cmd, 1);
    HexColor color = ColorArg(cmd, 2);
    VC::Type ctype = VCTypeArg(cmd, 3);

    HexBoard& brd = *m_pe.brd;
    HexPoint fcaptain = brd.getCaptain(from);
    HexPoint tcaptain = brd.getCaptain(to);
    const VCList& lst = brd.Cons(color).GetList(ctype, fcaptain, tcaptain);
    bitset_t un = lst.getGreedyUnion(); // FIXME: shouldn't be greedy!!

    PrintBitsetToHTP(cmd, un);
}

/** Returns a list of VCs that can be maintained by mohex for the
    player of the appropriate color. */
void BenzeneHtpEngine::CmdVCMaintain(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor color = ColorArg(cmd, 0);

    HexBoard& brd = *m_pe.brd;
    std::vector<VC> maintain;
    VCUtils::findMaintainableVCs(brd, color, maintain);

    cmd << "\n";
    std::vector<VC>::const_iterator it;
    for (it = maintain.begin(); it != maintain.end(); ++it) 
        PrintVC(cmd, *it, color);
}

//----------------------------------------------------------------------------
// Evaluation commands
//----------------------------------------------------------------------------

void BenzeneHtpEngine::CmdEvalTwoDist(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor color = ColorArg(cmd, 0);

    HexBoard& brd = m_pe.SyncBoard(m_game->Board());
    brd.ComputeAll(color, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);
    TwoDistance twod(TwoDistance::ADJACENT);
    twod.Evaluate(brd);

    for (BoardIterator it(brd.Interior()); it; ++it) {
        if (brd.isOccupied(*it)) continue;
        HexEval energy = twod.Score(*it, color);
        if (energy == EVAL_INFINITY)
            energy = -1;

        cmd << " " << HexPointUtil::toString(*it)
            << " " << energy;
    }
}

void BenzeneHtpEngine::CmdEvalResist(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor color = ColorArg(cmd, 0);

    HexBoard& brd = m_pe.SyncBoard(m_game->Board());
    brd.ComputeAll(color, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);
    Resistance resist;
    resist.Evaluate(brd);

    cmd << " res " << std::fixed << std::setprecision(3) << resist.Score()
        << " rew " << std::fixed << std::setprecision(3) << resist.Resist(WHITE)
        << " reb " << std::fixed << std::setprecision(3) << resist.Resist(BLACK);

    for (BoardIterator it(brd.Interior()); it; ++it) {
        if (brd.isOccupied(*it)) continue;
        HexEval energy = resist.Score(*it, color);
        if (energy == EVAL_INFINITY)
            energy = -1;

        cmd << " " << HexPointUtil::toString(*it)
            << " " << std::fixed << std::setprecision(3) << energy;
    }
}

void BenzeneHtpEngine::CmdEvalResistDelta(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor color = ColorArg(cmd, 0);

    HexBoard& brd = m_pe.SyncBoard(m_game->Board());
    brd.ComputeAll(color, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);
    Resistance resist;
    resist.Evaluate(brd);
    HexEval base = resist.Score();

    cmd << " res " << std::fixed << std::setprecision(3) << base;
    for (BitsetIterator it(brd.getEmpty()); it; ++it) {
        brd.PlayMove(color, *it);

        resist.Evaluate(brd);
        HexEval cur = resist.Score();

        cmd << " " << HexPointUtil::toString(*it)
            << " " << std::fixed << std::setprecision(3) << (cur - base);

        brd.UndoMove();
    }
}

void BenzeneHtpEngine::CmdEvalInfluence(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor color = ColorArg(cmd, 0);

    HexBoard& brd = m_pe.SyncBoard(m_game->Board());
    brd.ComputeAll(color, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);

    // Pre-compute edge adjacencies
    bitset_t northNbs 
        = ConUtil::ConnectedTo(brd.Cons(BLACK), brd, NORTH, VC::FULL);
    bitset_t southNbs 
        = ConUtil::ConnectedTo(brd.Cons(BLACK), brd, SOUTH, VC::FULL);
    bitset_t eastNbs 
        = ConUtil::ConnectedTo(brd.Cons(WHITE), brd, EAST, VC::FULL);
    bitset_t westNbs 
        = ConUtil::ConnectedTo(brd.Cons(WHITE), brd, WEST, VC::FULL);

    for (BoardIterator it(brd.Interior()); it; ++it) {
        if (brd.isOccupied(*it)) continue;

	// Compute neighbours, giving over-estimation to edges
	bitset_t b1 = ConUtil::ConnectedTo(brd.Cons(BLACK), brd, 
                                           *it, VC::FULL);
	if (b1.test(NORTH)) b1 |= northNbs;
	if (b1.test(SOUTH)) b1 |= southNbs;
	b1 &= brd.getEmpty();
	bitset_t b2 = ConUtil::ConnectedTo(brd.Cons(WHITE), brd, 
                                           *it, VC::FULL);
	if (b2.test(EAST)) b2 |= eastNbs;
	if (b2.test(WEST)) b2 |= westNbs;
	b2 &= brd.getEmpty();

	// Compute ratio of VCs at this cell, and use as measure of influence
	double v1 = (double) b1.count();
	double v2 = (double) b2.count();
	HexAssert(v1+v2 >= 1.0);
	double influence;
	if (color == BLACK)
	    influence = v1 / (v1 + v2);
	else
	    influence = v2 / (v1 + v2);

        cmd << " " << HexPointUtil::toString(*it) << " "
	    << std::fixed << std::setprecision(2) << influence;
    }
}

//----------------------------------------------------------------------------
// Solver commands
//----------------------------------------------------------------------------

void BenzeneHtpEngine::CmdSolveState(HtpCommand& cmd)
{
    cmd.CheckNuArgLessEqual(4);
    HexColor color = ColorArg(cmd, 0);

    bool use_db = false;
    std::string filename = "dummy";
    int maxstones = 5;
    int transtones = maxstones;
    if (cmd.NuArg() >= 2) {
        use_db = true;
        filename = cmd.Arg(1);
    }
    if (cmd.NuArg() == 3) {
        maxstones = cmd.IntArg(2, 1);
        transtones = maxstones;
    } else if (cmd.NuArg() == 4) {
        transtones = cmd.IntArg(2, -1);
        maxstones = cmd.IntArg(3, 1);
    }

    double timelimit = -1.0;  // FIXME: WHY ARE THESE HERE?!?!
    int depthlimit = -1;

    HexBoard& brd = m_se.SyncBoard(m_game->Board());

    Solver::SolutionSet solution;
    Solver::Result result = (use_db) ?
        m_solver->Solve(brd, color, filename, maxstones, transtones, solution,
                      depthlimit, timelimit) :
        m_solver->Solve(brd, color, solution, depthlimit, timelimit);

    m_solver->DumpStats(solution);

    HexColor winner = EMPTY;
    if (result != Solver::UNKNOWN) {
        winner = (result==Solver::WIN) ? color : !color;
        hex::log << hex::info;
        hex::log << winner << " wins!" << hex::endl;
        hex::log << brd.printBitset(solution.proof) << hex::endl;
    } else {
        hex::log << hex::info;
        hex::log << "Search aborted!" << hex::endl;
    }
    cmd << winner;
}

void BenzeneHtpEngine::CmdSolverClearTT(HtpCommand& UNUSED(cmd))
{
    m_solver_tt->clear();
}

void BenzeneHtpEngine::CmdSolverFindWinning(HtpCommand& cmd)
{
    cmd.CheckNuArgLessEqual(4);
    HexColor color = ColorArg(cmd, 0);
    HexColor other = !color;

    bool use_db = false;
    std::string filename = "dummy";
    int maxstones = 5; // FIXME: Good?
    int transtones = maxstones;
    if (cmd.NuArg() >= 2)
    {
        use_db = true;
        filename = cmd.Arg(1);
    }

    if (cmd.NuArg() == 3)
    {
        maxstones = cmd.IntArg(2, 1);
        transtones = maxstones;
    }
    else if (cmd.NuArg() == 4)
    {
        transtones = cmd.IntArg(2, -1);
        maxstones = cmd.IntArg(3, 1);
    }

    HexBoard& brd = m_se.SyncBoard(m_game->Board());
    brd.ComputeAll(color, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);
    bitset_t consider = PlayerUtils::MovesToConsider(brd, color);
    bitset_t winning;

    for (BitsetIterator p(consider); p; ++p)
    {
	if (!consider.test(*p)) continue;

        StoneBoard board(m_game->Board());
        board.playMove(color, *p);

        HexBoard& brd = m_se.SyncBoard(board);

        bitset_t proof;
        std::vector<HexPoint> pv;

        hex::log << hex::info
                 << "****** Trying " << HexPointUtil::toString(*p)
                 << " ******" << hex::endl
                 << brd << hex::endl;

        HexColor winner = EMPTY;
        Solver::SolutionSet solution;
        Solver::Result result = (use_db) 
            ? m_solver->Solve(brd, other, filename, 
                              maxstones, transtones, solution)
            : m_solver->Solve(brd, other, solution);
        m_solver->DumpStats(solution);
        hex::log << hex::info
                 << "Proof:" << brd.printBitset(solution.proof)
                 << hex::endl;

        if (result != Solver::UNKNOWN) {
            winner = (result==Solver::WIN) ? !color : color;
            hex::log << hex::info
                     << "****** " << winner << " wins ******"
                     << hex::endl;
        } else {
            hex::log << hex::info << "****** unknown ******"  << hex::endl;
        }


        if (winner == color) {
            winning.set(*p);
        } else {
	    consider &= solution.proof;
	}
    }

    hex::log << hex::info
             << "****** Winning Moves ******" << hex::endl
             << m_game->Board().printBitset(winning) << hex::endl;

    cmd << HexPointUtil::ToPointListString(winning);
}

//----------------------------------------------------------------------------

void BenzeneHtpEngine::CmdDBOpen(HtpCommand& cmd)
{
    cmd.CheckNuArgLessEqual(3);
    std::string filename = cmd.Arg(0);
    int maxstones = -1;
    int transtones = -1;

    if (cmd.NuArg() == 2) {
        maxstones = cmd.IntArg(1, 1);
        transtones = maxstones;
    } else if (cmd.NuArg() == 3) {
        transtones = cmd.IntArg(1, -1);
        maxstones = cmd.IntArg(2, 1);
    }

    if (m_db) delete m_db;

    const StoneBoard& brd = m_game->Board();

    bool success = true;
    m_db = new SolverDB();
    success = (maxstones == -1)
        ? m_db->open(brd.width(), brd.height(), filename)
        : m_db->open(brd.width(), brd.height(),  maxstones,
                     transtones, filename);

    if (!success) {
        delete m_db;
        m_db = 0;
        throw HtpFailure() << "Could not open database!";
    }
}

void BenzeneHtpEngine::CmdDBClose(HtpCommand& cmd)
{
    cmd.CheckNuArg(0);

    if (m_db) delete m_db;
    m_db = 0;

}

void BenzeneHtpEngine::CmdDBGet(HtpCommand& cmd)
{
    cmd.CheckNuArg(0);

    StoneBoard brd(m_game->Board());
    HexColor toplay = brd.WhoseTurn();
    SolvedState state;

    if (!m_db) {
        throw HtpFailure() << "No open database.";
        return;
    }

    if (!m_db->get(brd, state)) {
        cmd << "State not in database.";
        return;
    }

    // dump winner and proof
    cmd << (state.win ? toplay : !toplay);
    cmd << " " << state.nummoves;
    PrintBitsetToHTP(cmd, state.proof);

    // find winning/losing moves
    std::vector<int> nummoves(BITSETSIZE);
    std::vector<int> flags(BITSETSIZE);
    std::vector<HexPoint> winning, losing;
    for (BitsetIterator p(brd.getEmpty()); p; ++p) {
        brd.playMove(toplay, *p);

        if (m_db->get(brd, state)) {
            if (state.win)
                losing.push_back(*p);
            else
                winning.push_back(*p);

            nummoves[*p] = state.nummoves;
            flags[*p] = state.flags;
        }

        brd.undoMove(*p);
    }

    // dump winning moves
    cmd << " Winning";
    for (unsigned i=0; i<winning.size(); ++i) {
        cmd << " " << HexPointUtil::toString(winning[i]);
        cmd << " " << nummoves[winning[i]];
        if (flags[winning[i]] & SolvedState::FLAG_MIRROR_TRANSPOSITION)
            cmd << "m";
        else if (flags[winning[i]] & SolvedState::FLAG_TRANSPOSITION)
            cmd << "t";
    }

    // dump losing moves
    cmd << " Losing";
    for (unsigned i=0; i<losing.size(); ++i) {
        cmd << " " << HexPointUtil::toString(losing[i]);
        cmd << " " << nummoves[losing[i]];
        if (flags[losing[i]] & SolvedState::FLAG_MIRROR_TRANSPOSITION)
            cmd << "m";
        else if (flags[losing[i]] & SolvedState::FLAG_TRANSPOSITION)
            cmd << "t";
    }
}

//----------------------------------------------------------------------------

void BenzeneHtpEngine::CmdMiscDebug(HtpCommand& cmd)
{
//     cmd.CheckNuArg(1);
//     HexPoint point = MoveArg(cmd, 0);
    cmd << *m_pe.brd << hex::endl;
}

//----------------------------------------------------------------------------
