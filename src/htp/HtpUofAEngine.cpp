//----------------------------------------------------------------------------
// $Id: HtpUofAEngine.cpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#include "SgSystem.h"

#include "GraphUtils.hpp"
#include "OpeningBook.hpp"
#include "Resistance.hpp"
#include "SolverDB.hpp"
#include "TwoDistance.hpp"
#include "BoardUtils.hpp"
#include "BitsetIterator.hpp"
#include "PlayerUtils.hpp"
#include "VCUtils.hpp"

#include "HtpUofAEngine.hpp"

//----------------------------------------------------------------------------

namespace
{

} // namespace

//----------------------------------------------------------------------------

HtpUofAEngine::HtpUofAEngine(std::istream& in,
                             std::ostream& out,
                             UofAProgram &program)
    : HtpHexEngine(in, out, program),
      m_uofa_program(program),
      m_brd(0),
      m_book(0),
      m_db(0)
{
    RegisterCmd("score_for_last_move", &HtpUofAEngine::CmdScoreForLastMove);
    RegisterCmd("reg_genmove", &HtpUofAEngine::CmdRegGenMove);

    RegisterCmd("get_absorb_group", &HtpUofAEngine::CmdGetAbsorbGroup);

    RegisterCmd("book-open", &HtpUofAEngine::CmdBookOpen);
    RegisterCmd("book-expand", &HtpUofAEngine::CmdBookExpand);
    RegisterCmd("book-depths", &HtpUofAEngine::CmdBookMainLineDepth);
    RegisterCmd("book-sizes", &HtpUofAEngine::CmdBookTreeSize);
    RegisterCmd("book-scores", &HtpUofAEngine::CmdBookScores);
    RegisterCmd("book-priorities", &HtpUofAEngine::CmdBookPriorities);

    RegisterCmd("shortest-paths", &HtpUofAEngine::CmdOnShortestPaths);
    RegisterCmd("shortest-vc-paths", &HtpUofAEngine::CmdOnShortestVCPaths);

    RegisterCmd("compute-inferior", &HtpUofAEngine::CmdComputeInferior);
    RegisterCmd("compute-fillin", &HtpUofAEngine::CmdComputeFillin);
    RegisterCmd("compute-vulnerable", &HtpUofAEngine::CmdComputeVulnerable);
    RegisterCmd("compute-dominated", &HtpUofAEngine::CmdComputeDominated);
    RegisterCmd("find-comb-decomp", &HtpUofAEngine::CmdFindCombDecomp);
    RegisterCmd("find-split-decomp", &HtpUofAEngine::CmdFindSplitDecomp);
    RegisterCmd("encode-pattern", &HtpUofAEngine::CmdEncodePattern);

    RegisterCmd("vc-reset", &HtpUofAEngine::CmdResetVcs);

    RegisterCmd("vc-between-cells", &HtpUofAEngine::CmdGetVCsBetween);
    RegisterCmd("vc-connected-to", &HtpUofAEngine::CmdGetCellsConnectedTo);
    RegisterCmd("vc-get-mustplay", &HtpUofAEngine::CmdGetMustPlay);
    RegisterCmd("vc-intersection", &HtpUofAEngine::CmdVCIntersection);
    RegisterCmd("vc-union", &HtpUofAEngine::CmdVCUnion);
    RegisterCmd("vc-maintain", &HtpUofAEngine::CmdVCMaintain);

    RegisterCmd("vc-build", &HtpUofAEngine::CmdBuildStatic);
    RegisterCmd("vc-build-incremental", &HtpUofAEngine::CmdBuildIncremental);
    RegisterCmd("vc-undo-incremental", &HtpUofAEngine::CmdUndoIncremental);

    RegisterCmd("vc-dump", &HtpUofAEngine::CmdDumpVcs);

    RegisterCmd("eval-twod", &HtpUofAEngine::CmdEvalTwoDist);
    RegisterCmd("eval-resist", &HtpUofAEngine::CmdEvalResist);
    RegisterCmd("eval-resist-delta", &HtpUofAEngine::CmdEvalResistDelta);
    RegisterCmd("eval-resist-test", &HtpUofAEngine::CmdResistTest);
    RegisterCmd("eval-influence", &HtpUofAEngine::CmdEvalInfluence);
    RegisterCmd("eval-resist-rotate-check", 
                &HtpUofAEngine::CmdResistCompareRotation);

    RegisterCmd("solve-state", &HtpUofAEngine::CmdSolveState);
    RegisterCmd("solver-clear-tt", &HtpUofAEngine::CmdSolverClearTT);
    RegisterCmd("solver-find-winning", &HtpUofAEngine::CmdSolverFindWinning);

    RegisterCmd("db-open", &HtpUofAEngine::CmdDBOpen);
    RegisterCmd("db-close", &HtpUofAEngine::CmdDBClose);
    RegisterCmd("db-get", &HtpUofAEngine::CmdDBGet);

    RegisterCmd("misc-debug", &HtpUofAEngine::CmdMiscDebug);
}

