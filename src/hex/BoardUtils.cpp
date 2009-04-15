//----------------------------------------------------------------------------
// $Id: BoardUtils.cpp 1699 2008-09-24 23:55:56Z broderic $
//----------------------------------------------------------------------------

/** @file

*/

#include "SgSystem.h"
#include "SgRandom.h"

#include "BoardUtils.hpp"
#include "BitsetIterator.hpp"
#include "GraphUtils.hpp"
#include "Pattern.hpp"
#include "HashedPatternSet.hpp"

//----------------------------------------------------------------------------

/** Local helper functions. */
namespace {

bool g_BoardUtilsInitialized = false;
std::vector<Pattern> g_oppmiai[BLACK_AND_WHITE];
HashedPatternSet* g_hash_oppmiai[BLACK_AND_WHITE];

/** Initialize the miai pattern. */
void InitializeOppMiai()
{
    if (g_BoardUtilsInitialized) return;

    hex::log << hex::fine << "--InitializeOppMiai" << hex::endl;

    //
    // Miai between groups of opposite color.
    // W is marked; so if you use this pattern on the black members of 
    // group, it will tell you the white groups that are adjacent to it. 
    // Used in the decomposition stuff. 
    //
    //               . W  
    //              * .                        [oppmiai/0]
    // m:5,0,4,4,0;1,0,0,0,0;0,0,0,0,0;0,0,0,0,0;0,0,0,0,0;0,0,0,0,0;1
    std::string oppmiai = "m:5,0,4,4,0;1,0,0,0,0;0,0,0,0,0;0,0,0,0,0;0,0,0,0,0;0,0,0,0,0;1";

    Pattern pattern;
    if (!pattern.unserialize(oppmiai)) {
        HexAssert(false);
    }
    pattern.setName("oppmiai");

    g_oppmiai[BLACK].push_back(pattern);
    pattern.flipColors();
    g_oppmiai[WHITE].push_back(pattern);
        
    for (BWIterator c; c; ++c) {
        g_hash_oppmiai[*c] = new HashedPatternSet();
        g_hash_oppmiai[*c]->hash(g_oppmiai[*c]);
    }
}

/** @todo Is it possible to speed this up? */
void ComputeAdjacentByMiai(const HexBoard& brd, PointToBitset& adjByMiai)
{
    HexAssert(g_BoardUtilsInitialized);

    adjByMiai.clear();
    for (BWIterator color; color; ++color) {
        for (BitsetIterator p(brd.getColor(*color) & brd.getCells()); p; ++p) {

            PatternHits hits;
            brd.matchPatternsOnCell(*g_hash_oppmiai[*color], *p, 
                                    PatternBoard::MATCH_ALL, &hits);
            
            HexPoint cp = brd.getCaptain(*p);
            for (unsigned j=0; j<hits.size(); ++j) {
                HexPoint cj = brd.getCaptain(hits[j].moves1()[0]);
                adjByMiai[cj].set(cp);
                adjByMiai[cp].set(cj);
            }
        }
    }
}
 
} // namespace

//----------------------------------------------------------------------------

void BoardUtils::Initialize()
{
    InitializeOppMiai();
    g_BoardUtilsInitialized = true;
}

//----------------------------------------------------------------------------

HexPoint BoardUtils::RandomEmptyCell(const StoneBoard& brd)
{
    bitset_t moves = brd.getEmpty() & brd.getCells();
    int count = moves.count();
    if (count == 0) 
        return INVALID_POINT;
    
    int randMove = SgRandom::Global().Int(count) + 1;
    for (BitsetIterator p(moves); p; ++p) 
        if (--randMove==0) return *p;

    HexAssert(false);
    return INVALID_POINT;
}

//----------------------------------------------------------------------------

