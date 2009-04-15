//----------------------------------------------------------------------------
// $Id: ICEngine.cpp 1897 2009-02-05 01:43:36Z broderic $
//----------------------------------------------------------------------------

#include "Time.hpp"
#include "BitsetIterator.hpp"
#include "ICEngine.hpp"
#include "BoardUtils.hpp"

#include "boost/filesystem/path.hpp"

//----------------------------------------------------------------------------

namespace {

//----------------------------------------------------------------------------
   
/** Returns set of cells not reachable from either edge. These areas
    are dead, but may not be identified via patterns, etc. Thus we use
    a BFS-type algorithm, checking which areas we can reach from an
    edge without going through the opposite edge or stones of the
    opponent's colour. Note that if the game is already decided, all
    remaining empty cells are dead.
*/
bitset_t ComputeEdgeUnreachableRegions(const StoneBoard& brd, HexColor c,
                                       const bitset_t& stopSet,
                                       bool flowFrom1=true,
                                       bool flowFrom2=true)
{
    bitset_t reachable1, reachable2;
    bitset_t flowSet = (brd.getEmpty() | brd.getColor(c)) & brd.getCells();
    if (flowFrom1) 
    {
        bitset_t flowSet1 = flowSet;
        flowSet1.set(HexPointUtil::colorEdge1(c));
        reachable1 
            = BoardUtils::ReachableOnBitset(brd.Const(), flowSet1, stopSet,
                                            HexPointUtil::colorEdge1(c));
    }
    if (flowFrom2) 
    {
        bitset_t flowSet2 = flowSet;
        flowSet2.set(HexPointUtil::colorEdge2(c));
        reachable2 
            = BoardUtils::ReachableOnBitset(brd.Const(), flowSet2, stopSet,
                                            HexPointUtil::colorEdge2(c));
    }
    return brd.getEmpty() - (reachable1 | reachable2);
}

/** Computes dead regions on the board created by a single group's
    neighbour set. This finds dead regions that cannot be identified
    using only local patterns/properties.
*/
bitset_t ComputeDeadRegions(const GroupBoard& brd)
{
    if (brd.isGameOver())
	return brd.getEmpty();
    
    bitset_t dead;
    for (BoardIterator i(brd.Groups(NOT_EMPTY)); i; ++i) 
    {
        /** @note We believe single stone groups cannot isolate regions by
	    themselves (i.e. they need to be combined with a non-singleton
	    group to create a dead region. This should be proven [Phil]. */
	HexAssert(brd.isCaptain(*i));
	if (brd.GroupMembers(*i).count() == 1)
            continue;
	
	HexColor c = brd.getColor(*i);
	HexAssert(HexColorUtil::isBlackWhite(c));
	
	/** Compute which empty cells are reachable from the edges when we
	    cannot go through this group's empty neighbours (which form a
	    clique). If the clique covers one edge, we only compute
	    reachability from the opposite edge. */
	bitset_t cliqueCutset = brd.Nbs(*i, EMPTY);
	dead |=
	    ComputeEdgeUnreachableRegions(brd, c, cliqueCutset,
					  *i != HexPointUtil::colorEdge1(c),
					  *i != HexPointUtil::colorEdge2(c));
    }
    
    // Areas not reachable due to one or more clique cutsets are dead.
    HexAssert(BitsetUtil::IsSubsetOf(dead, brd.getEmpty()));
    return dead;
}

/** Finds dead regions formed by one group as well as a single cell adjacent
    to two of the group's neighbours (but not the group itself).
*/
bitset_t FindType1Cliques(const GroupBoard& brd)
{
    bitset_t dead;
    bitset_t empty = brd.getEmpty();
    
    // Find two cells that are adjacent through some group, but not directly.
    for (BitsetIterator x(empty); x; ++x) {
	for (BitsetIterator y(empty); *y != *x; ++y) {
	    if (brd.Adjacent(*x, *y)) continue;
	    bitset_t xyNbs = brd.Nbs(*x, NOT_EMPTY) & brd.Nbs(*y, NOT_EMPTY);
	    if (xyNbs.none()) continue;
	    
	    // Find a 3rd cell directly adjacent to the first two, but not
	    // adjacent to some group that connects them.
	    for (BitsetIterator z(empty); z; ++z) {
		if (!brd.Adjacent(*x, *z)) continue;
		if (!brd.Adjacent(*y, *z)) continue;
		HexAssert(*x != *z);
		HexAssert(*y != *z);
		bitset_t xyExclusiveNbs = xyNbs - brd.Nbs(*z, NOT_EMPTY);
		if (xyExclusiveNbs.none()) continue;
		
		// The 3 cells x,y,z form a clique.
		bitset_t clique;
		clique.set(*x);
		clique.set(*y);
		clique.set(*z);
		
		// The specific group(s) common to x and y do not affect the
		// stop set, so we check reachability at most once per color.
		if ((xyExclusiveNbs & brd.getBlack()).any()) {
		    dead |= ComputeEdgeUnreachableRegions(brd, BLACK, clique);
		    HexAssert(BitsetUtil::IsSubsetOf(dead, empty));
		}
		if ((xyExclusiveNbs & brd.getWhite()).any()) {
		    dead |= ComputeEdgeUnreachableRegions(brd, WHITE, clique);
		    HexAssert(BitsetUtil::IsSubsetOf(dead, empty));
		}
	    }
	}
    }
    HexAssert(BitsetUtil::IsSubsetOf(dead, empty));
    return dead;
}

/** Finds dead regions formed by two groups of the same color, using
    common empty neighbours and a direct adjacency between two of
    their exclusive neighbours.
*/
bitset_t FindType2Cliques(const GroupBoard& brd)
{
    bitset_t dead;
    bitset_t empty = brd.getEmpty();
    
    // Find two non-edge groups of the same color with both common
    // empty neighbours in common and also exclusive empty neighbours.
    for (BWIterator c; c; ++c) {
	for (BoardIterator g1(brd.Groups(*c)); g1; ++g1) {
	    if (HexPointUtil::isEdge(*g1)) continue;
	    bitset_t g1_nbs = brd.Nbs(*g1, EMPTY);
	    
	    for (BoardIterator g2(brd.Groups(*c)); *g2 != *g1; ++g2) {
		if (HexPointUtil::isEdge(*g2)) continue;
		bitset_t g2_nbs = brd.Nbs(*g2, EMPTY);
		if ((g1_nbs & g2_nbs).none()) continue;
		
		bitset_t g1Exclusive = g1_nbs - g2_nbs;
		if (g1Exclusive.none()) continue;
		bitset_t g2Exclusive = g2_nbs - g1_nbs;
		if (g2Exclusive.none()) continue;
		
		// Now find two cells exclusive neighbours of these two
		// groups that are directly adjacent to one another.
		for (BitsetIterator x(g1Exclusive); x; ++x) {
		    for (BitsetIterator y(g2Exclusive); y; ++y) {
			if (!brd.Adjacent(*x, *y)) continue;
			
			// Cells x, y and the common neighbours of
			// groups g1, g2 form a clique.
			bitset_t clique = g1_nbs & g2_nbs;
			clique.set(*x);
			clique.set(*y);
			dead |= ComputeEdgeUnreachableRegions(brd, *c, clique);
			HexAssert(BitsetUtil::IsSubsetOf(dead, empty));
		    }
		}
	    }
	}
    }
    HexAssert(BitsetUtil::IsSubsetOf(dead, empty));
    return dead;
}

/** Finds dead regions cutoff by cliques created by 3 groups of the
    same color.
*/
bitset_t FindType3Cliques(const GroupBoard& brd)
{
    bitset_t dead;
    bitset_t empty = brd.getEmpty();
    
    // Find 3 non-edge groups of the same color such that each pair has
    // a non-empty intersection of their empty neighbours.
    for (BWIterator c; c; ++c) {
	for (BoardIterator g1(brd.Groups(*c)); g1; ++g1) {
	    if (HexPointUtil::isEdge(*g1)) continue;
	    bitset_t g1_nbs = brd.Nbs(*g1, EMPTY);
	    
	    for (BoardIterator g2(brd.Groups(*c)); *g2 != *g1; ++g2) {
		if (HexPointUtil::isEdge(*g2)) continue;
		bitset_t g2_nbs = brd.Nbs(*g2, EMPTY);
		if ((g1_nbs & g2_nbs).none()) continue;
		
		for (BoardIterator g3(brd.Groups(*c)); *g3 != *g2; ++g3) {
		    if (HexPointUtil::isEdge(*g3)) continue;
		    bitset_t g3_nbs = brd.Nbs(*g3, EMPTY);
		    if ((g1_nbs & g3_nbs).none()) continue;
		    if ((g2_nbs & g3_nbs).none()) continue;
		    
		    // The union of the pairwise neighbour intersections
		    // of groups g1, g2, g3 form a clique.
		    bitset_t clique;
		    clique = (g1_nbs & g2_nbs) |
			     (g1_nbs & g3_nbs) |
			     (g2_nbs & g3_nbs);
		    dead |= ComputeEdgeUnreachableRegions(brd, *c, clique);
		    HexAssert(BitsetUtil::IsSubsetOf(dead, empty));
		}
	    }
	}
    }
    HexAssert(BitsetUtil::IsSubsetOf(dead, empty));
    return dead;
}

/** Computes dead regions on the board separated via a clique cutset
    composed of the intersection of three known maximal
    cliques. Returns the union of calls to FindType1Cliques(),
    FindType2Cliques(), and FindType3Cliques().
    
    This finds additional regions not identified via local patterns.
*/
bitset_t FindThreeSetCliques(const GroupBoard& brd)
{
    if (brd.isGameOver())
	return brd.getEmpty();

    bitset_t dead1 = FindType1Cliques(brd);
    bitset_t dead2 = FindType2Cliques(brd);
    bitset_t dead3 = FindType3Cliques(brd);

    return dead1 | dead2 | dead3;
}

//----------------------------------------------------------------------------

/** Returns true if the vector of points forms a clique on brd, while
    excluding any checks on exclude (to find pre-simplicial cells
    exclude should be in vn). */
bool IsClique(const ConstBoard& brd, const std::vector<HexPoint>& vn, 
              HexPoint exclude=INVALID_POINT)
{
    for (unsigned a=0; a<vn.size(); ++a) {
        if (vn[a] == exclude) continue;
        for (unsigned b=a+1; b<vn.size(); ++b) {
            if (vn[b] == exclude) continue;
            if (!brd.Adjacent(static_cast<HexPoint>(vn[a]),
                              static_cast<HexPoint>(vn[b])))
                return false;
        }
    }
    return true;
}

/** Finds dead and vulnerable cells using graph theory; ie, not using
    local patterns. The board will have any found dead cells
    filled-in. */
void UseGraphTheoryToFindDeadVulnerable(HexColor color, 
                                        PatternBoard& brd, 
                                        InferiorCells& inf)
{
    bitset_t simplicial;
    bitset_t adj_to_both_edges = 
        brd.Nbs(HexPointUtil::colorEdge1(color), EMPTY) &
        brd.Nbs(HexPointUtil::colorEdge2(color), EMPTY);
    bitset_t consider = brd.getEmpty();
    consider = consider - adj_to_both_edges;
    
    // find presimplicial cells and their dominators
    for (BitsetIterator p(consider); p; ++p) {
	
        std::set<HexPoint> enbs, cnbs;
        bitset_t empty_adj_to_group;
        bool adj_to_edge = false;
	HexPoint edgeNbr = INVALID_POINT;
	
        // Categorize neighbours as either 'empty' or 'color'. 
        for (BoardIterator nb(brd.ConstNbs(*p)); nb; ++nb) {
            HexColor ncolor = brd.getColor(*nb);
            if (ncolor == EMPTY) {
                enbs.insert(*nb);
            } else if (ncolor == color) {
                HexPoint cap = brd.getCaptain(*nb);
                bitset_t adj = brd.Nbs(cap, EMPTY);
                adj.reset(*p);
		
                // Ignore color groups with no empty neighbours (after
                // removing *p).  If color group has one non-*p
                // neighbour, store it as an empty neighbour.
                // Otherwise, add as a color group (helps us to
                // identify cliques later).  Lastly, edges are a
                // special case - always added as a group.
                if (HexPointUtil::isColorEdge(cap, color)) {
		    HexAssert(!adj_to_edge || edgeNbr == cap);
                    adj_to_edge = true;
		    edgeNbr = cap;
                    cnbs.insert(cap);
                    empty_adj_to_group |= adj;
                } else if (adj.count() == 1) {
                    enbs.insert(static_cast<HexPoint>
                                (BitsetUtil::FindSetBit(adj)));
                } else if (adj.count() >= 2) {
                    cnbs.insert(cap);
                    empty_adj_to_group |= adj;
                }
            }
        }
        
        // Remove empty neighbours that are adjacent to a color neighbour.
        std::set<HexPoint>::iterator it;
        for (it = enbs.begin(); it != enbs.end(); ) {
            if (empty_adj_to_group.test(*it)) {
                enbs.erase(it);
                it = enbs.begin();
            } else {
                ++it;
            }
        }
	
        ////////////////////////////////////////////////////////////
        // if adjacent to one empty cell, or a single group of
        // your color, then neighbours are a clique, so *p is dead.
        if (enbs.size() + cnbs.size() <= 1) {
            simplicial.set(*p);
        }
        // Handle cells adjacent to the edge and those adjacent to
        // multiple groups of color (2 or 3). Need to test whether
        // the edge/a group's neighbours include all other groups'
        // neighbours, possibly omitting one. This, along with at most
        // one empty neighbour, makes the cell dead or vulnerable.
        else if (adj_to_edge || cnbs.size() >= 2) {

	    if (enbs.size() >= 2) continue;
	    
	    if (cnbs.size() == 1) {

		HexAssert(adj_to_edge && enbs.size() == 1);
                inf.AddVulnerable(*p, *enbs.begin());

	    } else {

		HexAssert(!adj_to_edge || 
                          HexPointUtil::isColorEdge(edgeNbr, color));

		bitset_t killers_bs;
		bool isPreSimp = false;
		
		// Determine if *p is dead (flag if vulnerable)
		for (std::set<HexPoint>::iterator i = cnbs.begin();
                     i != cnbs.end(); 
                     ++i) 
               {

		    // When adjacent to the edge, only the edge can
		    // trump other groups' adjacencies.
		    if (adj_to_edge && *i != edgeNbr) continue;

		    bitset_t remainingNbs = 
                        empty_adj_to_group - brd.Nbs(*i, EMPTY);
		    
		    if (remainingNbs.count() == 0) {
			if (enbs.size() == 0) {
			    simplicial.set(*p);
			} else {
			    HexAssert(enbs.size() == 1);
			    isPreSimp = true;
			    killers_bs.set(*enbs.begin());
			}
		    } else if (remainingNbs.count() == 1 && enbs.size() == 0) {
			isPreSimp = true;
			killers_bs.set(BitsetUtil::FindSetBit(remainingNbs));
		    }
		}
		
		if (!simplicial.test(*p) && isPreSimp) {
		    HexAssert(killers_bs.any());
                    for (BitsetIterator k(killers_bs); k; ++k) {
                        inf.AddVulnerable(*p, *k);
		    }
		}
	    }

        }
	// If many neighbours and previous cases didn't apply, 
        // then most likely *p is not dead or vulnerable.
	else if (enbs.size() + cnbs.size() >= 4) {
            
            // do nothing

	}
	
	// If adjacent to one group and some empty cells, then *p
	// cannot be dead, but might be vulnerable.
	else if (cnbs.size() == 1) {

	    if (enbs.size() > 1) continue;

	    HexAssert(enbs.size() == 1);
	    HexAssert(empty_adj_to_group.count() >= 2);
	    
	    // The single empty neighbour always kills *p
            inf.AddVulnerable(*p, *enbs.begin());
	    
	    if (empty_adj_to_group.count() == 2) {
		// If the single group has only two neighbours, it is
		// possible that one or both of its neighbours are
		// adjacent to the single direct neighbour, causing us
		// to have more killers of *p
		HexPoint omit = *enbs.begin();
                for (BitsetIterator i(empty_adj_to_group); i; ++i) {
                    enbs.insert(*i);
		}
		
		// determine the additional killers of this vulnerable
		// cell
		std::vector<HexPoint> vn(enbs.begin(), enbs.end());
		for (unsigned ex=0; ex<vn.size(); ++ex) {
		    if (vn[ex] == omit) continue;
		    if (IsClique(brd.Const(), vn, vn[ex])) {
                        inf.AddVulnerable(*p, vn[ex]);
		    }
		}
	    }
	}
        else {

            // If all empty neighbours form a clique, is dead. Otherwise
            // check if eliminating one makes the rest a clique.
            HexAssert(cnbs.size() == 0);
            std::vector<HexPoint> vn(enbs.begin(), enbs.end());

            if (IsClique(brd.Const(), vn)) {

                simplicial.set(*p);

            } else {

                for (unsigned ex=0; ex<vn.size(); ++ex) {
                    if (IsClique(brd.Const(), vn, vn[ex])) {
                        inf.AddVulnerable(*p, vn[ex]);
                    }
                }
            }
        }
    }

    // add the simplicial stones to the board
    inf.AddDead(simplicial);
    brd.addColor(DEAD_COLOR, simplicial);
    brd.update(simplicial);
    brd.absorb(simplicial);
}

//----------------------------------------------------------------------------

}  // anonymous namespace

