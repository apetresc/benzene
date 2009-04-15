//----------------------------------------------------------------------------
// $Id: MoHexPlayer.cpp 1898 2009-02-05 02:01:04Z broderic $
//----------------------------------------------------------------------------

#include "SgSystem.h"

#include "SgTimer.h"

#include "BitsetIterator.hpp"
#include "BoardUtils.hpp"
#include "Connections.hpp"
#include "HexUctUtil.hpp"
#include "HexUctKnowledge.hpp"
#include "HexUctSearch.hpp"
#include "HexUctPolicy.hpp"
#include "MoHexPlayer.hpp"
#include "PlayerUtils.hpp"
#include "Time.hpp"
#include "VCUtils.hpp"

//----------------------------------------------------------------------------

namespace {

//----------------------------------------------------------------------------

typedef enum 
{
    KEEP_GOING,
    FOUND_WIN
} WorkState;

struct WorkThread
{
    WorkThread(int threadId, HexBoard& brd, HexColor color,
               const bitset_t& consider, 
               WorkState& state, bitset_t& losing, HexPoint& oneMoveWin,
               HexUctInitialData& data, boost::barrier& finished)
        : threadId(threadId), brd(brd), color(color), 
          consider(consider), state(state), losing(losing),
          oneMoveWin(oneMoveWin), data(data), finished(finished)
    {
    }

    void operator()();