HtpUofAEngine::~HtpUofAEngine()
{
    DeleteBoard();
}

//----------------------------------------------------------------------------

void HtpUofAEngine::RegisterCmd(const std::string& name,
                               GtpCallback<HtpUofAEngine>::Method method)
{
    Register(name, new GtpCallback<HtpUofAEngine>(this, method));
}

VC::Type
HtpUofAEngine::VCTypeArg(const HtpCommand& cmd, std::size_t number) const
{
    return VCTypeUtil::fromString(cmd.ArgToLower(number));
}

void HtpUofAEngine::PrintVC(HtpCommand& cmd, const VC& vc, 
                            HexColor color) const
{
    cmd << color << " " << vc << '\n';
}

void HtpUofAEngine::DeleteBoard()
{
    if (m_brd) {
        hex::log << hex::fine;
        hex::log << "HtpUofAEngine:: Freeing old board..." << hex::endl;
        delete m_brd;
        m_brd = 0;
    }
}

HexBoard* HtpUofAEngine::SyncBoard(const StoneBoard& board)
{
    // free old board if this board is a different size
    if (m_brd &&
        (m_brd->width() != board.width() ||
         m_brd->height() != board.height()))
    {
        DeleteBoard();
    }

    // allocate new board of correct size
    if (m_brd == 0) {
        hex::log << hex::fine;
        hex::log << "HtpUofAEngine: Constructing new board..." << hex::endl;
        m_brd = new HexBoard(board.width(), board.height(),
                             *m_uofa_program.ICE());
    }

    m_brd->startNewGame();
    m_brd->addColor(BLACK, board.getBlack());
    m_brd->addColor(WHITE, board.getWhite());
    m_brd->setPlayed(board.getPlayed());

    int flags = HexBoard::USE_ICE | HexBoard::USE_VCS;
    if (hex::settings.get_bool("global-use-decompositions")) 
        flags |= HexBoard::USE_DECOMPOSITIONS;
    m_brd->SetFlags(flags);

    return m_brd;
}

/** Generates a move. */
HexPoint HtpUofAEngine::GenMove(HexColor color, double time_remaining)
{
    return Player()->genmove(*SyncBoard(m_game->Board()),
                             *m_game, color, time_remaining, 
                             m_score_for_last_move);
}

////////////////////////////////////////////////////////////////////////
// Commands
////////////////////////////////////////////////////////////////////////

/** Returns score of last move generated by 'genmove'. */
void HtpUofAEngine::CmdScoreForLastMove(HtpCommand& cmd)
{
    cmd << m_score_for_last_move;
}

/** Generates a move, but does not play it. */
void HtpUofAEngine::CmdRegGenMove(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    double score;
    HexPoint move = Player()->genmove(*SyncBoard(m_game->Board()),
                                      *m_game, ColorArg(cmd, 0),
                                      -1, score);
    cmd << HexPointUtil::toString(move);
}

/** Returns the set of stones this stone is part of. */
void HtpUofAEngine::CmdGetAbsorbGroup(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexPoint cell = MoveArg(cmd, 0);
    GroupBoard board(m_game->Board());
    board.absorb();

    if (board.getColor(cell) == EMPTY)
        return;

    HexPoint captain = board.getCaptain(cell);
    cmd << HexPointUtil::toString(captain);

    int c = 1;
    for (BoardIterator p(board.const_locations()); p; ++p) {
        if (*p == captain) continue;
        if (board.getCaptain(*p) == captain) {
            cmd << " " << HexPointUtil::toString(*p);
            if ((++c % 10) ==  0) cmd << "\n";
        }
    }
}

//----------------------------------------------------------------------

/** Opens/Creates an opening book for the current boardsize.
    Usage: "book-expand [filename] {alpha}"
*/
void HtpUofAEngine::CmdBookOpen(HtpCommand& cmd)
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

bool HtpUofAEngine::StateMatchesBook(const StoneBoard& board)
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
void HtpUofAEngine::CmdBookExpand(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    int iterations = cmd.IntArg(0, 1);
    HexBoard* board = SyncBoard(m_game->Board());

    if (!StateMatchesBook(*board))
        return;

    m_book->Expand(*board, iterations);
}

void HtpUofAEngine::CmdBookMainLineDepth(HtpCommand& cmd)
{
    HexBoard* board = SyncBoard(m_game->Board());
    HexColor color = board->WhoseTurn();

    if (!StateMatchesBook(*board))
        return;

    board->ComputeAll(color, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);
    bitset_t consider = PlayerUtils::MovesToConsider(*board, color);
    for (BitsetIterator p(consider); p; ++p) {
        board->playMove(color, *p);
        cmd << " " << HexPointUtil::toString(*p)
            << " " << m_book->GetMainLineDepth(*board, !color);
        board->undoMove(*p);
    }
}

