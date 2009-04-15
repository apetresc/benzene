//----------------------------------------------------------------------------
// $Id: PatternBoard.cpp 1426 2008-06-04 21:18:22Z broderic $
//----------------------------------------------------------------------------

#include "BitsetIterator.hpp"
#include "PatternBoard.hpp"

//----------------------------------------------------------------------------

/** Data for each boardsize. */
std::vector<PatternBoard::PatternBoardData> PatternBoard::s_data;

//----------------------------------------------------------------------------

PatternBoard::PatternBoard(int size)
    : GroupBoard(size),
      m_update_radius(Pattern::MAX_EXTENSION)
{
    init();
}


PatternBoard::PatternBoard(int width, int height)
    : GroupBoard(width, height),
      m_update_radius(Pattern::MAX_EXTENSION)
{
    init();
}

PatternBoard::PatternBoard(const StoneBoard& brd)
    : GroupBoard(brd),
      m_update_radius(Pattern::MAX_EXTENSION)
{
    init();
}

void PatternBoard::LoadStaticData()
{
    for (unsigned i=0; i<s_data.size(); ++i) {
        if (width() == s_data[i].width && height() == s_data[i].height) {
            hex::log << "Data exists!" << hex::endl;
            m_index = i;
            return;
        }
    }

    hex::log << "Data does not exist. Creating..." << hex::endl;

    s_data.push_back(PatternBoardData());
    m_index = s_data.size()-1;
    PatternBoardData& data = s_data.back();
    initGodelLookups(data);
}

void PatternBoard::init()
{
    hex::log << hex::fine << "--- PatternBoard" << hex::endl;
    hex::log << "sizeof(PatternBoard) = " << sizeof(PatternBoard) << hex::endl;

    LoadStaticData();
}

PatternBoard::~PatternBoard()
{
}

//----------------------------------------------------------------------------

void PatternBoard::updateRingGodel(HexPoint cell)
{
    HexAssert(isCell(cell));
    HexColor color = getColor(cell);
    HexAssert(HexColorUtil::isBlackWhite(color));

    /** @note if Pattern::NUM_SLICES != 6, this won't work!! 
        This also relies on the fact that slice 3 is opposite 0, 4 opposite 1,
        etc. */
    for (int opp_slice=3, slice=0; slice<Pattern::NUM_SLICES; ++slice) {
        HexPoint p = s_data[m_index].inverse_slice_godel[cell][slice][0];
        m_ring_godel[p].AddColorToSlice(opp_slice, color);
        if (++opp_slice == Pattern::NUM_SLICES) opp_slice = 0;
    }
}

void PatternBoard::update(HexPoint cell)
{
    if (HexPointUtil::isSwap(cell)) 
        return;

    HexAssert(isLocation(cell));

    int r = m_update_radius;
    HexColor color = getColor(cell);
    HexAssert(HexColorUtil::isBlackWhite(color));

    if (HexPointUtil::isEdge(cell)) 
        goto handleEdge;
    
    for (BoardIterator p = const_nbs(cell, r); p; ++p) {
        int slice = s_data[m_index].played_in_slice[*p][cell];
        int godel = s_data[m_index].played_in_godel[*p][cell];
        m_slice_godel[*p][color][slice] |= godel;

        // update *p's ring godel if we played next to it
        if (godel == 1) {
            m_ring_godel[*p].AddColorToSlice(slice, color);
        }
    }
    return;

 handleEdge:

    int edge = cell - FIRST_EDGE;
    for (BoardIterator p = const_nbs(cell, r); p; ++p) {
        for (int slice=0; slice<Pattern::NUM_SLICES; slice++) {
            int godel = s_data[m_index].played_in_edge[*p][edge][slice];
            m_slice_godel[*p][color][slice] |= godel;

            // update *p's ring godel if we played next to it
            if ((godel & 1) == 1) {

                m_ring_godel[*p].AddColorToSlice(slice, color);

            }
        }            
    }
}

void PatternBoard::update(const bitset_t& changed)
{
    for (BitsetIterator p(changed); p; ++p) {
        HexAssert(isOccupied(*p));
        update(*p);
    }
}

void PatternBoard::update()
{
    clearGodels();
    for (BitsetIterator p(getBlack() | getWhite()); p; ++p) {
        update(*p);
    }
}

//----------------------------------------------------------------------------

