//----------------------------------------------------------------------------
// $Id: ConstBoard.cpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#include "ConstBoard.hpp"
#include "BitsetIterator.hpp"

//----------------------------------------------------------------------------

/** Data for each board dimension encountered. */
std::vector<ConstBoard::StaticData> ConstBoard::s_data;

//----------------------------------------------------------------------------

ConstBoard::ConstBoard(int size) 
    : m_width(size),
      m_height(size)
{
    HexAssert(1 <= m_width && m_width <= MAX_WIDTH);
    HexAssert(1 <= m_height && m_height <= MAX_HEIGHT);
    init();
}

ConstBoard::ConstBoard(int width, int height)
    : m_width(width),
      m_height(height)
{
    HexAssert(1 <= m_width && m_width <= MAX_WIDTH);
    HexAssert(1 <= m_height && m_height <= MAX_HEIGHT);
    init();
}

ConstBoard::~ConstBoard()
{
}

//----------------------------------------------------------------------

bitset_t ConstBoard::PackBitset(const bitset_t& in) const
{
    int j=0;
    bitset_t ret;
    for (BoardIterator it(const_cells()); it; ++it, ++j) {
        if (in.test(*it)) 
            ret.set(j);
    }
    return ret;
}

bitset_t ConstBoard::UnpackBitset(const bitset_t& in) const
{
    int j=0;
    bitset_t ret;
    for (BoardIterator it(const_cells()); it; ++it, ++j) {
        if (in.test(j))
            ret.set(*it);
    }
    return ret;
}

//----------------------------------------------------------------------

int ConstBoard::distance(HexPoint x, HexPoint y) const
{
    HexAssert(isValid(x));
    HexAssert(isValid(y));
    
    if (HexPointUtil::isEdge(y)) 
        return distanceToEdge(x, y);
    else if (HexPointUtil::isEdge(x)) 
        return distanceToEdge(y, x);
    
    int r1, c1, r2, c2;
    HexPointUtil::pointToCoords(x, r1, c1);
    HexPointUtil::pointToCoords(y, r2, c2);
    int dr = r1 - r2;
    int dc = c1 - c2;

    return (dr*dc >= 0) 
        ? std::abs(dr) + std::abs(dc) 
        : std::max(std::abs(dr), std::abs(dc));
}

HexPoint ConstBoard::rotate(HexPoint p) const
{
    HexAssert(isValid(p));
    
    if (!isLocation(p)) return p;
    if (HexPointUtil::isEdge(p)) return HexPointUtil::oppositeEdge(p);
    
    int x, y;
    HexPointUtil::pointToCoords(p, x, y);
    return HexPointUtil::coordsToPoint(width()-1-x, height()-1-y);
}

bitset_t ConstBoard::rotate(const bitset_t& bs) const
{
    bitset_t ret;
    for (BitsetIterator it(bs); it; ++it) {
        ret.set(rotate(*it));
    }
    return ret;
}

HexPoint ConstBoard::mirror(HexPoint p) const
{
    HexAssert(isValid(p));
    HexAssert(width() == height());
    
    if (!isLocation(p)) return p;
    
    if (HexPointUtil::isEdge(p)) {
	if (HexPointUtil::isColorEdge(p, VERTICAL_COLOR))
	    return HexPointUtil::rightEdge(p);
	else
	    return HexPointUtil::leftEdge(p);
    }
    
    int x, y;
    HexPointUtil::pointToCoords(p, x, y);
    return HexPointUtil::coordsToPoint(y, x);
}

bitset_t ConstBoard::mirror(const bitset_t& bs) const
{
    bitset_t ret;
    for (BitsetIterator it(bs); it; ++it) {
        ret.set(mirror(*it));
    }
    return ret;
}

HexPoint ConstBoard::centerPoint() const
{
    HexAssert((width() & 1) && (height() & 1));
    return centerPointRight();
}

HexPoint ConstBoard::centerPointRight() const
{
    int x = width() / 2;
    int y = height() / 2;

    if (!(width() & 1) && !(height() & 1)) y--;

    return HexPointUtil::coordsToPoint(x, y);
}