//----------------------------------------------------------------------------

ICEngine::ICEngine()
    : m_find_presimplicial_pairs(true),
      m_find_permanently_inferior(false),
      m_find_all_pattern_killers(true),
      m_find_all_pattern_dominators(false),
      m_use_handcoded_patterns(true),
      m_backup_opponent_dead(false),
      m_find_three_sided_dead_regions(false),
      m_iterative_dead_regions(false)
{
    LoadHandCodedPatterns();
    LoadPatterns();
}

ICEngine::~ICEngine()
{
}

//----------------------------------------------------------------------------

void ICEngine::LoadHandCodedPatterns()
{
    HandCodedPattern::CreatePatterns(m_hand_coded);
    hex::log << hex::fine << "ICEngine: " << m_hand_coded.size()
             << " hand coded patterns." << hex::endl;
}

void ICEngine::LoadPatterns()
{
#ifdef ABS_TOP_SRCDIR
    m_patterns.LoadPatterns(boost::filesystem::path(ABS_TOP_SRCDIR) 
                            / "share" / "ice-patterns.txt");
#else
    hex::log << hex::warning << "**** NO ICE PATTERNS LOADED ***" << hex::endl;
#endif
}    

//----------------------------------------------------------------------------

int ICEngine::ComputeDeadCaptured(PatternBoard& board, 
                                  InferiorCells& inf, 
                                  HexColorSet colors_to_capture) const
{
    // find dead and captured cells and fill them in. 
    int count = 0;
    while (true) {

        // search for dead; if some are found, fill them in
        // and iterate again.
        while (true) {

            /** @todo This can be optmized quite a bit. */
            bitset_t dead = FindDead(board, board.getEmpty());
            if (dead.none()) break;

            count += dead.count();
            inf.AddDead(dead);
            board.addColor(DEAD_COLOR, dead);
            board.update(dead);
            board.absorb(dead);
        }

        // search for black captured cells; if some are found,
        // fill them in and go back to look for more dead. 
        {
            bitset_t black;
            if (HexColorSetUtil::InSet(BLACK, colors_to_capture)) {
                black = FindCaptured(board, BLACK, board.getEmpty());
            }
            if (black.any()) {
                count += black.count();
                inf.AddCaptured(BLACK, black);
                board.addColor(BLACK, black);
                board.update(black);
                board.absorb(black);
                continue;
            }
        }

        // search for white captured cells; if some are found, fill
        // them in and go back to look for more dead/black captured.
        {
            bitset_t white;
            if (HexColorSetUtil::InSet(WHITE, colors_to_capture)) {
                white = FindCaptured(board, WHITE, board.getEmpty());
            }
            if (white.any()) {
                count += white.count();
                inf.AddCaptured(WHITE, white);
                board.addColor(WHITE, white);
                board.update(white);
                board.absorb(white);
                continue;
            }
        }

        // did not find any fillin, so abort.
        break;
    }

    return count;
}