void HtpUofAEngine::CmdBookTreeSize(HtpCommand& cmd)
{
    HexBoard* board = SyncBoard(m_game->Board());
    HexColor color = board->WhoseTurn();

    if (!StateMatchesBook(*board))
        return;

    board->ComputeAll(color, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);
    bitset_t consider = PlayerUtils::MovesToConsider(*board, color);
    for (BitsetIterator p(consider); p; ++p) {
        board->playMove(color, *p);
        cmd << " " << HexPointUtil::toString(*p)
            << " " << m_book->GetTreeSize(*board, !color);
        board->undoMove(*p);
    }
}

void HtpUofAEngine::CmdBookScores(HtpCommand& cmd)
{
    HexBoard* board = SyncBoard(m_game->Board());
    HexColor color = board->WhoseTurn();

    if (!StateMatchesBook(*board))
        return;

    board->ComputeAll(color, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);
    bitset_t consider = PlayerUtils::MovesToConsider(*board, color);
    for (BitsetIterator p(consider); p; ++p) {
        board->playMove(color, *p);
        OpeningBookNode node = m_book->GetNode(board->Hash());

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

        board->undoMove(*p);
    }
}

void HtpUofAEngine::CmdBookPriorities(HtpCommand& cmd)
{
    HexBoard* board = SyncBoard(m_game->Board());
    HexColor color = board->WhoseTurn();

    if (!StateMatchesBook(*board))
        return;

    board->ComputeAll(color, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);
    float alpha = (m_book->GetSettings()).alpha;
    OpeningBookNode parent = m_book->GetNode(board->Hash());
    HexAssert(!parent.IsDummy());

    bitset_t consider = PlayerUtils::MovesToConsider(*board, color);
    for (BitsetIterator p(consider); p; ++p) {
        board->playMove(color, *p);
        OpeningBookNode succ = m_book->GetNode(board->Hash());

        if (!succ.IsDummy()) {
            cmd << " " << HexPointUtil::toString(*p);
            // negated because we're looking at it from the parent state
            float value = parent.ComputeExpPriority(succ.getPropValue(), alpha,
						    succ.getExpPriority());
	    cmd << " " << std::fixed << std::setprecision(1) << value;
        }

        board->undoMove(*p);
    }
}

//----------------------------------------------------------------------

/** Finds the set of empty cells on shortest edge-to-edge paths for
    the given colour using adjacencies that are direct or through
    groups. */
void HtpUofAEngine::CmdOnShortestPaths(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor color = ColorArg(cmd, 0);

    HexBoard* board = SyncBoard(m_game->Board());
    board->ComputeAll(color, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);

    PrintBitsetToHTP(cmd,
                     GraphUtils::CellsOnShortestWinningPaths(*board, color));
}

/** Same as above but uses VCs for adjacencies instead. */
void HtpUofAEngine::CmdOnShortestVCPaths(HtpCommand& cmd)
{
    cmd.CheckNuArgLessEqual(2);
    HexColor color = ColorArg(cmd, 0);
    bool inclEdges = (cmd.NuArg() == 2);

    HexBoard* board = SyncBoard(m_game->Board());
    board->ComputeAll(color, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);

    PrintBitsetToHTP(cmd,
                     GraphUtils::CellsOnShortestWinningVCPaths(*board,
                                                               color,
                                                               inclEdges));
}

/** Does inferior cell analysis. First argument is the color of the
    player. */
void HtpUofAEngine::CmdComputeInferior(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor color = ColorArg(cmd, 0);

    HexBoard* board = SyncBoard(m_game->Board());
    board->update();
    board->absorb();

    InferiorCells inf;
    m_uofa_program.ICE()->computeInferiorCells(color, *board, inf);

    cmd << inf.GuiOutput();
    cmd << "\n";
}

/** Computes fillin for the given board. Color argument affects order
    for computing vulnerable/presimplicial pairs. */
void HtpUofAEngine::CmdComputeFillin(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor color = ColorArg(cmd, 0);

    HexBoard* board = SyncBoard(m_game->Board());
    board->update();
    board->absorb();

    InferiorCells inf;
    m_uofa_program.ICE()->computeFillin(color, *board, inf);
    inf.ClearVulnerable();

    cmd << inf.GuiOutput();
    cmd << "\n";
}

