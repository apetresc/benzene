//----------------------------------------------------------------------------
// $Id: MoHexPlayer.cpp 1715 2008-10-28 22:13:37Z broderic $
//----------------------------------------------------------------------------

#include "SgSystem.h"

#include "SgTimer.h"

#include "BitsetIterator.hpp"
#include "BoardUtils.hpp"
#include "HexUctUtil.hpp"
#include "HexUctInit.hpp"
#include "HexUctSearch.hpp"
#include "HexUctPolicy.hpp"
#include "MoHexPlayer.hpp"
#include "PlayerUtils.hpp"
#include "Time.hpp"
#include "VCUtils.hpp"

//----------------------------------------------------------------------------

MoHexPlayer::MoHexPlayer()
    : UofAPlayer()
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

HexPoint MoHexPlayer::Search(HexBoard& b, HexColor color, HexPoint lastMove,
                             const bitset_t& given_to_consider, 
                             double time_remaining, double& score) 
{
    double start = HexGetTime();
    
    HexAssert(HexColorUtil::isBlackWhite(color));
    HexAssert(!b.isGameOver());

    int numthreads = hex::settings.get_int("uct-num-threads");
    int maxnodes = hex::settings.get_int("uct-max-nodes");
    int numgames = NumberOfGames(b);

    hex::log << hex::info;
    hex::log << "--- MoHexPlayer::Search()" << hex::endl;
    hex::log << "Board:" << b << hex::endl;
    hex::log << "Color to move: " << color << hex::endl;
    hex::log << "Number of Games: " << numgames << hex::endl;
    hex::log << "Number of Threads: " << numthreads << hex::endl;
    hex::log << "Max Nodes: " << maxnodes
             << " (" << sizeof(SgUctNode)*maxnodes << " bytes)" << hex::endl;
    hex::log << "Time Remaining: " << time_remaining << hex::endl;

    //
    // Create the initial state data
    //
    HexPoint oneMoveWin = INVALID_POINT;
    HexUctInitialData data;
    data.root_last_move_played = lastMove;
    bitset_t consider = given_to_consider;

    // set clock to use real-time if more than 1-thread
    SgTimer timer;
    timer.Start();
    ComputeUctInitialData(numthreads, b, color, consider, data, oneMoveWin);
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

    // check if moves to consider is a singleton
    if (consider.count() == 1) {
        hex::log << hex::info << "Only a single move left!" << hex::endl;
        return static_cast<HexPoint>(BitsetUtil::FindSetBit(consider));
    }
    
    std::string config_dir = hex::settings.get("config-data-path");

    // Create/load shared policy data
    HexUctSharedPolicy shared_policy;
    shared_policy.LoadPatterns(config_dir);

    //
    // Create uct search class
    //
    b.ClearPatternCheckStats();
    HexThreadStateFactory* factory = new HexThreadStateFactory(&shared_policy);
    HexUctSearch uct(b, &data, factory, HexUctUtil::ComputeMaxNumMoves());

    //
    // Set search settings
    //
    uct.SetNumberThreads(numthreads);
    uct.SetMaxNodes(maxnodes);
    
    if (hex::settings.get_bool("uct-enable-init")) {
	uct.SetPriorKnowledge(new HexUctPriorKnowledgeFactory(config_dir));
	uct.SetPriorInit(SG_UCTPRIORINIT_BOTH);
    }
    
    SgUctMoveSelect moveSelect = SG_UCTMOVESELECT_VALUE;
    if (hex::settings.get_bool("uct-use-count"))
        moveSelect = SG_UCTMOVESELECT_COUNT;
    uct.SetMoveSelect(moveSelect);

    uct.SetExpandThreshold(hex::settings.get_int("uct-expand-threshold"));
    uct.SetBiasTermConstant(hex::settings.get_double("uct-bias-term"));

    uct.SetRave(hex::settings.get_bool("uct-enable-rave"));
    uct.SetRaveWeightInitial(hex::settings.get_double("uct-rave-weight-initial"));
    uct.SetRaveWeightFinal(hex::settings.get_double("uct-rave-weight-final"));

    hex::log << hex::info;
    hex::log << "numthreads: " << uct.NumberThreads() << hex::endl;
    hex::log << "bias: " << uct.BiasTermConstant() << hex::endl;
    hex::log << "use count: " 
             << (moveSelect == SG_UCTMOVESELECT_COUNT) << hex::endl;
    hex::log << "expand threshold: " << uct.ExpandThreshold() << hex::endl;
    hex::log << "rave: " << (uct.Rave() ? "enabled" : "disabled") << hex::endl;
    hex::log << "rave initial: " << uct.RaveWeightInitial() << hex::endl;
    hex::log << "rave final: " << uct.RaveWeightFinal() << hex::endl;
    
    int old_radius = b.updateRadius();
    b.setUpdateRadius(hex::settings.get_int("uct-tree-update-radius"));

    hex::log << "update radius: " << b.updateRadius() << hex::endl;

    //
    // Search  
    //
    std::vector<SgMove> sequence;
    std::vector<SgMove> filter;

    SgUctTree* tree = new SgUctTree();
    tree->CreateAllocators(numthreads);
    tree->SetMaxNodes(maxnodes);

    double maxtime = hex::settings.get_int("uct-max-time-per-move");
    double timelimit = std::min(time_remaining, maxtime);
    timelimit -= elapsed;
    if (timelimit < 0) {
        hex::log << hex::warning << "***** timelimit < 0!! *****";
        timelimit = 30;
    }
    hex::log << "timelimit: " << timelimit << hex::endl;

    score = uct.Search(numgames, static_cast<int>(timelimit), 
                       sequence, filter, tree);

    b.setUpdateRadius(old_radius);

    //
    // Output some stats
    //
    hex::log << hex::info;
    hex::log << "Score: " << score << hex::endl;

    hex::log << "Sequence: ";
    int bitShift = HexUctUtil::ComputeMaintainVCBitShift();
    for (int i=0; i<(int)sequence.size(); i++) 
        hex::log << "  " << HexUctUtil::MoveString(sequence[i], bitShift);
    hex::log << hex::endl;

    hex::log << shared_policy.DumpStatistics() << hex::endl;
    hex::log << b.DumpPatternCheckStats() << hex::endl;
    
    double end = HexGetTime();
    hex::log << "Elapsed Time: " << (end - start) << " secs " << hex::endl;

    if (hex::settings.get_bool("uct-save-games")) {
        std::string filename = hex::settings.get("uct-save-games-file");
        uct.SaveGames(filename);
        hex::log << "Games saved in '" << filename << "'." << hex::endl;
    }
    
    // clean-up
    delete tree;
    
    // return move recommended by HexUctSearch
    if (sequence.size() > 0) 
        return static_cast<HexPoint>(sequence[0]);

    // it is possible that HexUctSearch only did 1 rollout
    // (probably because it ran out of time to do more);
    // in this case, the move sequence is empty and so 
    // we give a warning and return a random move.
    hex::log << hex::warning;
    hex::log << "**** HexUctSearch returned empty sequence!" << hex::endl;
    hex::log << "**** Returning random move!" << hex::endl;
    return BoardUtils::RandomEmptyCell(b);
}

