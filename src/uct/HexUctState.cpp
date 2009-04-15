//----------------------------------------------------------------------------
// $Id: HexUctState.cpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#include "SgSystem.h"

#include "SgException.h"
#include "SgMove.h"

#include "BoardUtils.hpp"
#include "BitsetIterator.hpp"
#include "HexUctPolicy.hpp"
#include "HexUctUtil.hpp"
#include "PatternBoard.hpp"

//----------------------------------------------------------------------------

namespace
{

/** Returns true if board is entirely filled. */
bool GameOver(const StoneBoard& brd)
{
    return brd.getEmpty().none();
}

/** Determines the winner of a filled-in board. */
HexColor GetWinner(const StoneBoard& brd)
{
    HexAssert(GameOver(brd));
    if (BoardUtils::ConnectedOnBitset(brd, brd.getColor(BLACK), NORTH, SOUTH))
        return BLACK;
    return WHITE;
}

/** Determines the winner of a filled-in board, using added adjacencies for
    a particular player c. */
HexColor GetWinner(const StoneBoard& brd,
		   const std::vector<HexPointPair>& addedEdges,
		   HexColor c)
{
    HexAssert(GameOver(brd));
    HexAssert(HexColorUtil::isBlackWhite(c));
    /** Note: we must check whether player c won, since it is possible for
	both players to win (!c on the normal filled board and c on
	this board with added edges). */
    if (BoardUtils::ConnectedOnBitset(brd, brd.getColor(c),
				      HexPointUtil::colorEdge1(c),
				      HexPointUtil::colorEdge2(c),
				      addedEdges))
        return c;
    return !c;
}

} // namespace

//----------------------------------------------------------------------------

HexUctState::AssertionHandler::AssertionHandler(const HexUctState& state)
    : m_state(state)
{
}

void HexUctState::AssertionHandler::Run()
{
    /** @todo Make Logger an std::ostream! */
    m_state.Dump(std::cerr);
}

//----------------------------------------------------------------------------

HexUctState::HexUctState(std::size_t threadId,
			 const SgUctSearch& sch,
                         const PatternBoard& bd,
                         const HexUctInitialData* data)
    : SgUctThreadState(threadId, HexUctUtil::ComputeMaxNumMoves()),
      m_assertionHandler(*this),

      m_bd(bd),
      m_initial_data(data),
      m_search(sch),
      m_policy(0),
      
      m_isInPlayout(false),
      
      m_treeUpdateRadius(hex::settings.get_int("uct-tree-update-radius")),
      m_rolloutUpdateRadius(hex::settings.get_int("uct-rollout-update-radius")),
      
      m_maintainingVCs(hex::settings.get_bool("uct-maintain-vcs")),
      m_maintainVCBitShift(0)
{
}

HexUctState::~HexUctState()
{
    FreePolicy();
}

void HexUctState::FreePolicy()
{
    if (m_policy) 
        delete m_policy;
}
void HexUctState::SetPolicy(HexUctSearchPolicy* policy)
{
    FreePolicy();
    m_policy = policy;
}

void HexUctState::Dump(std::ostream& out) const
{
    out << "HexUctState[" << m_threadId << "] ";
    if (m_isInPlayout) out << "[playout]";
    out << "board:" << m_bd;
}

float HexUctState::Evaluate()
{
    HexAssert(GameOver(m_bd));

    float score;
    if (m_maintainingVCs) {
	score = (GetWinner(m_bd, m_maintainedVCs,
			   m_initial_data->root_to_play) ==
		 m_toPlay) ? 1.0 : 0.0;
    } else {
	score = (GetWinner(m_bd) == m_toPlay) ? 1.0 : 0.0;
    }

    return score;
}

void HexUctState::Execute(SgMove sgmove)
{
    HexPoint move = static_cast<HexPoint>(sgmove);
    
    if (!m_isInPlayout)
	ExecuteTreeMove(move);
    else
	ExecuteRolloutMove(move);
    
    m_toPlay = !m_toPlay;
}