int ICEngine::FillinPermanentlyInferior(PatternBoard& board, HexColor color, 
                                        InferiorCells& out, 
                                        HexColorSet colors_to_capture) const
{
    if (!m_find_permanently_inferior) 
        return 0;
    if (!HexColorSetUtil::InSet(color, colors_to_capture)) 
        return 0;

    bitset_t carrier;
    bitset_t perm = FindPermanentlyInferior(board, color, board.getEmpty(), 
                                            carrier);
    out.AddPermInf(color, perm, carrier);
    board.addColor(color, perm);
    board.update(perm);
    board.absorb(perm);
    return perm.count();
}

int ICEngine::FillInVulnerable(HexColor color, PatternBoard& board, 
                               InferiorCells& inf, 
                               HexColorSet colors_to_capture) const
{
    int count = 0;
    inf.ClearVulnerable();

    UseGraphTheoryToFindDeadVulnerable(color, board, inf);

    // Find vulnerable cells with local patterns--do not ignore the
    // presimplicial cells previously found because a pattern
    // may encode another dominator.
    bitset_t consider = board.getEmpty() - inf.Dead();
    FindVulnerable(board, color, consider, inf);
    
    // Fill in presimplicial pairs only if we are doing fillin for the
    // other player.
    if (HexColorSetUtil::InSet(!color, colors_to_capture)) 
    {
        bitset_t captured = inf.FindPresimplicialPairs();
        if (captured.any()) {
            inf.AddCaptured(!color, captured);
            board.addColor(!color, captured);
            board.update(captured);
            board.absorb(captured);
        }
        count += captured.count();
    }
    return count;
}