//----------------------------------------------------------------------------

namespace 
{

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
  
    brd.ComputeAll(color, HexBoard::DO_NOT_REMOVE_WINNING_FILLIN);
    HexColor other = !color;

    for (BitsetIterator p(consider); p; ++p) {

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

    hex::log << hex::info << "Thread " << threadId << " finished." << hex::endl;

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
    for (int i=0; i<numthreads; ++i) {
        boardSet[i] = new HexBoard(brd.width(), brd.height(), brd.ICE());

        boardSet[i]->startNewGame();
        boardSet[i]->setColor(BLACK, brd.getBlack());
        boardSet[i]->setColor(WHITE, brd.getWhite());
        
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

} // namespace
 
void MoHexPlayer::ComputeUctInitialData(int numthreads, 
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
    // 5) Compute the VCs that first player to move can choose to maintain.


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

    /** @todo IMPLEMENT BACKING UP IN THREADED MODE!!! */
    /** Backing up cannot cause this to happen, right? */
    HexAssert(!PlayerUtils::IsDeterminedState(brd, color));

    // Use the backed-up ice info to shrink the moves to consider
    if (hex::settings.get_bool("uct-backup-ice-info")) {

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

int MoHexPlayer::NumberOfGames(const ConstBoard& brd) const
{
    if (hex::settings.get_bool("uct-scale-num-games-to-size")) {
        int factor = hex::settings.get_int("uct-scale-num-games-per-cell");
        return brd.width()*brd.height()*factor;
    }
    return hex::settings.get_int("uct-num-games");
}

//----------------------------------------------------------------------------