void PatternBoard::matchPatternsOnCell(const HashedPatternSet& patset, 
                                       HexPoint cell,
                                       MatchMode mode,
                                       PatternHits* hits) const
{
    const RingGodel& ring_godel = m_ring_godel[cell];
    const RotatedPatternList& rlist = patset.ListForGodel(ring_godel);

    RotatedPatternList::const_iterator it = rlist.begin();
    for (; it != rlist.end(); ++it) {
        std::vector<HexPoint> moves1, moves2;
        if (checkRotatedPattern(cell, *it, moves1, moves2)) {
            if (hits != 0) {
                hits->push_back(PatternHit(it->pattern(), moves1, moves2));
            }
            if (mode == STOP_AT_FIRST_HIT)
                break;
        }
    }
}

bitset_t PatternBoard::matchPatternsOnBoard(const bitset_t& consider, 
                                            const HashedPatternSet& patset, 
                                            MatchMode mode, 
                                            PatternHits* hits) const 
{
    bitset_t ret;    
    for (BitsetIterator p(consider & getCells()); p; ++p) {
        matchPatternsOnCell(patset, *p, mode, &hits[*p]);
        if (!hits[*p].empty()) {
            ret.set(*p);
        }
    }
    return ret;
}

bitset_t PatternBoard::matchPatternsOnBoard(const bitset_t& consider, 
                                            const HashedPatternSet& patset, 
                                            HexPoint* pattern_move) const 
{
    bitset_t ret;    
    for (BitsetIterator p(consider & getCells()); p; ++p) {
        PatternHits hits;
        matchPatternsOnCell(patset, *p, STOP_AT_FIRST_HIT, &hits);
        if (!hits.empty()) {
            ret.set(*p);
            if (!hits[0].moves1().empty()) {
                pattern_move[*p] = hits[0].moves1()[0];
            }
        }
    }
    return ret;
}

//-----------------------------------------------------------------------------

int g_addmod6[6][6] =
    { 
        {0, 1, 2, 3, 4, 5 },
        {1, 2, 3, 4, 5, 0 },
        {2, 3, 4, 5, 0, 1 },
        {3, 4, 5, 0, 1, 2 },
        {4, 5, 0, 1, 2, 3 },
        {5, 0, 1, 2, 3, 4 }
    };

int g_submod6[6][6] = 
    {
        {0, 5, 4, 3, 2, 1 },
        {1, 0, 5, 4, 3, 2 },
        {2, 1, 0, 5, 4, 3 },
        {3, 2, 1, 0, 5, 4 },
        {4, 3, 2, 1, 0, 5 },
        {5, 4, 3, 2, 1, 0 }
    };


HexPoint PatternBoard::getRotatedMove(HexPoint cell, 
                                      int slice, int bit, int angle) const
{
    //slice = g_submod6[slice][angle];
    slice = (slice + 6 - angle) % Pattern::NUM_SLICES;
    return s_data[m_index].inverse_slice_godel[cell][slice][bit];
}

bool PatternBoard::checkRotatedSlices(HexPoint cell, 
                                      const RotatedPattern& rotpat) const
{
    return checkRotatedSlices(cell, *rotpat.pattern(), rotpat.angle());
}

bool PatternBoard::checkRotatedSlices(HexPoint cell, 
                                      const Pattern& pattern,
                                      int angle) const
{
    const int *gb = m_slice_godel[cell][BLACK];
    const int *gw = m_slice_godel[cell][WHITE];
    const Pattern::slice_t* pat = pattern.getData();

    bool matches = true;
    for (int i=0; matches && i<Pattern::NUM_SLICES; ++i) {
        m_statistics.slice_checks++;
        
        //int j = g_addmod6[angle][i];
        int j = (angle+i)%Pattern::NUM_SLICES;
     
        int black_b = gb[i] & pat[j][Pattern::FEATURE_CELLS];
        int white_b = gw[i] & pat[j][Pattern::FEATURE_CELLS];
        int empty_b = black_b | white_b;

        int black_p = pat[j][Pattern::FEATURE_BLACK];
        int white_p = pat[j][Pattern::FEATURE_WHITE];
        int empty_p = pat[j][Pattern::FEATURE_CELLS] - 
                      pat[j][Pattern::FEATURE_BLACK] -
                      pat[j][Pattern::FEATURE_WHITE];

        // black cells on the board must be a superset of the
        // black cells in the pattern since the obtuse corner
        // is both black and white. 

        // @todo all cells to be black/white/empty (ie, dead cells),
        // in which case we would use ((empty_b & empty_p) != empty_p)

        if (empty_b & empty_p)
            matches = false;
        else if ((black_b & black_p) != black_p)
            matches = false;
        else if ((white_b & white_p) != white_p)
            matches = false;

    }

    return matches;
}