int ICEngine::FillInUnreachable(PatternBoard& board, 
                                InferiorCells& out) const
{
    bitset_t notReachable = ComputeDeadRegions(board);

    if (m_find_three_sided_dead_regions)
        notReachable |= FindThreeSetCliques(board);
    
    if (notReachable.any()) 
    {
        out.AddDead(notReachable);
        board.addColor(DEAD_COLOR, notReachable);
        board.update(notReachable);
        board.absorb(notReachable);
    }
    return notReachable.count();
}

void ICEngine::ComputeFillin(HexColor color, PatternBoard& board, 
                             InferiorCells& out,
                             HexColorSet colors_to_capture) const
{
    out.Clear();
    for (int iteration=0; ; ++iteration) 
    {
        int count=0;
        count += ComputeDeadCaptured(board, out, colors_to_capture);
        count += FillinPermanentlyInferior(board, color, out, 
                                           colors_to_capture);
        count += FillinPermanentlyInferior(board, !color, out, 
                                           colors_to_capture);
        count += FillInVulnerable(!color, board, out, colors_to_capture);
        count += FillInVulnerable(color, board, out, colors_to_capture);

        if (m_iterative_dead_regions)
            count += FillInUnreachable(board, out);

        if (count == 0)
            break;
    }
    
    if (!m_iterative_dead_regions)
        FillInUnreachable(board, out);
}