HexPoint ConstBoard::centerPointLeft() const
{
    int x = width() / 2;
    int y = height() / 2;

    if (!(width() & 1)) x--;
    if ((width() & 1) && !(height() & 1)) y--;

    return HexPointUtil::coordsToPoint(x, y);
}

bool ConstBoard::isAdjacent(HexPoint p1, HexPoint p2) const
{
    for (BoardIterator it = const_nbs(p1); it; ++it) {
        if (*it == p2) return true;
    }
    return false;
}

int ConstBoard::distanceToEdge(HexPoint from, HexPoint edge) const
{
    HexAssert(HexPointUtil::isEdge(edge));
    
    if (HexPointUtil::isEdge(from)) {
	if (from == edge)                             return 0;
	if (HexPointUtil::oppositeEdge(from) != edge) return 1;
	if (edge == NORTH || edge == SOUTH)           return height();
	return width();
    }
    
    int r, c;
    HexPointUtil::pointToCoords(from, c, r);
    switch(edge) {
    case NORTH: return r+1;
    case SOUTH: return height()-r;
    case EAST:  return width()-c;
    default:    return c+1;
    }
}

HexPoint ConstBoard::coordsToPoint(int x, int y) const
{
    if (x <= -2 || x > width())      return INVALID_POINT;
    if (y <= -2 || y > height())     return INVALID_POINT;
    if ((x == -1 || x == width()) &&
	(y == -1 || y == height()))  return INVALID_POINT;

    if (y == -1)       return NORTH;
    if (y == height()) return SOUTH;
    if (x == -1)       return WEST;
    if (x == width())  return EAST;

    return HexPointUtil::coordsToPoint(x, y);
}

HexPoint ConstBoard::PointInDir(HexPoint point, HexDirection dir) const
{
    if (HexPointUtil::isEdge(point))
        return point;

    int x, y;
    HexAssert(HexPointUtil::isInteriorCell(point));
    HexPointUtil::pointToCoords(point, x, y);
    x += HexPointUtil::DeltaX(dir);
    y += HexPointUtil::DeltaY(dir);
    return coordsToPoint(x, y);
}

bool ConstBoard::ShiftBitset(const bitset_t& bs, HexDirection dir,
                             bitset_t& out) const
{
    out.reset();
    bool still_inside = true;
    for (BitsetIterator p(bs); p; ++p) {
        HexPoint s = PointInDir(*p, dir);
        if (!HexPointUtil::isEdge(*p) && HexPointUtil::isEdge(s))
            still_inside = false;
        out.set(s);
    }
    return still_inside;
}

//----------------------------------------------------------------------
// Construction stuff
//----------------------------------------------------------------------

int ConstBoard::init()
{
    hex::log << hex::fine << "--- ConstBoard"
             << " (" << width() << " x " << height() << ")" << hex::endl;
    hex::log << "sizeof(ConstBoard) = " << sizeof(ConstBoard) << hex::endl;

    // see if it's already computed
    for (unsigned i=0; i<s_data.size(); ++i) {
        if (s_data[i].width == width() && s_data[i].height == height()) {
            hex::log << "Data exists!" << hex::endl;
            m_index = i;
            return i;
        }
    }

    hex::log << "Data does not exist! Creating..." << hex::endl;

    // compute data for this new boardsize
    s_data.push_back(ConstBoard::StaticData());
    ConstBoard::StaticData& data = s_data.back();
    m_index = s_data.size()-1;

    data.width = width();
    data.height = height();

    computePointList(data);
    createIterators(data);
    computeValid(data);
    computeNeighbours(data);

    return s_data.size()-1;
}

void ConstBoard::computePointList(ConstBoard::StaticData& data)
{
    hex::log << hex::fine << "computePointList" << hex::endl;
    data.points.clear();

    /** @note There are several pieces of code that rely on the fact
        points are visited in the order (a1,b1,...,a2,b2,...,etc)
        (StoneBoard::GetBoardID() is one).  Do not change this order
        unless you know what you are doing!!.
    */
    for (int p = FIRST_SPECIAL; p < FIRST_CELL; ++p) 
        data.points.push_back(static_cast<HexPoint>(p));

    for (int y=0; y<height(); y++) {
        for (int x=0; x<width(); x++) {
            data.points.push_back(HexPointUtil::coordsToPoint(x, y));
        }
    }
    hex::log << hex::endl;

    data.points.push_back(INVALID_POINT);
}