bool PatternBoard::checkRingGodel(HexPoint cell, 
                                  const RotatedPattern& rotpat) const
{
    return checkRingGodel(cell, *rotpat.pattern(), rotpat.angle());
}

bool PatternBoard::checkRingGodel(HexPoint cell, 
                                  const Pattern& pattern,
                                  int angle) const
{
    m_statistics.ring_checks++;
    return pattern.RingGodel(angle).MatchesGodel(m_ring_godel[cell]);
}

bool PatternBoard::checkRotatedPattern(HexPoint cell, 
                                       const RotatedPattern& rotpat,
                                       std::vector<HexPoint>& moves1,
                                       std::vector<HexPoint>& moves2) const
{
    HexAssert(isCell(cell));

    m_statistics.pattern_checks++;

    bool matches = checkRingGodel(cell, rotpat);

    const Pattern* pattern = rotpat.pattern();
    if (matches && pattern->extension() > 1) {
        matches = checkRotatedSlices(cell, rotpat);
    }

    if (matches) {
        if (pattern->getFlags() & Pattern::HAS_MOVES1) {
            for (unsigned i=0; i<pattern->getMoves1().size(); ++i) {
                int slice = pattern->getMoves1()[i].first;
                int bit = pattern->getMoves1()[i].second;
                moves1.push_back(getRotatedMove(cell, slice, 
                                                  bit, rotpat.angle()));
            }
        }

        if (pattern->getFlags() & Pattern::HAS_MOVES2) {
            for (unsigned i=0; i<pattern->getMoves2().size(); ++i) {
                int slice = pattern->getMoves2()[i].first;
                int bit = pattern->getMoves2()[i].second;
                moves2.push_back(getRotatedMove(cell, slice, 
                                                bit, rotpat.angle()));
            }
        }

        return true;
    }
    return false;
}

//----------------------------------------------------------------------------

void PatternBoard::ClearPatternCheckStats()
{
    m_statistics.pattern_checks = 0;
    m_statistics.ring_checks = 0;
    m_statistics.slice_checks = 0;
}

std::string PatternBoard::DumpPatternCheckStats()
{
    std::ostringstream os;
    
    os << std::endl;
    os << "    Patterns Checked: " << m_statistics.pattern_checks << std::endl;
    os << " Ring Godels Checked: " << m_statistics.ring_checks << std::endl;
    os << "      Slices Checked: " << m_statistics.slice_checks << std::endl;
    os << " Avg Rings Per Check: " 
       << std::setprecision(4) 
       << (double)m_statistics.ring_checks/m_statistics.pattern_checks
       << std::endl;
    os << "Avg Slices Per Check: " 
       << std::setprecision(4) 
       << (double)m_statistics.slice_checks/m_statistics.pattern_checks
       << std::endl;

    return os.str();
}

//----------------------------------------------------------------------
// protected members
//----------------------------------------------------------------------

void PatternBoard::clear()
{    
    GroupBoard::clear();
    clearGodels();
}


//----------------------------------------------------------------------
// private members
//----------------------------------------------------------------------

void PatternBoard::clearGodels()
{
    memset(m_slice_godel, 0, sizeof(m_slice_godel));

    // set all ring godels to empty
    for (BoardIterator p(const_cells()); p; ++p) {
        m_ring_godel[*p].SetEmpty();
    }
}

/** 
 *  Fills the played_in_slice and played_in_godel arrays.
 *
 *  played_in_slice[p1][p2] holds the slice in p1 in which p2 resides.
 *  played_in_godel[p1][p2] is the key to change p1's slice's godel
 *  number by.  This value is always 0 for [p1][p1], and for [p1][z]
 *  where z is out of range of the slice.
 *
 *  Edges are handled as a special case, since an edge appears in more
 *  than a single slice for most cells.  For pattern matching purposes
 *  we assume that each edge is a line of stones of its color.
 *  Patterns requiring stones beyond this edge will not match.
 *  However, empty cells in patterns beyond the edge will still match,
 *  but this should not cause any problems.
 *
 *  The method I use here to fill in the tables is pretty dumb.
 *  There is probably a nicer way, but this works and should be easy
 *  to maintain. 
 *
 */