void HexUctState::ExecuteTreeMove(HexPoint move)
{
    if (m_new_game)
    {
	// Play the first-ply cell and add its respective fill-in.
	HexAssert(m_numStonesPlayed == 0);
	HexAssert(m_toPlay == m_initial_data->root_to_play);
	
	if (m_bd.getEmpty() != m_bd.getCells())
	    m_bd.startNewGame();
	HexAssert(m_bd.getEmpty() == m_bd.getCells());
	PointToBitset::const_iterator it;
	it = m_initial_data->ply1_black_stones.find(move);
	HexAssert(it != m_initial_data->ply1_black_stones.end());
        m_bd.addColor(BLACK, it->second);
        it = m_initial_data->ply1_white_stones.find(move);
        HexAssert(it != m_initial_data->ply1_white_stones.end());
        m_bd.addColor(WHITE, it->second);
        m_bd.update();
	
        m_new_game = false;
	m_numStonesPlayed++;
	m_lastMovePlayed = move;
    } 
    else if (move == INVALID_POINT) 
    {
	// Do nothing - this is just to alternate color.
	HexAssert(m_numStonesPlayed == 1);
	HexAssert(m_toPlay == !(m_initial_data->root_to_play));
	HexAssert(m_maintainingVCs);
	HexAssert(!m_opptMaintainedVCs);
	HexAssert(!m_playerMaintainedVCs);
	
	m_opptMaintainedVCs = true;
    } 
    else if (move >= FIRST_INVALID) 
    {
	// Maintain the selected VCs.
	HexAssert(m_numStonesPlayed == 1);
	HexAssert(m_toPlay == m_initial_data->root_to_play);
	HexAssert(m_maintainingVCs);
	HexAssert(!m_playerMaintainedVCs);
	bitset_t moveAsBitset(move);
	HexAssert(moveAsBitset.test(m_maintainVCBitShift));
	HexPoint cell = m_lastMovePlayed;
	
	for (uint i=0; i<m_initial_data->maintainVCs[cell].size(); i++) {
	    if (moveAsBitset.test(i)) {
		VC vc = m_initial_data->maintainVCs[cell][i];
		m_maintainedVCs.push_back(std::make_pair(vc.x(), vc.y()));
		HexAssert(BitsetUtil::IsSubsetOf(vc.carrier(),
						 m_bd.getEmpty()));
		m_bd.addColor(!m_toPlay, vc.carrier());
	    }
	}
	m_bd.update();
	
	m_playerMaintainedVCs = true;
    }
    else
    {
	ExecutePlainMove(move, m_treeUpdateRadius);
    }
}

void HexUctState::ExecuteRolloutMove(HexPoint move)
{
    ExecutePlainMove(move, m_rolloutUpdateRadius);
}

void HexUctState::ExecutePlainMove(HexPoint cell, int updateRadius)
{
    // Simply play a stone on the given cell.
    HexAssert(m_bd.isEmpty(cell));
    HexAssert(m_bd.updateRadius() == updateRadius);
    
    m_bd.playMove(m_toPlay, cell);
    if (updateRadius == 1)
	m_bd.updateRingGodel(cell);
    else
	m_bd.update(cell);
    
    m_numStonesPlayed++;
    m_lastMovePlayed = cell;
}


void HexUctState::GenerateAllMoves(std::vector<SgMove>& moves)
{
    // Some experiments with MoGo like playout heuristics vs. GNu Go Level 1
    // show that randomization in the UCT tree has no positive effect, so we
    // don't do any costly shuffling of the moves
    moves.clear();
    
    HexAssert(m_new_game == (m_numStonesPlayed == 0));
    if (m_new_game)
    {
	for (BitsetIterator it(m_initial_data->ply1_moves_to_consider); it; ++it)
	    moves.push_back(*it);
    }
    else if (m_numStonesPlayed == 1) 
    {
	if (m_maintainingVCs && !m_opptMaintainedVCs) 
	{
	    // To maintain color-alternating turns only.
	    moves.push_back(INVALID_POINT);
	}
	else if (m_maintainingVCs && !m_playerMaintainedVCs) 
	{
	    // Maintain any valid subset of VCs for maintenance, given the
	    // first move played.
	    HexPoint cell = m_lastMovePlayed;
	    
	    // Compute number of possible subsets = 2^(# VCs can maintain).
	    int numChoices = (int) m_initial_data->maintainVCs[cell].size();
	    int numCombinations = (1 << numChoices);
	    
	    for (int i=0; i<numCombinations; ++i) {
		HexAssert(0 == (i & (1 << numChoices)));
		
		// Skip this move if two or more maintained VCs overlap.
		bool disjointVCs = true;
		bitset_t carriers;
		for (int j=0; j<numChoices; ++j) {
		    if (i & (1 << j)) {
			VC vc = m_initial_data->maintainVCs[cell][j];
			if ((vc.carrier() & carriers).any()) {
			    disjointVCs = false;
			    break;
			}
			carriers |= vc.carrier();
		    }
		}
		if (!disjointVCs) continue;
		
		// Add this VC-maintenance move to the list.
		int maintainMove = i;
		HexAssert(maintainMove < (1 << m_maintainVCBitShift));
		maintainMove += (1 << m_maintainVCBitShift);
		moves.push_back((SgMove) maintainMove);
	    }
	} 
	else 
	{
	    // We're about to play the second stone...
	    bitset_t opptMustplay;
	    if (m_maintainedVCs.empty()) 
	    {
		PointToBitset::const_iterator it 
		    = m_initial_data->ply2_moves_to_consider.find(m_lastMovePlayed);
		HexAssert(it != m_initial_data->ply2_moves_to_consider.end());
		opptMustplay = it->second;
	    } 
	    else 
	    {
		// Cannot use mustplay when maintain VCs.
		opptMustplay = m_bd.getEmpty();
	    }
	    
	    for (BitsetIterator it(opptMustplay); it; ++it) 
		moves.push_back(*it);
	}
    } 
    else 
    {
	for (BitsetIterator it(m_bd.getEmpty()); it; ++it)
	    moves.push_back(*it);
    }

    // @todo Add swap?
    //moves.push_back(SG_PASS);
}