void ConstBoard::createIterators(ConstBoard::StaticData& data)
{
    hex::log << hex::fine << "createIterators" << hex::endl;
    int p = 0;
    while (data.points[p] != FIRST_SPECIAL) p++;
    data.all_index = p;

    while (data.points[p] != FIRST_EDGE) p++;
    data.locations_index = p;

    while (data.points[p] != FIRST_CELL) p++;
    data.cells_index = p;
}

void ConstBoard::computeValid(ConstBoard::StaticData& data)
{
    hex::log << hex::fine << "computeValid" << hex::endl;

    data.valid.reset();
    for (BoardIterator i(const_valid()); i; ++i) {
        data.valid.set(*i);
    }

    data.locations.reset();
    for (BoardIterator i(const_locations()); i; ++i) {
        data.locations.set(*i);
    }

    data.cells.reset();
    for (BoardIterator i(const_cells()); i; ++i) {
        data.cells.set(*i);
    }
}

void ConstBoard::computeNeighbours(ConstBoard::StaticData& data)
{
    hex::log << hex::fine << "computeNeighbours" << hex::endl;

    // clear all info
    for (BoardIterator i(const_locations()); i; ++i) {
        for (int r=0; r<=Pattern::MAX_EXTENSION; r++)
            data.neighbours[*i][r].clear();
    }
    
    // try all directions for interior cells
    for (BoardIterator i(const_cells()); i; ++i) {
        int x, y;
        HexPoint cur = *i;
        HexPointUtil::pointToCoords(cur, x, y);

        for (int a=0; a<NUM_DIRECTIONS; a++) {
            int fwd = a;
            int lft = (a + 2) % NUM_DIRECTIONS;

            int x1 = x + HexPointUtil::DeltaX(fwd);
            int y1 = y + HexPointUtil::DeltaY(fwd);

            for (int r=1; r<=Pattern::MAX_EXTENSION; r++) {
                int x2 = x1;
                int y2 = y1;
                for (int t=0; t<r; t++) {
                    HexPoint p = coordsToPoint(x2, y2);
		    
                    if (p != INVALID_POINT) {
                        // add p to cur's list and cur to p's list
                        // for each radius in [r, MAX_EXTENSION]. 
                        for (int v=r; v <= Pattern::MAX_EXTENSION; v++) {
                            std::vector<HexPoint>::iterator result;
                            
                            result = find(data.neighbours[cur][v].begin(), 
                                          data.neighbours[cur][v].end(), p);
                            if (result == data.neighbours[cur][v].end())
                                data.neighbours[cur][v].push_back(p);
                            
                            result = find(data.neighbours[p][v].begin(), 
                                          data.neighbours[p][v].end(), cur);
                            if (result == data.neighbours[p][v].end())
                                data.neighbours[p][v].push_back(cur);
                        }
                    }

                    x2 += HexPointUtil::DeltaX(lft);
                    y2 += HexPointUtil::DeltaY(lft);
                }
                x1 += HexPointUtil::DeltaX(fwd);
                y1 += HexPointUtil::DeltaY(fwd);
            }
        }
    }
    
    // add edges to neighbour lists of neighbouring edges

    /** @bug NORTH is now distance 2 from SOUTH, but won't
        appear in the neighbour lists for r >= 2; likewise for
        EAST/WEST. */
    for (BoardIterator i(const_locations()); i; ++i) {
	if (!HexPointUtil::isEdge(*i)) continue;
        for (int r=1; r <= Pattern::MAX_EXTENSION; r++) {
	    // edges sharing a corner are distance one apart
            data.neighbours[*i][r].push_back(HexPointUtil::leftEdge(*i));
            data.neighbours[*i][r].push_back(HexPointUtil::rightEdge(*i));
	
        }
    }
    
    // Null terminate the lists.
    for (BoardIterator i(const_locations()); i; ++i) {
        for (int r=1; r <= Pattern::MAX_EXTENSION; r++) {
            data.neighbours[*i][r].push_back(INVALID_POINT);
        }
    }
}

//----------------------------------------------------------------------------