/** Computes vulnerable cells on the current board for the given color. */
void HtpUofAEngine::CmdComputeVulnerable(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor col = ColorArg(cmd, 0);

    HexBoard* board = SyncBoard(m_game->Board());
    board->update();
    board->absorb();

    InferiorCells inf;
    m_uofa_program.ICE()->findVulnerable(*board, col, board->getEmpty(), inf);

    cmd << inf.GuiOutput();
    cmd << "\n";
}

/** Computes dominated cells on the current board for the given color. */
void HtpUofAEngine::CmdComputeDominated(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor col = ColorArg(cmd, 0);

    HexBoard* board = SyncBoard(m_game->Board());
    board->update();
    board->absorb();

    InferiorCells inf;
    m_uofa_program.ICE()->findDominated(*board, col, board->getEmpty(), inf);

    cmd << inf.GuiOutput();
    cmd << "\n";
}

// tries to find a combinatorial decomposition of the board state
void HtpUofAEngine::CmdFindCombDecomp(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor color = ColorArg(cmd, 0);

    HexBoard* brd = SyncBoard(m_game->Board());
    brd->ComputeAll(BLACK, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);

    bitset_t capturedVC;
    if (BoardUtils::FindCombinatorialDecomposition(*brd, color, capturedVC)) {
        hex::log << hex::info << "Found decomposition!" << hex::endl;
        PrintBitsetToHTP(cmd, capturedVC);
    }
}

void HtpUofAEngine::CmdFindSplitDecomp(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor color = ColorArg(cmd, 0);

    HexBoard* brd = SyncBoard(m_game->Board());
    brd->ComputeAll(BLACK, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);
    HexPoint group;
    bitset_t capturedVC;
    if (BoardUtils::FindSplittingDecomposition(*brd, color, group,
					       capturedVC)) {
        hex::log << hex::info << "Found split decomp: "
                 << HexPointUtil::toString(group) << "!"
                 << hex::endl;
        PrintBitsetToHTP(cmd, capturedVC);
    }
}

void HtpUofAEngine::CmdEncodePattern(HtpCommand& cmd)
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

void HtpUofAEngine::CmdResetVcs(HtpCommand& UNUSED(cmd))
{
    DeleteBoard();
}

void HtpUofAEngine::CmdBuildStatic(HtpCommand& cmd)
{
    cmd.CheckNuArgLessEqual(2);

    int max_ors = Connections::DEFAULT;
    HexColor color = ColorArg(cmd, 0);
    if (cmd.NuArg() == 2)
        max_ors = cmd.IntArg(1, 2);

    HexBoard* brd = SyncBoard(m_game->Board());
    brd->ComputeAll(color, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN, max_ors);
    cmd << brd->getInferiorCells().GuiOutput();
    if (!PlayerUtils::IsDeterminedState(*brd, color))
    {
        bitset_t consider = PlayerUtils::MovesToConsider(*brd, color);
        cmd << BoardUtils::GuiDumpOutsideConsiderSet(*brd, consider,
                                              brd->getInferiorCells().All());
    }
    cmd << "\n";
}

void HtpUofAEngine::CmdBuildIncremental(HtpCommand& cmd)
{
    cmd.CheckNuArgLessEqual(3);
    int max_ors = Connections::DEFAULT;
    HexColor color = ColorArg(cmd, 0);
    HexPoint point = MoveArg(cmd, 1);
    if (cmd.NuArg() == 3)
        max_ors = cmd.IntArg(2, 2);

    Board()->PlayMove(color, point, max_ors);
    cmd << Board()->getInferiorCells().GuiOutput();
    if (!PlayerUtils::IsDeterminedState(*Board(), color))
    {
        bitset_t consider = PlayerUtils::MovesToConsider(*Board(), color);
        cmd << BoardUtils::GuiDumpOutsideConsiderSet(*Board(), consider,
                                           Board()->getInferiorCells().All());
    }

    cmd << "\n";
}

void HtpUofAEngine::CmdUndoIncremental(HtpCommand& UNUSED(cmd))
{
    Board()->UndoMove();
}

/** Returns a list of VCs between the given two cells.
    Format: "vc-between-cells x y c t", where x and y are the cells,
    c is the color of the player, and t is the type of connection
    (0-conn, 1-conn, etc). */