bool BoardUtils::ConnectedOnBitset(const ConstBoard& brd, 
                                   const bitset_t& carrier,
                                   HexPoint p1, HexPoint p2)
{
    HexAssert(carrier.test(p1));
    HexAssert(carrier.test(p2));
    bitset_t seen = ReachableOnBitset(brd, carrier, EMPTY_BITSET, p1);
    return seen.test(p2);
}

bool BoardUtils::ConnectedOnBitset(const ConstBoard& brd, 
                                   const bitset_t& carrier,
                                   HexPoint p1, HexPoint p2,
				   const std::vector<HexPointPair>& addedEdges)
{
    HexAssert(carrier.test(p1));
    HexAssert(carrier.test(p2));
    bitset_t seen = ReachableOnBitset(brd, carrier, EMPTY_BITSET, 
                                      p1, addedEdges);
    return seen.test(p2);
}

bitset_t BoardUtils::ReachableOnBitset(const ConstBoard& brd, 
                                       const bitset_t& carrier,
                                       const bitset_t& stopset,
                                       HexPoint start)
{
    HexAssert(carrier.test(start));

    bitset_t seen;
    std::queue<HexPoint> q;
    q.push(start);
    seen.set(start);

    while (!q.empty()) {
        HexPoint p = q.front();
        q.pop();
        if (stopset.test(p)) continue;
        for (BoardIterator nb(brd.const_nbs(p)); nb; ++nb) {
            if (carrier.test(*nb) && !seen.test(*nb)) {
                q.push(*nb);
                seen.set(*nb);
            }
        }
    }

    return seen;
}

bitset_t BoardUtils::ReachableOnBitset(const ConstBoard& brd, 
                                       const bitset_t& carrier,
                                       const bitset_t& stopset,
                                       HexPoint start,
			       const std::vector<HexPointPair>& addedEdges)
{
    HexAssert(carrier.test(start));
    
    bitset_t seen;
    std::queue<HexPoint> q;
    q.push(start);
    seen.set(start);
    std::vector<HexPointPair>::const_iterator i;
    
    while (!q.empty()) {
        HexPoint p = q.front();
        q.pop();
        if (stopset.test(p)) continue;
	// Using normal board edges
        for (BoardIterator nb(brd.const_nbs(p)); nb; ++nb) {
            if (carrier.test(*nb) && !seen.test(*nb)) {
                q.push(*nb);
                seen.set(*nb);
            }
        }
	// Using added edges
	for (i = addedEdges.begin(); i != addedEdges.end(); ++i) {
	    if ((*i).first == p || (*i).second == p) {
		HexAssert(carrier.test((*i).first));
		HexAssert(carrier.test((*i).second));
		
		if (!seen.test((*i).first)) {
		    HexAssert(seen.test((*i).second));
		    q.push((*i).first);
		    seen.set((*i).first);
		} else if (!seen.test((*i).second)) {
		    q.push((*i).second);
		    seen.set((*i).second);
		}
	    }
	}
    }
    
    return seen;
}

//----------------------------------------------------------------------------