SgMove HexUctState::GenerateRandomMove(bool& skipRaveUpdate)
{
    skipRaveUpdate = false;
    
    if (GameOver(m_bd)) {
        return SG_NULLMOVE;
    }
    
    SgPoint move = m_policy->GenerateMove(m_bd, m_toPlay, m_lastMovePlayed);
    HexAssert(move != SG_NULLMOVE);
    return move;
}

void HexUctState::StartSearch()
{
    if (m_maintainingVCs) {
	/** Compute the shift required to move a bit in location 0 until
	    it reaches a power of 2 >= FIRST_INVALID. Then we set
	    m_maintainVCBitShift to one greater than this value. */
	HexAssert(m_maintainVCBitShift == 0);
	m_maintainVCBitShift = HexUctUtil::ComputeMaintainVCBitShift();
	HexAssert(m_maintainVCBitShift > 0);
    }
}

void HexUctState::TakeBackInTree(std::size_t nuMoves)
{
    SG_UNUSED(nuMoves);
}

void HexUctState::TakeBackPlayout(std::size_t nuMoves)
{
    SG_UNUSED(nuMoves);
}

SgBlackWhite HexUctState::ToPlay() const
{
    return HexUctUtil::ToSgBlackWhite(m_toPlay);
}

void HexUctState::GameStart()
{
    m_new_game = true;
    m_isInPlayout = false;
    m_numStonesPlayed = 0;
    m_toPlay = m_initial_data->root_to_play;
    m_lastMovePlayed = m_initial_data->root_last_move_played;
    m_bd.setUpdateRadius(m_treeUpdateRadius);
    m_bd.startNewGame();
    
    if (!m_search.Tree().Root().HasChildren()) {
	// If tree only consists of root node, then need to add root
	// position's stones to board. Otherwise we use pre-computed
	// 1-ply filled-in positions.
	HexAssert(m_bd.getEmpty() == m_bd.getCells());
	m_bd.addColor(BLACK, m_initial_data->root_black_stones);
	m_bd.addColor(WHITE, m_initial_data->root_white_stones);
        m_bd.update();
    }
    
    if (m_maintainingVCs) {
	m_opptMaintainedVCs = false;
	m_playerMaintainedVCs = false;
	m_maintainedVCs.clear();
    }
}

void HexUctState::StartPlayouts()
{
    m_isInPlayout = true;
    m_bd.setUpdateRadius(m_rolloutUpdateRadius);
    
    /** Rollout radius should normally be no bigger than tree radius, but if
	it is, we need to do an extra update for each rollout during the
	transition from the tree phase to the rollout phase. */
    if (m_rolloutUpdateRadius > m_treeUpdateRadius)
	m_bd.update();
}

void HexUctState::StartPlayout()
{
    m_policy->InitializeForRollout(m_bd);
}

void HexUctState::EndPlayout()
{
}

//----------------------------------------------------------------------------