void HtpUofAEngine::CmdGetVCsBetween(HtpCommand& cmd)
{
    cmd.CheckNuArg(4);
    HexPoint from = MoveArg(cmd, 0);
    HexPoint to = MoveArg(cmd, 1);
    HexColor color = ColorArg(cmd, 2);
    VC::Type ctype = VCTypeArg(cmd, 3);

    HexBoard* brd = Board();
    HexPoint fcaptain = brd->getCaptain(from);
    HexPoint tcaptain = brd->getCaptain(to);

    std::vector<VC> vc;
    brd->Cons(color).getVCs(fcaptain, tcaptain, ctype, vc);
    const VCList& lst = brd->Cons(color).getVCList(fcaptain, tcaptain, ctype);

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
void HtpUofAEngine::CmdGetCellsConnectedTo(HtpCommand& cmd)
{
    cmd.CheckNuArg(3);
    HexPoint from = MoveArg(cmd, 0);
    HexColor color = ColorArg(cmd, 1);
    VC::Type ctype = VCTypeArg(cmd, 2);
    bitset_t pt = Board()->Cons(color).ConnectedTo(from, ctype);
    PrintBitsetToHTP(cmd, pt);
}

void HtpUofAEngine::CmdGetMustPlay(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor color = ColorArg(cmd, 0);
    bitset_t mustplay = Board()->getMustplay(color);
    InferiorCells inf(Board()->getInferiorCells());
    inf.ClearVulnerable();
    inf.ClearDominated();
    cmd << inf.GuiOutput();
    if (!PlayerUtils::IsDeterminedState(*Board(), color))
    {
        bitset_t consider = PlayerUtils::MovesToConsider(*Board(), color);
        cmd << BoardUtils::GuiDumpOutsideConsiderSet(*Board(), consider,
                                                     inf.All());
    }
}

void HtpUofAEngine::CmdVCIntersection(HtpCommand& cmd)
{
    cmd.CheckNuArg(4);
    HexPoint from = MoveArg(cmd, 0);
    HexPoint to = MoveArg(cmd, 1);
    HexColor color = ColorArg(cmd, 2);
    VC::Type ctype = VCTypeArg(cmd, 3);

    HexBoard* brd = Board();
    HexPoint fcaptain = brd->getCaptain(from);
    HexPoint tcaptain = brd->getCaptain(to);
    const VCList& lst = brd->Cons(color).getVCList(fcaptain, tcaptain, ctype);
    bitset_t intersection = lst.hardIntersection();

    PrintBitsetToHTP(cmd, intersection);
}

void HtpUofAEngine::CmdVCUnion(HtpCommand& cmd)
{
    cmd.CheckNuArg(4);
    HexPoint from = MoveArg(cmd, 0);
    HexPoint to = MoveArg(cmd, 1);
    HexColor color = ColorArg(cmd, 2);
    VC::Type ctype = VCTypeArg(cmd, 3);

    HexBoard* brd = Board();
    HexPoint fcaptain = brd->getCaptain(from);
    HexPoint tcaptain = brd->getCaptain(to);
    const VCList& lst = brd->Cons(color).getVCList(fcaptain, tcaptain, ctype);
    bitset_t un = lst.getGreedyUnion(); // FIXME: shouldn't be greedy!!

    PrintBitsetToHTP(cmd, un);
}

/** Returns a list of VCs that can be maintained by mohex for the
    player of the appropriate color. */
void HtpUofAEngine::CmdVCMaintain(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor color = ColorArg(cmd, 0);

    HexBoard* brd = Board();
    std::vector<VC> maintain;
    VCUtils::findMaintainableVCs(*brd, color, maintain);

    cmd << "\n";
    std::vector<VC>::const_iterator it;
    for (it = maintain.begin(); it != maintain.end(); ++it) 
        PrintVC(cmd, *it, color);
}

void HtpUofAEngine::CmdDumpVcs(HtpCommand& cmd)
{
    cmd.CheckNuArg(2);
    HexColor color = ColorArg(cmd, 0);
    VC::Type ctype = VCTypeArg(cmd, 1);
    hex::log << hex::info << Board()->Cons(color).dump(ctype) << hex::endl;
}

//----------------------------------------------------------------------------
// Evaluation commands
//----------------------------------------------------------------------------

void HtpUofAEngine::CmdEvalTwoDist(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor color = ColorArg(cmd, 0);

    HexBoard* brd = SyncBoard(m_game->Board());
    brd->ComputeAll(color, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);
    TwoDistance twod(TwoDistance::ADJACENT);
    twod.Evaluate(*brd);

    for (BoardIterator it(brd->const_cells()); it; ++it) {
        if (brd->isOccupied(*it)) continue;
        HexEval energy = twod.Score(*it, color);
        if (energy == EVAL_INFINITY)
            energy = -1;

        cmd << " " << HexPointUtil::toString(*it)
            << " " << energy;
    }
}

void HtpUofAEngine::CmdEvalResist(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor color = ColorArg(cmd, 0);

    HexBoard* brd = SyncBoard(m_game->Board());
    brd->ComputeAll(color, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);
    Resistance resist;
    resist.Evaluate(*brd);

    cmd << " res " << std::fixed << std::setprecision(3) << resist.Score()
        << " rew " << std::fixed << std::setprecision(3) << resist.Resist(WHITE)
        << " reb " << std::fixed << std::setprecision(3) << resist.Resist(BLACK);

    for (BoardIterator it(brd->const_cells()); it; ++it) {
        if (brd->isOccupied(*it)) continue;
        HexEval energy = resist.Score(*it, color);
        if (energy == EVAL_INFINITY)
            energy = -1;

        cmd << " " << HexPointUtil::toString(*it)
            << " " << std::fixed << std::setprecision(3) << energy;
    }
}

void HtpUofAEngine::CmdEvalResistDelta(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor color = ColorArg(cmd, 0);

    HexBoard* brd = SyncBoard(m_game->Board());
    brd->ComputeAll(color, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);
    Resistance resist;
    resist.Evaluate(*brd);
    HexEval base = resist.Score();

    cmd << " res " << std::fixed << std::setprecision(3) << base;
    for (BitsetIterator it(brd->getEmpty()); it; ++it) {
        brd->PlayMove(color, *it);

        resist.Evaluate(*brd);
        HexEval cur = resist.Score();

        cmd << " " << HexPointUtil::toString(*it)
            << " " << std::fixed << std::setprecision(3) << (cur - base);

        brd->UndoMove();
    }
}

void HtpUofAEngine::CmdResistCompareRotation(HtpCommand& cmd)
{
    cmd.CheckNuArg(0);

    HexBoard* brd = SyncBoard(m_game->Board());

    const StoneBoard& cbd = m_game->Board();
    HexBoard bd1(cbd.width(), cbd.height(), brd->ICE());
    HexBoard bd2(cbd.width(), cbd.height(), brd->ICE());
    bd1.SetFlags(HexBoard::USE_VCS);
    bd2.SetFlags(HexBoard::USE_VCS);

    bd1.startNewGame();
    bd1.addColor(BLACK, brd->getBlack());
    bd1.addColor(WHITE, brd->getWhite());
    bd1.setPlayed(brd->getPlayed());

    bd2.startNewGame();
    bd2.addColor(BLACK, brd->getBlack());
    bd2.addColor(WHITE, brd->getWhite());
    bd2.setPlayed(brd->getPlayed());
    bd2.rotateBoard();
    
    bd1.ComputeAll(BLACK, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);
    Resistance resist;
    resist.Evaluate(bd1);
    HexEval base1 = resist.Score();
    HexEval rew1 = resist.Resist(WHITE);
    HexEval reb1 = resist.Resist(BLACK);

    bd2.ComputeAll(BLACK, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);
    resist.Evaluate(bd2);
    HexEval base2 = resist.Score();
    HexEval rew2 = resist.Resist(WHITE);
    HexEval reb2 = resist.Resist(BLACK);
    
    bool resist_differ = false;
    bool circuits_differ = false;

    // compare evaluation scores
    if (fabs(base1 - base2) > 0.001) 
    {
        hex::log << hex::info;
        hex::log << "######################" << hex::endl;
        hex::log << bd1 << hex::endl;
        hex::log << "res = " << std::fixed << base2 << hex::endl
                 << "rew = " << std::fixed << rew2 << hex::endl
                 << "reb = " << std::fixed << reb2 << hex::endl;

        
        hex::log << bd2 << hex::endl;
        hex::log << "res = " << std::fixed << base1 << hex::endl
                 << "rew = " << std::fixed << rew1 << hex::endl
                 << "reb = " << std::fixed << reb1 << hex::endl;
        hex::log << "######################" << hex::endl;
        resist_differ = true;
    }

    // compare the vc connected-to sets
    int differences[BLACK_AND_WHITE] = {0, 0};
    for (BWIterator c; c; ++c) 
    {
        for (BoardIterator p1(bd1.Groups()); p1; ++p1) 
        {
            for (BoardIterator p2(bd1.Groups()); *p2 != *p1; ++p2) 
            {
                HexPoint rp1 = bd1.rotate(*p1);
                HexPoint rp2 = bd1.rotate(*p2);

                bool b1 = bd1.Cons(*c).doesVCExist(*p1, *p2, VC::FULL);
                bool b2 = bd2.Cons(*c).doesVCExist(rp1, rp2, VC::FULL);
                if (b1 != b2) 
                {
                    hex::log << hex::info
                             << "Circuits differ ("
                             << *c << "): "
                             << "(" << HexPointUtil::toString(*p1)
                             << ", "<< HexPointUtil::toString(*p2)
                             << ") != (" << HexPointUtil::toString(rp1)
                             << ", "<< HexPointUtil::toString(rp2)
                             << ") " << b1 << " " << b2
                             << hex::endl;
                    circuits_differ = true;
                    differences[*c]++;
                }
            }
        }
    }
    HexAssert(!resist_differ || circuits_differ);

    hex::log << hex::info 
             << "Black differences: " << differences[BLACK] << hex::endl
             << "White differences: " << differences[WHITE] << hex::endl;

    if (resist_differ || circuits_differ) cmd << "failed";
    else cmd << "passed";
}


void HtpUofAEngine::CmdResistTest(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    int n = cmd.IntArg(0);

    HexBoard* brd = SyncBoard(m_game->Board());

    const StoneBoard& cbd = m_game->Board();
    HexBoard bd1(cbd.width(), cbd.height(), brd->ICE());
    HexBoard bd2(cbd.width(), cbd.height(), brd->ICE());

    int failed = 0;
    for (int i=0; i<n; ++i) {

        hex::log << hex::info << "Test " << i << ":";

        bd1.startNewGame();
        bd2.startNewGame();
        HexColor c = BLACK;
        for (int j=0; j<20; ++j) {
            HexPoint p = BoardUtils::RandomEmptyCell(bd1);
            hex::log << " " << HexPointUtil::toString(p);
            bd1.playMove(c, p);
            bd2.playMove(c, bd1.rotate(p));
            c = HexColorUtil::otherColor(c);
        }
        hex::log << hex::endl;
        hex::log << bd1 << hex::endl;

        bd1.ComputeAll(BLACK, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);
        Resistance resist;
        resist.Evaluate(bd1);
        HexEval base1 = resist.Score();
        HexEval rew1 = resist.Resist(WHITE);
        HexEval reb1 = resist.Resist(BLACK);

        bd2.ComputeAll(BLACK, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);
        resist.Evaluate(bd2);
        HexEval base2 = resist.Score();
        HexEval rew2 = resist.Resist(WHITE);
        HexEval reb2 = resist.Resist(BLACK);

        bool resist_differ = false;
        bool circuits_differ = false;

        if (fabs(base1 - base2) > 0.001) {
            hex::log << hex::info;
            hex::log << "######################" << hex::endl;
            hex::log << bd1 << hex::endl;
            hex::log << "res = " << std::fixed << base2 << hex::endl
                     << "rew = " << std::fixed << rew2 << hex::endl
                     << "reb = " << std::fixed << reb2 << hex::endl;


            hex::log << bd2 << hex::endl;
            hex::log << "res = " << std::fixed << base1 << hex::endl
                     << "rew = " << std::fixed << rew1 << hex::endl
                     << "reb = " << std::fixed << reb1 << hex::endl;
            hex::log << "######################" << hex::endl;
            resist_differ = true;
            failed++;
        }

        // compare the vc connected-to sets
        for (BoardIterator p1(bd1.Groups()); p1; ++p1) {
            for (BoardIterator p2(bd1.Groups()); *p2 != *p1; ++p2) {
                HexPoint rp1 = bd1.rotate(*p1);
                HexPoint rp2 = bd1.rotate(*p2);
                for (BWIterator c; c; ++c) {
                    bool b1 = bd1.Cons(*c).doesVCExist(*p1, *p2, VC::FULL);
                    bool b2 = bd2.Cons(*c).doesVCExist(rp1, rp2, VC::FULL);

                    if (b1 != b2) {
                        hex::log << hex::info
                                 << "Circuits differ ("
                                 << *c << "): "
                                 << "(" << HexPointUtil::toString(*p1)
                                 << ", "<< HexPointUtil::toString(*p2)
                                 << ") != (" << HexPointUtil::toString(rp1)
                                 << ", "<< HexPointUtil::toString(rp2)
                                 << ") " << b1 << " " << b2
                                 << hex::endl;
                        circuits_differ = true;
                    }
                }
            }
        }

        HexAssert(!resist_differ || circuits_differ);

        bd2.rotateBoard();
        if (bd1 != bd2) {
            hex::log << hex::info;
            hex::log << "******************************" << hex::endl;
            hex::log << "STONES NOT EQUAL UNDER ROTATION!" << hex::endl;
            hex::log << bd1 << hex::endl;
            bd2.rotateBoard();
            hex::log << bd2 << hex::endl;
            hex::log << "******************************" << hex::endl;
        }
    }
    hex::log << hex::info
             << "Passed " << (n - failed) << " / " << n << " tests."
             << hex::endl;

    if (failed) cmd << "failed";
    else cmd << "passed";
}

void HtpUofAEngine::CmdEvalInfluence(HtpCommand& cmd)
{
    cmd.CheckNuArg(1);
    HexColor color = ColorArg(cmd, 0);

    HexBoard* brd = SyncBoard(m_game->Board());
    brd->ComputeAll(color, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);

    // Pre-compute edge adjacencies
    bitset_t northNbs = brd->Cons(BLACK).ConnectedTo(NORTH, VC::FULL);
    bitset_t southNbs = brd->Cons(BLACK).ConnectedTo(SOUTH, VC::FULL);
    bitset_t eastNbs = brd->Cons(WHITE).ConnectedTo(EAST, VC::FULL);
    bitset_t westNbs = brd->Cons(WHITE).ConnectedTo(WEST, VC::FULL);

    for (BoardIterator it(brd->const_cells()); it; ++it) {
        if (brd->isOccupied(*it)) continue;

	// Compute neighbours, giving over-estimation to edges
	bitset_t b1 = brd->Cons(BLACK).ConnectedTo(*it, VC::FULL);
	if (b1.test(NORTH)) b1 |= northNbs;
	if (b1.test(SOUTH)) b1 |= southNbs;
	b1 &= brd->getEmpty();
	bitset_t b2 = brd->Cons(WHITE).ConnectedTo(*it, VC::FULL);
	if (b2.test(EAST)) b2 |= eastNbs;
	if (b2.test(WEST)) b2 |= westNbs;
	b2 &= brd->getEmpty();

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

void HtpUofAEngine::CmdSolveState(HtpCommand& cmd)
{
    cmd.CheckNuArgLessEqual(4);
    HexColor color = ColorArg(cmd, 0);

    bool use_db = false;
    std::string filename = "dummy";
    int maxstones = hex::settings.get_int("solverdb-default-maxstones");
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

    double timelimit = hex::settings.get_double("solver-timelimit");
    int depthlimit = hex::settings.get_int("solver-depthlimit");

    HexBoard* brd = SyncBoard(m_game->Board());
    Solver* solver = m_uofa_program.GetSolver();

    Solver::SolutionSet solution;
    Solver::Result result = (use_db) ?
        solver->solve(*brd, color, filename, maxstones, transtones, solution,
                      depthlimit, timelimit) :
        solver->solve(*brd, color, solution, depthlimit, timelimit);

    solver->DumpStats(solution);

    HexColor winner = EMPTY;
    if (result != Solver::UNKNOWN) {
        winner = (result==Solver::WIN) ? color : !color;
        hex::log << hex::info;
        hex::log << winner << " wins!" << hex::endl;
        hex::log << brd->printBitset(solution.proof) << hex::endl;
    } else {
        hex::log << hex::info;
        hex::log << "Search aborted!" << hex::endl;
    }
    cmd << winner;
}

void HtpUofAEngine::CmdSolverClearTT(HtpCommand& UNUSED(cmd))
{
    m_uofa_program.GetSolver()->ClearTT();
}

void HtpUofAEngine::CmdSolverFindWinning(HtpCommand& cmd)
{
    cmd.CheckNuArgLessEqual(4);
    HexColor color = ColorArg(cmd, 0);
    HexColor other = !color;

    bool use_db = false;
    std::string filename = "dummy";
    int maxstones = hex::settings.get_int("solverdb-default-maxstones");
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

    Solver* solver = m_uofa_program.GetSolver();
    HexBoard* board = SyncBoard(m_game->Board());
    board->ComputeAll(color, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);

    bitset_t consider = PlayerUtils::MovesToConsider(*board, color);
    bitset_t winning;

    for (BitsetIterator p(consider); p; ++p)
    {
	if (!consider.test(*p)) continue;

        StoneBoard brd(m_game->Board());
        brd.playMove(color, *p);

        HexBoard* board = SyncBoard(brd);

        bitset_t proof;
        std::vector<HexPoint> pv;

        hex::log << hex::info
                 << "****** Trying " << HexPointUtil::toString(*p)
                 << " ******" << hex::endl
                 << brd << hex::endl;

        HexColor winner = EMPTY;
        Solver::SolutionSet solution;
        Solver::Result result = (use_db) ?
            solver->solve(*board, other, filename, maxstones, transtones, solution):
            solver->solve(*board, other, solution);
        solver->DumpStats(solution);
        hex::log << hex::info
                 << "Proof:" << board->printBitset(solution.proof)
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

void HtpUofAEngine::CmdDBOpen(HtpCommand& cmd)
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

void HtpUofAEngine::CmdDBClose(HtpCommand& cmd)
{
    cmd.CheckNuArg(0);

    if (m_db) delete m_db;
    m_db = 0;

}

void HtpUofAEngine::CmdDBGet(HtpCommand& cmd)
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

void HtpUofAEngine::CmdMiscDebug(HtpCommand& cmd)
{
//     cmd.CheckNuArg(1);
//     HexPoint point = MoveArg(cmd, 0);
    cmd << *Board() << hex::endl;
}

//----------------------------------------------------------------------------
// Interrupt/Pondering

void HtpUofAEngine::InitPonder()
{
}

void HtpUofAEngine::Ponder()
{
}

void HtpUofAEngine::StopPonder()
{
}

void HtpUofAEngine::Interrupt()
{
}

//----------------------------------------------------------------------------