    int threadId;
    HexBoard& brd;
    HexColor color;
    bitset_t consider;
    WorkState& state;
    bitset_t& losing;
    HexPoint& oneMoveWin;
    HexUctInitialData& data;
    boost::barrier& finished;
};

void WorkThread::operator()()
{
    hex::log << hex::info << "Thread " << threadId << " started." << hex::endl;

    HexColor other = !color;
    for (BitsetIterator p(consider); p; ++p) 
    {
        // abort if some other thread found a win
        if (state == FOUND_WIN) {
            hex::log << hex::info << threadId 
                     << ": Aborting due to win." << hex::endl;
            break;
        }

        //hex::log << hex::info << threadId << ": " << *p << hex::endl;

        brd.PlayMove(color, *p);

        // found a winning move!
        if (PlayerUtils::IsLostGame(brd, other))
        {
            state = FOUND_WIN;
	    oneMoveWin = *p;
            brd.UndoMove();
            hex::log << hex::info << "Thread " << threadId 
                     << " found win: " << oneMoveWin << hex::endl;
            break;
        }	

        // store the fill-in
        data.ply1_black_stones[*p] = brd.getBlack();
        data.ply1_white_stones[*p] = brd.getWhite();

        // mark losing states and set moves to consider
        if (PlayerUtils::IsWonGame(brd, other))
        {
            losing.set(*p);
            data.ply2_moves_to_consider[*p] 
                = PlayerUtils::MovesToConsiderInLosingState(brd, other);
        } 
        else 
        {
            data.ply2_moves_to_consider[*p] 
                = PlayerUtils::MovesToConsider(brd, other);
        }

        brd.UndoMove();
    }

    hex::log << hex::info << "Thread " << threadId 
             << " finished." << hex::endl;

#if 0
    hex::log << hex::info << "Thread " << threadId << "'s consider set:"
             << brd.printBitset(PlayerUtils::MovesToConsider(brd, color))
             << hex::endl;
#endif

    finished.wait();
}

void SplitBitsetEvenly(const bitset_t& bs, int n, 
                             std::vector<bitset_t>& out)
{
    int c=0;
    for (BitsetIterator p(bs); p; ++p) {
        out[c%n].set(*p);
        c++;
    }
}

void DoThreadedWork(int numthreads, HexBoard& brd, HexColor color,
                    const bitset_t& consider,
                    HexUctInitialData& data,
                    bitset_t& losing,
                    HexPoint& oneMoveWin)
{
    boost::barrier finished(numthreads+1);
    std::vector<boost::thread*> thread(numthreads);
    std::vector<HexUctInitialData> dataSet(numthreads);
    std::vector<bitset_t> losingSet(numthreads);
    std::vector<bitset_t> considerSet(numthreads);
    std::vector<HexBoard*> boardSet(numthreads);
    SplitBitsetEvenly(consider, numthreads, considerSet);

    WorkState state = KEEP_GOING;
    for (int i=0; i<numthreads; ++i) 
    {
        boardSet[i] = new HexBoard(brd);
        thread[i] = new boost::thread
            (WorkThread(i, *boardSet[i], color, considerSet[i],
                        state, losingSet[i], oneMoveWin,
                        dataSet[i], finished));
    }
    finished.wait();

    // union data if we didn't find a win
    // @todo Union it anyway even if we did find a win? 
    if (state != FOUND_WIN) 
    {
        for (int i=0; i<numthreads; ++i) {
            losing |= losingSet[i];
            data.Union(dataSet[i]);
            brd.AddDominationArcs(boardSet[i]->GetBackedUp());
        }
    }

    // join threads and free memory
    for (int i=0; i<numthreads; ++i) {
        thread[i]->join();
        delete thread[i];
        delete boardSet[i];
    }
}

/** Performs the one-ply pre-search. */
void ComputeUctInitialData(int numthreads, bool backupIceInfo, 
                           HexBoard& brd, HexColor color,
                           bitset_t& consider,
                           HexUctInitialData& data,
                           HexPoint& oneMoveWin)
{
    hex::log << hex::info
             << "Told to add these moves to root of UCT tree:" << hex::endl 
             << brd.printBitset(consider) << hex::endl;
    
    // For each 1-ply move that we're told to consider:
    // 1) If the move gives us a win, no need for UCT - just use this move
    // 2) If the move is a loss, note this fact - we'll likely prune it later
    // 3) Compute the 2nd-ply moves to consider so that only reasonable
    //    opponent replies are considered
    // 4) Store the state of the board fill-in to shorten rollouts and 
    //    improve their accuracy

    bitset_t losing;
    data.root_to_play = color;
    data.root_black_stones = brd.getBlack();
    data.root_white_stones = brd.getWhite();

    DoThreadedWork(numthreads, brd, color, consider, data,
                   losing, oneMoveWin);

    // abort out if we found a one-move win
    if (oneMoveWin != INVALID_POINT) {
        return;
    }

    /** Backing up cannot cause this to happen, right? */
    HexAssert(!PlayerUtils::IsDeterminedState(brd, color));

    // Use the backed-up ice info to shrink the moves to consider
    if (backupIceInfo) 
    {
        bitset_t new_consider 
            = PlayerUtils::MovesToConsider(brd, color) & consider;

        if (new_consider.count() < consider.count()) {
            consider = new_consider;       
            hex::log << hex::info
                     << "$$$$$$$$$$$$$$ new moves to consider $$$$$$$$$$$$$$$" 
                     << brd.printBitset(consider)
                     << hex::endl;
        }
    }

    // Subtract any losing moves from the set we consider, unless all of them
    // are losing (in which case UCT search will find which one resists the
    // loss well).
    HexAssert(oneMoveWin == INVALID_POINT);
    if (losing.any()) 
    {
	if (BitsetUtil::IsSubsetOf(consider, losing)) 
        {
	    hex::log << hex::info
                     << "************************************" << hex::endl
                     << " All UCT root children are losing!!" << hex::endl
                     << "************************************" << hex::endl;
	} 
        else 
        {
	    consider = consider - losing;
	    hex::log << hex::info
                     << "Actually added these moves to root of UCT tree:"
		     << hex::endl << brd.printBitset(consider) << hex::endl;
	}
    }

    // Add the appropriate children to the root of the UCT tree
    HexAssert(consider.any());
    data.ply1_moves_to_consider = consider;
}

//----------------------------------------------------------------------------

} // namespace

//----------------------------------------------------------------------------

MoHexPlayer::MoHexPlayer()
    : BenzenePlayer(),
      m_shared_policy(),
      m_search(new HexThreadStateFactory(&m_shared_policy), 
               HexUctUtil::ComputeMaxNumMoves()),
      m_backup_ice_info(true),
      m_max_games(500000),
      m_max_time(9999999)
{
    hex::log << hex::fine << "--- MoHexPlayer" << hex::endl;
}

MoHexPlayer::~MoHexPlayer()
{
}

//----------------------------------------------------------------------------

HexPoint MoHexPlayer::search(HexBoard& brd, 
                             const Game& game_state,
			     HexColor color,
                             const bitset_t& consider,
                             double time_remaining,
                             double& score)
{
    HexPoint lastMove = INVALID_POINT;
    if (!game_state.History().empty()) {
	lastMove = game_state.History().back().point();
	if (lastMove == SWAP_PIECES) {
	  HexAssert(game_state.History().size() == 2);
	  lastMove = game_state.History().front().point();
	}
    }

    HexPoint ret = Search(brd, color, lastMove, consider, 
                          time_remaining, score);

    // scale score to be within [-1, 1] instead of [0, 1]
    score = score*2 - 1.0;

    return ret;
}