void ICEngine::ComputeInferiorCells(HexColor color, PatternBoard& board,
                                    InferiorCells& out) const
{
    hash_t h1 = board.Hash();
    double s = HexGetTime();

    ComputeFillin(color, board, out);

    {
        bitset_t consider = board.getEmpty() - out.Vulnerable();
        FindDominated(board, color, consider, out);
    }

    // Play opponent in all empty cells, any dead they created
    // are actually vulnerable to the move played. 
    if (m_backup_opponent_dead) 
    {
        int found = BackupOpponentDead(color, board, out);
        if (found) {
            hex::log << hex::fine << "Found " << found 
                     << " cells vulnerable to opponent moves." << hex::endl;
        }
    }
    
    double e = HexGetTime();
    hash_t h2 = board.Hash();
    HexAssert(h1 == h2);

    hex::log << hex::fine << "  " << (e-s) 
             << "s to find inferior cells." << hex::endl;
}

int ICEngine::BackupOpponentDead(HexColor color, const PatternBoard& board, 
                                 InferiorCells& out) const
{
    PatternBoard brd(board);
    bitset_t dominated = out.Dominated();

    int found = 0;
    for (BitsetIterator p(board.getEmpty()); p; ++p) {
        brd.startNewGame();
        brd.setColor(BLACK, board.getBlack());
        brd.setColor(WHITE, board.getWhite());
        brd.playMove(!color, *p);
        brd.absorb();
        brd.update();

        InferiorCells inf;
        ComputeFillin(color, brd, inf);
        bitset_t filled = inf.Fillin(BLACK) | inf.Fillin(WHITE);

        for (BitsetIterator d(inf.Dead()); d; ++d) {
            /** @todo Add if already vulnerable? */
            if (!out.Vulnerable().test(*d) && !dominated.test(*d)) {
                bitset_t carrier = filled;
                carrier.reset(*d);
                carrier.reset(*p);
                out.AddVulnerable(*d, VulnerableKiller(*p, carrier));
                found++;
            }
        }
    }
    return found;
}

