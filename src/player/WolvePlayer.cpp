//----------------------------------------------------------------------------
// $Id: WolvePlayer.cpp 1715 2008-10-28 22:13:37Z broderic $
//----------------------------------------------------------------------------

#include "BitsetIterator.hpp"
#include "HexEval.hpp"
#include "PlayerUtils.hpp"
#include "SequenceHash.hpp"
#include "WolvePlayer.hpp"

//----------------------------------------------------------------------------

WolvePlayer::WolvePlayer()
    : UofAPlayer() 
{
    hex::log << hex::fine << "--- WolvePlayer" << hex::endl;
}

WolvePlayer::~WolvePlayer()
{
}

//----------------------------------------------------------------------------

HexPoint WolvePlayer::search(HexBoard& brd, 
                             const Game& UNUSED(game_state),
			     HexColor color, 
                             const bitset_t& consider,
                             double time_remaining,
                             double& score)
{
    hex::log << hex::fine << "WolvePlayer::search()" << hex::endl;

    hex::log << hex::info << "Using consider set:" 
             << brd.printBitset(consider) << hex::endl;

    WolveSearch wolve_search(brd, color, consider);
    wolve_search.SetGuiFx(hex::settings.get_bool("wolve-use-guifx"));
    
    std::vector<int> plywidth;
    std::vector<HexPoint> PV;

    int depth = hex::settings.get_int("wolve-max-depth");

    // remove all -'s from the plywidths string.
    std::string widths = hex::settings.get("wolve-ply-width");
    for (std::size_t i=0; i<widths.size(); ++i)
        if (widths[i] == '-') widths[i] = ' ';

    std::istringstream is;
    is.str(widths);
    hex::log << hex::info << "Plywidths:";
    for (int i=0; is && i<depth; ++i) {
        int j;
        is >> j;
        plywidth.push_back(j);
        hex::log << " " << j;
    }
    hex::log << hex::endl;

    //
    // Initialize a TT if needed
    //
    TransTable<SearchedState>* tt = 0;
    if (hex::settings.get_bool("wolve-use-tt")) {
        int bits = hex::settings.get_int("wolve-tt-size");
        hex::log << hex::info << "Creating TT with " 
                 << (1<<bits) << " states..." << hex::endl;
        tt = new TransTable<SearchedState>(bits);
        wolve_search.SetTT(tt);
    }

    std::vector<int> depths;
    if (hex::settings.get_bool("wolve-use-iterative-deepening"))
    {
        // Always do the 1-ply search so that backing-up 
        // can shrink the root moves. Only do even ply from then
        // on because we want to be pessimistic--optimistic searching
        // leads to bad results in Hex.
        depths.push_back(1);
        for (std::size_t i=2; i<=plywidth.size(); i+=2)
            depths.push_back(i);
    }
    else 
    {
        depths.push_back(plywidth.size());
    }

    int timelimit = hex::settings.get_int("wolve-timelimit");

    // 
    // Do the search
    //
    score = wolve_search.Search(plywidth, 
                                depths, 
                                timelimit, 
                                PV);

    //
    // Dump info on search
    //
    hex::log << hex::info << wolve_search.DumpStats() << hex::endl;

    std::vector<HexMoveValue> root_evals(wolve_search.RootMoveScores());
    stable_sort(root_evals.begin(), root_evals.end(),
                std::greater<HexMoveValue>());

    int num = hex::settings.get_int("wolve-show-root-scores");
    for (int i=0; i<num && i<(int)root_evals.size(); ++i) {
        hex::log << " (" << root_evals[i].point()
                 << ", " << std::setprecision(3) 
                 << root_evals[i].value() << ")";
    }
    hex::log << hex::endl;

    if (tt) delete tt;

    // truncate score to be within [-1, 1].
    // @todo Is this ok? 
    if (score > 1.0) score = 1.0;
    else if (score < -1.0) score = -1.0;
    
    HexAssert(PV.size() > 0);
    return PV[0];
}

//----------------------------------------------------------------------------