//----------------------------------------------------------------------------

HexPoint MoHexPlayer::Search(HexBoard& brd, HexColor color, HexPoint lastMove,
                             const bitset_t& given_to_consider, 
                             double time_remaining, double& score) 
{
    double start = HexGetTime();
    
    HexAssert(HexColorUtil::isBlackWhite(color));
    HexAssert(!brd.isGameOver());

    hex::log << hex::info
             << "--- MoHexPlayer::Search()" << hex::endl
             << "Board:" << brd << hex::endl
             << "Color: " << color << hex::endl
             << "MaxGames: " << m_max_games << hex::endl
             << "NumberThreads: " << m_search.NumberThreads() << hex::endl
             << "MaxNodes: " << m_search.MaxNodes()
             << " (" << sizeof(SgUctNode)*m_search.MaxNodes() << " bytes)" 
             << hex::endl
             << "TimeRemaining: " << time_remaining << hex::endl;

    // Create the initial state data
    HexPoint oneMoveWin = INVALID_POINT;
    HexUctInitialData data;
    data.root_last_move_played = lastMove;
    bitset_t consider = given_to_consider;

    // set clock to use real-time if more than 1-thread
    SgTimer timer;
    timer.Start();
    ComputeUctInitialData(m_search.NumberThreads(), 
                          m_backup_ice_info, brd, color, 
                          consider, data, oneMoveWin);
    m_search.SetInitialData(&data);
    timer.Stop();
    double elapsed = timer.GetTime();
    time_remaining -= elapsed;
    
    hex::log << hex::info << "Time to compute initial data: " 
             << FormattedTime(elapsed) << hex::endl;
    
    // If a winning VC is found after a one ply move, no need to search
    if (oneMoveWin != INVALID_POINT) {
	hex::log << hex::info;
	hex::log << "Winning VC found before UCT search!" << hex::endl;
	hex::log << "Sequence: ";
	hex::log << HexPointUtil::toString(oneMoveWin) << hex::endl;
        score = IMMEDIATE_WIN;
	return oneMoveWin;
    }

    // Abort early if MTC is singleton
    if (consider.count() == 1) {
        hex::log << hex::info << "Only a single move left!" << hex::endl;
        return static_cast<HexPoint>(BitsetUtil::FindSetBit(consider));
    }

    double timelimit = std::min(time_remaining, m_max_time);
    if (timelimit < 0) {
        hex::log << hex::warning << "***** timelimit < 0!! *****";
        timelimit = 30;
    }
    hex::log << "timelimit: " << timelimit << hex::endl;

    brd.ClearPatternCheckStats();
    int old_radius = brd.updateRadius();
    brd.setUpdateRadius(m_search.TreeUpdateRadius());

    std::vector<SgMove> sequence;
    m_search.SetBoard(brd);
    score = m_search.Search(m_max_games, timelimit, sequence);

    brd.setUpdateRadius(old_radius);

    double end = HexGetTime();

    // Output stats
    std::ostringstream os;
    os << '\n';
#if COLLECT_PATTERN_STATISTICS
    os << m_shared_policy.DumpStatistics() << '\n';
    os << brd.DumpPatternCheckStats() << '\n';
#endif
    os << "Elapsed Time   " << FormattedTime(end - start) << '\n';
    m_search.WriteStatistics(os);
    os << "Score          " << score << "\n"
       << "Sequence      ";
    for (int i=0; i<(int)sequence.size(); i++) 
        os << " " << HexUctUtil::MoveString(sequence[i]);
    os << '\n';
    
    hex::log << hex::info << os.str() << hex::endl;

#if 0
    if (m_save_games) {
        std::string filename = "uct-games.sgf";
        uct.SaveGames(filename);
        hex::log << "Games saved in '" << filename << "'." << hex::endl;
    }
#endif

    // Return move recommended by HexUctSearch
    if (sequence.size() > 0) 
        return static_cast<HexPoint>(sequence[0]);

    // It is possible that HexUctSearch only did 1 rollout (probably
    // because it ran out of time to do more); in this case, the move
    // sequence is empty and so we give a warning and return a random
    // move.
    hex::log << hex::warning;
    hex::log << "**** HexUctSearch returned empty sequence!" << hex::endl;
    hex::log << "**** Returning random move!" << hex::endl;
    return BoardUtils::RandomEmptyCell(brd);
}

//----------------------------------------------------------------------------