//----------------------------------------------------------------------------

bitset_t ICEngine::FindDead(const PatternBoard& board,
                            const bitset_t& consider) const
{
    return board.matchPatternsOnBoard(consider, m_patterns.HashedDead());
}

bitset_t ICEngine::FindCaptured(const PatternBoard& board, HexColor color, 
                                const bitset_t& consider) const
{
    bitset_t captured;
    for (BitsetIterator p(consider); p; ++p) {
        if (captured.test(*p)) continue;
        
        PatternHits hits;
        board.matchPatternsOnCell(m_patterns.HashedCaptured(color), *p,
                                  PatternBoard::STOP_AT_FIRST_HIT, hits);

        // mark carrier as captured if carrier does not intersect the
        // set of captured cells found in this pass. 
        if (!hits.empty()) {
            HexAssert(hits.size() == 1);
            const std::vector<HexPoint>& moves = hits[0].moves2();

            bitset_t carrier;
            for (unsigned i=0; i<moves.size(); ++i) {
                carrier.set(moves[i]);
            }
            carrier.set(*p);

            if ((carrier & captured).none()) {
                captured |= carrier;
            }
        }
    }
    return captured;
}

bitset_t ICEngine::FindPermanentlyInferior(const PatternBoard& board, 
                                           HexColor color, 
                                           const bitset_t& consider,
                                           bitset_t& carrier) const
{
    std::vector<PatternHits> hits(FIRST_INVALID);
    bitset_t ret 
        = board.matchPatternsOnBoard(consider, 
                                     m_patterns.HashedPermInf(color), 
                                     PatternBoard::STOP_AT_FIRST_HIT, hits);

    for (BitsetIterator p(ret); p; ++p) 
    {
        HexAssert(hits[*p].size()==1);
        const std::vector<HexPoint>& moves = hits[*p][0].moves2();
        for (unsigned i=0; i<moves.size(); ++i) {
            carrier.set(moves[i]);
        }
    }
    
    // @todo Ensure that no dead cells intersect perm.inf. carrier.
    // If cell a is dead and belongs to black's perm.inf. carrier,
    // then make a black captured instead. 
    // ^^^ DO WE STILL WANT THIS? WHAT IS THIS? --broderic
    
    return ret;
}