bool BoardUtils::FindCombinatorialDecomposition(const HexBoard& brd,
						HexColor color,
						bitset_t& captured)
{
    // If game is over or decided, don't do any work.
    HexPoint edge1 = HexPointUtil::colorEdge1(color);
    HexPoint edge2 = HexPointUtil::colorEdge2(color);
    const Connections& cons = brd.Cons(color);
    if (brd.isGameOver() || cons.doesVCExist(edge1, edge2, VC::FULL)) {
	captured.reset();
	return false;
    }
    
    /** Compute neighbouring groups of opposite color.

        @note Assumes that edges that touch are adjacent. See
        ConstBoard for more details. 
    */
    PointToBitset adjTo;
    PointToBitset adjByMiai;
    ComputeAdjacentByMiai(brd, adjByMiai);
    for (BoardIterator g(brd.Groups(color)); g; ++g) {
	bitset_t opptNbs = adjByMiai[*g] | brd.Nbs(*g, !color);
	if (opptNbs.count() >= 2) {
	    adjTo[*g] = opptNbs;
	}
    }
    // The two color edges are in the list. If no other groups are, then quit.
    HexAssert(adjTo.size() >= 2);
    if (adjTo.size() == 2) {
	captured.reset();
	return false;
    }
    
    // Compute graph representing board from color's perspective.
    PointToBitset graphNbs;
    brd.ComputeDigraph(color, graphNbs);
    
    // Find (ordered) pairs of color groups that are VC-connected and have at
    // least two adjacent opponent groups in common.
    PointToBitset::iterator g1, g2;
    for (g1 = adjTo.begin(); g1 != adjTo.end(); ++g1) {
	for (g2 = adjTo.begin(); g2 != g1; ++g2) {
	    if ((g1->second & g2->second).count() < 2) continue;
	    if (!cons.doesVCExist(g1->first, g2->first, VC::FULL)) continue;
	    
	    // This is such a pair, so at least one of the two is not an edge.
	    // Find which color edges are not equal to either of these groups.
	    HexAssert(!HexPointUtil::isEdge(g1->first) ||
		      !HexPointUtil::isEdge(g2->first));
	    bool edge1Free = (g1->first != edge1 && g2->first != edge1);
	    bool edge2Free = (g1->first != edge2 && g2->first != edge2);
	    
	    // Find the set of empty cells bounded by these two groups.
	    bitset_t stopSet = graphNbs[g1->first] | graphNbs[g2->first];
	    bitset_t decompArea;
	    decompArea.reset();
	    if (edge1Free)
		decompArea |= GraphUtils::BFS(edge1, graphNbs, stopSet);
	    if (edge2Free)
		decompArea |= GraphUtils::BFS(edge2, graphNbs, stopSet);
	    decompArea.flip();
	    decompArea &= brd.getEmpty();
	    
	    // If the pair has a VC confined to these cells, then we have
	    // a decomposition - return it.
	    const VCList& vl = cons.getVCList(g1->first, g2->first, VC::FULL);
	    for (VCList::const_iterator vi=vl.begin(); vi!=vl.end(); ++vi) {
		if (BitsetUtil::IsSubsetOf(vi->carrier(), decompArea)) {
		    captured = vi->carrier();
		    return true;
		}
	    }
	}
    }
    
    // No combinatorial decomposition with a VC was found.
    captured.reset();
    return false;
}

bool BoardUtils::FindSplittingDecomposition(const HexBoard& brd,
					    HexColor color,
					    HexPoint& group,
					    bitset_t& captured)
{
    // compute nbrs of color edges
    PointToBitset adjByMiai;
    ComputeAdjacentByMiai(brd, adjByMiai);
    HexPoint edge1 = HexPointUtil::colorEdge1(!color);
    HexPoint edge2 = HexPointUtil::colorEdge2(!color);
    bitset_t adjto1 = adjByMiai[edge1] | brd.Nbs(edge1, color);
    bitset_t adjto2 = adjByMiai[edge2] | brd.Nbs(edge2, color);

    // @note must & with getCells() because we want non-edge groups;
    // this assumes that edges are always captains. 
    bitset_t adjToBothEdges = adjto1 & adjto2 & brd.getCells();

    // if there is a group adjacent to both opponent edges, return it.
    if (adjToBothEdges.any()) {
	captured.reset();
	group = static_cast<HexPoint>
            (BitsetUtil::FirstSetBit(adjToBothEdges));
	return true;
    }

    return false;
}

//----------------------------------------------------------------------------

std::string BoardUtils::GuiDumpOutsideConsiderSet(const StoneBoard& brd, 
                                                  const bitset_t& consider,
                                                  const bitset_t& remove)
{
    std::ostringstream os;
    bitset_t outside = brd.getEmpty() - (remove | consider);
    for (BitsetIterator p(outside); p; ++p) 
        os << " " << *p << " x";
    return os.str();
}

//----------------------------------------------------------------------------