WolveWorkThread::WolveWorkThread(HexBoard& brd, 
                                 boost::barrier& start,
                                 boost::barrier& completed,
                                 const WolveThreadAction& action,
                                 const HexPoint& move,
                                 const HexColor& color)
    : m_brd(brd),
      m_start(start),
      m_completed(completed),
      m_action(action),
      m_move(move),
      m_color(color)
{
}

void WolveWorkThread::operator()()
{
    bool finished = false;
    while (true)
    {
        m_start.wait();

        switch(m_action) {
        case WOLVE_THREAD_QUIT:
            finished = true;
            break;

        case WOLVE_THREAD_UNDO:
            m_brd.UndoMove();
            break;

        case WOLVE_THREAD_PLAY:
            m_brd.PlayMove(m_color, m_move);
            break;
        }

        if (finished) break;

        m_completed.wait();
    }

    hex::log << hex::info << "Exiting..." << hex::endl;
}

//----------------------------------------------------------------------------

WolveSearch::WolveSearch(HexBoard& board, HexColor color, 
                         const bitset_t& rootMTC)
    : HexAbSearch(board, color),

      m_no_fillin_board(),
      m_rootMTC(rootMTC),

      m_varTT(16),           // 16bit variation trans-table

      m_start_threads(3),
      m_threads_completed(3)
{
}

WolveSearch::~WolveSearch()
{
}

void WolveSearch::OnStartSearch()
{
    if (!m_no_fillin_board.get() ||
        m_no_fillin_board->width() != m_brd.width() || 
        m_no_fillin_board->height() != m_brd.height())
    {
        hex::log << hex::info << "Wolve: Creating new board..." << hex::endl;
        m_no_fillin_board.reset
            (new HexBoard(m_brd.width(), m_brd.height(), m_brd.ICE()));
    }

    SetupNonFillinBoard();
    StartThreads();
}

void WolveSearch::EnteredNewState()
{
}

HexEval WolveSearch::Evaluate()
{
    Resistance resist;
    ComputeResistance(resist);

    HexEval score = (m_toplay == BLACK) 
        ?  resist.Score() 
        : -resist.Score();

    hex::log << hex::fine << "Score for " << m_toplay
             << ": " << score << hex::endl;

    return score;
}

void WolveSearch::GenerateMoves(std::vector<HexPoint>& moves)
{
    HexAssert(BitsetUtil::IsSubsetOf(m_brd.getEmpty(), 
                                     m_no_fillin_board->getEmpty()));

    Resistance resist;
    ComputeResistance(resist);

    // Get moves to consider:
    //   1) from the variation tt, if this variation has been visited before.
    //   2) from the passed in consider set, if at the root.
    //   3) from computing it ourselves.
    bitset_t consider;

    VariationInfo varInfo;
    if (m_varTT.get(SequenceHash::Hash(m_sequence), varInfo)
        && hex::settings.get_bool("wolve-backup-ice-info")) 
    {
        hex::log << hex::info << "Using consider set from TT." << hex::endl;
        hex::log << HexPointUtil::ToPointListString(m_sequence) << hex::endl;
        hex::log << m_brd << hex::endl;
        HexAssert(hex::settings.get_bool("wolve-use-iterative-deepening"));
        consider = varInfo.consider;
    } 
    else if (m_current_depth == 0)
    {
        hex::log << hex::info << "Using root consider set." << hex::endl;
        consider = m_rootMTC;
    }
    else 
    {
        hex::log<<hex::info << "Computing our own consider set." << hex::endl;
        consider = PlayerUtils::MovesToConsider(m_brd, m_toplay);
    }

    m_consider.push_back(consider);
    HexAssert((int)m_consider.size() == m_current_depth+1);

    // order the moves
    moves.clear();
    std::vector<std::pair<HexEval, HexPoint> > mvsc;
    for (BitsetIterator it(consider); it; ++it) {

        // prefer the best move from the TT if possible
        HexEval score = (*it == m_tt_bestmove) 
            ? 10000
            : resist.Score(*it);

        mvsc.push_back(std::make_pair(-score, *it));
    }

    /** @note to ensure we are deterministic, we must use stable_sort. */
    stable_sort(mvsc.begin(), mvsc.end());
    
    for (unsigned i=0; i<mvsc.size(); ++i) {
        moves.push_back(mvsc[i].second);
    }
}