void ICEngine::FindVulnerable(const PatternBoard& board, HexColor color,
                              const bitset_t& consider,
                              InferiorCells& inf) const
{
    PatternBoard::MatchMode matchmode = PatternBoard::STOP_AT_FIRST_HIT;
    if (m_find_all_pattern_killers)
        matchmode = PatternBoard::MATCH_ALL;

    std::vector<PatternHits> hits(FIRST_INVALID);
    bitset_t vul 
        = board.matchPatternsOnBoard(consider, 
                                     m_patterns.HashedVulnerable(color),
                                     matchmode, hits);

    // add the new vulnerable cells with their killers
    for (BitsetIterator p(vul); p; ++p) 
    {
        for (unsigned j=0; j<hits[*p].size(); ++j) 
        {
            const std::vector<HexPoint>& moves1 = hits[*p][j].moves1();
            HexAssert(moves1.size() == 1);
            HexPoint killer = moves1[0];
            
            bitset_t carrier;
            const std::vector<HexPoint>& moves2 = hits[*p][j].moves2();
            for (unsigned i=0; i<moves2.size(); ++i) {
                carrier.set(moves2[i]);
            }
            inf.AddVulnerable(*p, VulnerableKiller(killer, carrier));
        }
    }
}

void ICEngine::FindDominated(const PatternBoard& board, HexColor color, 
                             const bitset_t& consider,
                             InferiorCells& inf) const
{
    PatternBoard::MatchMode matchmode = PatternBoard::STOP_AT_FIRST_HIT;
    if (m_find_all_pattern_dominators)
        matchmode = PatternBoard::MATCH_ALL;

    // find dominators using patterns
    std::vector<PatternHits> hits(FIRST_INVALID);
    bitset_t dom = board.matchPatternsOnBoard(consider, 
                                             m_patterns.HashedDominated(color),
                                              matchmode, hits);

    // add the new dominated cells with their dominators
    for (BitsetIterator p(dom); p; ++p) {
        for (unsigned j=0; j<hits[*p].size(); ++j) {
            const std::vector<HexPoint>& moves1 = hits[*p][j].moves1();
            HexAssert(moves1.size() == 1);
            inf.AddDominated(*p, moves1[0]);
        }
    }

    // add dominators found via hand coded patterns
    if (m_use_handcoded_patterns) {
        FindHandCodedDominated(board, color, consider, inf);
    }
}