void PatternBoard::initGodelLookups(PatternBoard::PatternBoardData& data) const
{
    hex::log << hex::fine;
    hex::log << "initGodelLookups (" << width() << " x " << height() <<")"
             << hex::endl;

    data.width = width();
    data.height = height();

    memset(data.played_in_slice, 0, sizeof(data.played_in_slice));
    memset(data.played_in_godel, 0, sizeof(data.played_in_godel));
    memset(data.played_in_edge,  0, sizeof(data.played_in_edge));

    for (BoardIterator ip1=const_cells(); ip1; ++ip1) {
        int x, y;
        HexPoint p1 = *ip1;
        HexPointUtil::pointToCoords(p1, x, y);

        for (int s=0; s<Pattern::NUM_SLICES; s++) {
            int fwd = s;
            int lft = (s + 2) % NUM_DIRECTIONS;
        
            int x1 = x + HexPointUtil::DeltaX(fwd); 
            int y1 = y + HexPointUtil::DeltaY(fwd);

            for (int i=1, g=0; i<=Pattern::MAX_EXTENSION; i++) {
                int x2 = x1;
                int y2 = y1;
                for (int j=0; j<i; j++) {

                    // handle obtuse corner: both colors get it. 
                    if (x2 == -1 && y2 == height()) {

                        // southwest obtuse corner
                        data.played_in_edge[p1][SOUTH - FIRST_EDGE][s] 
                            |= (1 << g);
                        data.played_in_edge[p1][WEST - FIRST_EDGE][s]
                            |= (1 << g);

                    } else if (x2 == width() && y2 == -1) {

                        // northeast obtuse corner
                        data.played_in_edge[p1][NORTH - FIRST_EDGE][s] 
                            |= (1 << g);
                        data.played_in_edge[p1][EAST - FIRST_EDGE][s]
                            |= (1 << g);

                    } else {
                    
                        HexPoint p2 = coordsToPoint(x2, y2);
                        if (p2 != INVALID_POINT) {
                            if (HexPointUtil::isEdge(p2)) {
                                data.played_in_edge[p1][p2 - FIRST_EDGE][s] 
                                    |= (1 << g);
                            } else {
                                data.played_in_slice[p1][p2] = s;
                                data.played_in_godel[p1][p2] = (1 << g);
                            }
                        }
                    }

                    x2 += HexPointUtil::DeltaX(lft);
                    y2 += HexPointUtil::DeltaY(lft);
                    g++;
                }
                x1 += HexPointUtil::DeltaX(fwd);
                y1 += HexPointUtil::DeltaY(fwd);
            }
        }
    }

    // now compute the inverse lookup; that is, center cell, a slice
    // and a godel number, what cell does that slice and godel number
    // correspond to?  Used for finding marked cells in patterns.
    
    int xoffset[Pattern::NUM_SLICES][32];
    int yoffset[Pattern::NUM_SLICES][32];

    for (int s=0; s<Pattern::NUM_SLICES; s++) {
        int fwd = s;
        int lft = (s + 2) % NUM_DIRECTIONS;

        int x1 = HexPointUtil::DeltaX(fwd); 
        int y1 = HexPointUtil::DeltaY(fwd);
        for (int i=1, g=0; i<=Pattern::MAX_EXTENSION; i++) {
            int x2 = x1;
            int y2 = y1;
            for (int j=0; j<i; j++) {
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

    memset(data.inverse_slice_godel, 0, sizeof(data.inverse_slice_godel));
    int d = Pattern::MAX_EXTENSION;
    int e = d*(d+1)/2;  // find last valid godel

    for (BoardIterator i=const_cells(); i; ++i) {
        int x, y;
        HexPoint p = *i;
        HexPointUtil::pointToCoords(p, x, y);

        for (int s=0; s<Pattern::NUM_SLICES; s++) {
            for (int g=0; g<e; g++) {
                int nx = x + xoffset[s][g];
                int ny = y + yoffset[s][g];
		
		HexPoint np = coordsToPoint(nx, ny);
		if (np != INVALID_POINT)
		    data.inverse_slice_godel[p][s][g] = np;
            }
        }
    }    
}

//----------------------------------------------------------------------------