void WolveSearch::ExecuteMove(HexPoint move)
{
    if (hex::settings.get_bool("wolve-use-threads"))
    {
        m_threadAction = WOLVE_THREAD_PLAY;
        m_threadMove = move;
        m_threadColor = m_toplay;
        m_start_threads.wait();
        m_threads_completed.wait();
    } 
    else
    {
        //hex::log << hex::info << m_brd << hex::endl
        //         << HexPointUtil::ToPointListString(m_sequence) << hex::endl;
        m_brd.PlayMove(m_toplay, move);
        m_no_fillin_board->PlayMove(m_toplay, move);
    }
}

void WolveSearch::UndoMove(HexPoint UNUSED(move))
{
    if (hex::settings.get_bool("wolve-use-threads"))
    {
        m_threadAction = WOLVE_THREAD_UNDO;
        m_start_threads.wait();
        m_threads_completed.wait();
    } 
    else
    {
        m_brd.UndoMove();
        m_no_fillin_board->UndoMove();
    }
}

void WolveSearch::AfterStateSearched()
{
    if (hex::settings.get_bool("wolve-backup-ice-info")) {

        bitset_t old_consider = m_consider[m_current_depth];
        bitset_t new_consider 
            = PlayerUtils::MovesToConsider(m_brd, m_toplay) & old_consider;
              
        if (new_consider.count() < old_consider.count()) {
            hex::log << hex::info << "Shrank moves to consider by " 
                     << (old_consider.count() - new_consider.count()) 
                     << ", from:" << m_brd.printBitset(old_consider) 
                     << hex::endl << "to: " << m_brd.printBitset(new_consider)
                     << hex::endl;
        }

        // store new consider set in varTT
        hash_t hash = SequenceHash::Hash(m_sequence);
        m_varTT.put(VariationInfo(hash, m_current_depth, new_consider));
    }

    m_consider.pop_back();
}

void WolveSearch::OnSearchComplete()
{
    StopThreads();
}

//----------------------------------------------------------------------------

void WolveSearch::SetupNonFillinBoard()
{
    m_no_fillin_board->startNewGame();
    m_no_fillin_board->setColor(BLACK, m_brd.getBlack());
    m_no_fillin_board->setColor(WHITE, m_brd.getWhite());    
    m_no_fillin_board->setPlayed(m_brd.getPlayed());
    // do not use ICE or decomps on our board
    m_no_fillin_board->SetFlags(HexBoard::USE_VCS);
    m_no_fillin_board->ComputeAll(m_toplay, 
                                  // note this does not really matter
                                  // since we aren't using ice anyway!
                                  HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);
}

void WolveSearch::ComputeResistance(Resistance& resist)
{
    AdjacencyGraph graphs[BLACK_AND_WHITE];
    ResistanceUtil::AddAdjacencies(*m_no_fillin_board, graphs);
    ResistanceUtil::AddAdjacencies(m_brd, graphs);
    resist.Evaluate(*m_no_fillin_board, graphs);
}

void WolveSearch::StartThreads()
{
    if (!hex::settings.get_bool("wolve-use-threads"))
        return;

    hex::log << hex::info << "WolveSearch::StartThreads()" << hex::endl;

    m_thread[0].reset(new boost::thread(WolveWorkThread(m_brd, 
                                                        m_start_threads,
                                                        m_threads_completed,
                                                        m_threadAction,
                                                        m_threadMove,
                                                        m_threadColor)));
    
    m_thread[1].reset(new boost::thread(WolveWorkThread(*m_no_fillin_board, 
                                                        m_start_threads,
                                                        m_threads_completed,
                                                        m_threadAction,
                                                        m_threadMove,
                                                        m_threadColor)));
}

void WolveSearch::StopThreads()
{
    if (!hex::settings.get_bool("wolve-use-threads"))
        return;

    hex::log << hex::info << "WolveSearch::StopThreads()" << hex::endl;

    m_threadAction = WOLVE_THREAD_QUIT;
    m_start_threads.wait();
    
    m_thread[0]->join();
    m_thread[1]->join();

    m_thread[0].reset();
    m_thread[1].reset();
}

//----------------------------------------------------------------------------