void ICEngine::FindHandCodedDominated(const StoneBoard& board, 
                                      HexColor color,
                                      const bitset_t& consider, 
                                      InferiorCells& inf) const
{
    // If board is rectangular, these hand-coded patterns should not
    // be used because they need to be mirrored (which is not a valid
    // operation on non-square boards).
    if (board.width() != board.height()) return;
    
    for (unsigned i=0; i<m_hand_coded.size(); ++i) {
        CheckHandCodedDominates(board, color, m_hand_coded[i], 
                                consider, inf);
    }
}

void ICEngine::CheckHandCodedDominates(const StoneBoard& brd,
                                       HexColor color,
                                       const HandCodedPattern& pattern, 
                                       const bitset_t& consider, 
                                       InferiorCells& inf) const
{
    if (brd.width() < 4 || brd.height() < 3)
        return;

    HandCodedPattern pat(pattern);

    // mirror and flip colors if checking for white
    if (color == WHITE) {
        pat.mirror(brd.Const());
        pat.flipColors();
    }

    // do top corner
    if (consider.test(pat.dominatee()) && pat.check(brd)) {
        inf.AddDominated(pat.dominatee(), pat.dominator());
    }

    // do bottom corner
    pat.rotate(brd.Const());
    if (consider.test(pat.dominatee()) && pat.check(brd)) {
        inf.AddDominated(pat.dominatee(), pat.dominator());
    }
}

//----------------------------------------------------------------------------

void IceUtil::Update(InferiorCells& out, const InferiorCells& in, 
                     PatternBoard& UNUSED(brd))
{
    // overwrite old vulnerable/dominated with new vulnerable/dominated 
    out.ClearVulnerable();
    out.ClearDominated();
    out.AddVulnerableFrom(in);
    out.AddDominatedFrom(in);

    // add the new fillin to the old fillin
    for (BWIterator c; c; ++c) {
        out.AddCaptured(*c, in.Captured(*c));
        out.AddPermInfFrom(*c, in);
    }

    // Add the new dead cells. 

    // @bug We currently just add the dead, but we need a proper
    // method to dead with stuff going into perm.inf. carriers.

#if 1

    out.AddDead(in.Dead());

#else 

    // @note Any dead cell that is in a perm.inf. carrier needs to be
    // switched to captured. If dead cell is in perm.inf. carriers for
    // both colors, then we do not add it at all.

    bool board_changed = false;
    bitset_t pc[BLACK_AND_WHITE];
    pc[BLACK] = out.PermInfCarrier(BLACK);
    pc[WHITE] = out.PermInfCarrier(WHITE);
    //hex::log << hex::info << brd.printBitset(pc[BLACK]) << hex::endl;
    //hex::log << brd.printBitset(pc[WHITE]) << hex::endl;

    for (BitsetIterator d(in.Dead()); d; ++d) {
        //hex::log << *d << hex::endl;

        if (pc[BLACK].test(*d) && pc[WHITE].test(*d)) {
            brd.setColor(EMPTY, *d);
            board_changed = true;
        } 
        else if (pc[BLACK].test(*d)) {
            out.AddCaptured(BLACK, *d);
            if (brd.getColor(*d) != BLACK) {
                brd.setColor(EMPTY, *d);
                brd.setColor(BLACK, *d);
                board_changed = true;
            }
        } 
        else if (pc[WHITE].test(*d)) {
            out.AddCaptured(WHITE, *d);
            if (brd.getColor(*d) != WHITE) {
                brd.setColor(EMPTY, *d);
                brd.setColor(WHITE, *d);
                board_changed = true;
            }
        } 
        else {
            out.AddDead(*d);
        }
    }

    if (board_changed) {
        brd.absorb();
        brd.update();
    }
#endif
}

//----------------------------------------------------------------------------
