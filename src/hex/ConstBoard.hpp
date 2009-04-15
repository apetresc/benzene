//----------------------------------------------------------------------------
// $Id: ConstBoard.hpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#ifndef CONSTBOARD_H
#define CONSTBOARD_H

#include "Hex.hpp"
#include "Pattern.hpp"
#include "BoardIterator.hpp"

//----------------------------------------------------------------------------

/** @page boardrepresentation Board Representation
    The HexPoints on the board are laid out as in the following diagram
    (see @ref hexpoints for information on HexPoints):
  
    @verbatim

                    NORTH
               \--a--b--c-...-\
               1\  0  1  2 ... \ 1
      WEST      2\ 16 17 18 ... \ 2  EAST
                 3\ 32 33 34 ... \ 3
                   \--a--b--c-...-\
                        SOUTH
    @endverbatim
*/

/** @page cellneighbours Cell Neighbours 
    The neighbour lists for the interior cells behave as you would
    expect, e.q. a1 is adjacent to b1, NORTH, WEST, and a2. For edges,
    adjacent edges are added to the neighbour lists for all radius',
    but the closure of this is not computed.  For example, WEST is in
    the radius 1 neighbour list of NORTH, but SOUTH is not in the
    radius 2 neighbour list of NORTH.  Nor is this closure computed
    for interior cells over edges; e.g, a1 is distance 1 from NORTH
    but not distance 2 from EAST (except on a 1x1 board, of course).
*/

/** Base class for all boards.  
    
    ConstBoard contains data and methods for dealing with the constant
    aspects of a Hex board.  That is, ConstBoard stores a cell's
    neighbours, cell-to-cell distances, etc. It also offers iterators
    to run over the board and a cell's neighbours and methods to
    rotate points and bitsets.

    See @ref boardrepresentation for how the points are laid out on an
    example board, and @ref cellneighbours for a discussion on how
    neighbours are calculated.

    This class does not track played stones. To keep track of played
    stone information see StoneBoard.
 */
class ConstBoard
{
public:
    
    /** Constructs a square board. */
    ConstBoard(int size);

    /** Constructs a rectangular board. */
    ConstBoard(int width, int height);

    /** Destructor. */
    ~ConstBoard();

    //------------------------------------------------------------------------

    /** Returns the width of the board. */
    int width() const;

    /** Returns the height of the board. */
    int height() const;

    /** Returns a bitset with all valid board cells. */
    bitset_t getCells() const;

    /** Returns a bitset with all valid board locations
	(cells and edges). */
    bitset_t getLocations() const;

    /** Returns a bitset of cells comprising all valid moves
	(this includes swap and resign). */
    bitset_t getValid() const;
    
    /** Returns true if cell is a valid cell on this board. */
    bool isCell(HexPoint cell) const;

    /** Returns true if bs encodes a set of valid cells. */
    bool isCell(const bitset_t& bs) const;
    
    /** Returns true if cell is a location on this board. */
    bool isLocation(HexPoint cell) const;

    /** Returns true if bs encodes a set of valid locations. */
    bool isLocation(const bitset_t& bs) const;
    
    /** Returns true if cell is a valid move on this board. */
    bool isValid(HexPoint cell) const;

    /** Returns true if bs encodes a set of valid moves. */
    bool isValid(const bitset_t& bs) const;

    //------------------------------------------------------------------------

    /** Packs a bitset on this boardsize. 
        Output bitset has a bit for each cell on the board, starting
        at a1. That is, an 8x8 board can fit into exactly 64 bits. 
        If in has a bit set at a1, then the packed bitset will
        have its 0th bit set; if in has a bit at a2, the second bit
        is set, etc, etc. */
    bitset_t PackBitset(const bitset_t& in) const;

    /** Unpacks a bitset to the canonical representation. */
    bitset_t UnpackBitset(const bitset_t& in) const;

    //------------------------------------------------------------------------

    /** Returns the distance between the two cells. */
    int distance(HexPoint x, HexPoint y) const;

    /** Rotates the given point 180' about the center of the board. */
    HexPoint rotate(HexPoint p) const;

    /** Rotates the given bitset 180' about the center of the
        board. */
    bitset_t rotate(const bitset_t& bs) const;

    /** Mirrors the given point in the diagonal joining acute corners.
	Note that this function requires square boards! */
    HexPoint mirror(HexPoint p) const;

    /** Mirrors the given bitset in the acute diagonal (requires that
	width equals height). */
    bitset_t mirror(const bitset_t& bs) const;

    /** Returns the center point on boards where both dimensions are
	odd (fails on all other boards). */
    HexPoint centerPoint() const;

    /** These two methods return the two points nearest the center of
	the board. On boards with one or more even dimensions, out of
	the center-most points the "top right" and "bottom left"
	points are returned as they relate to the main
	diagonals/visually. On boards with two odd dimensions, both of
	these methods return the same point as centerPoint(). */
    HexPoint centerPointRight() const;

    /** See centerPointRight(). */
    HexPoint centerPointLeft() const;

    //------------------------------------------------------------------------

    /** Returns a iterator to the interior board cells. */
    BoardIterator const_cells() const;
    
    /** Returns an iterator to the board cells, starting on
        the outer edges. */
    BoardIterator const_locations() const;

    /** Returns an iterator that runs over all valid moves. */
    BoardIterator const_valid() const;

    /** Returns an iterator to the first neighbour of cell. 
        @see @ref cellneighbours
    */
    BoardIterator const_nbs(HexPoint cell) const;

    /** Returns an iterator to the neighbourhood extending outward by
        radius cells of cell. */
    BoardIterator const_nbs(HexPoint cell, int radius) const;

    //------------------------------------------------------------------------

    /** Returns true if p1 is adjacent to p2. */
    bool isAdjacent(HexPoint p1, HexPoint p2) const;
    
    /** Returns HexPoint at the coordinates (x, y). Note that edges
        have a single coordinate equal to -1 or width()/height(). */
    HexPoint coordsToPoint(int x, int y) const;

    /** Returns HexPoint in direction dir from the point p. 
        If p is an edge, returns p. */
    HexPoint PointInDir(HexPoint p, HexDirection dir) const;

    /** Shifts the bitset in direction dir using PointInDir(). Returns
        true each interior point is still an interior point. */
    bool ShiftBitset(const bitset_t& bs, HexDirection dir,
                     bitset_t& out) const;


private:

    int distanceToEdge(HexPoint from, HexPoint edge) const;

    //-----------------------------------------------------------------------

    /** Constant data for a single boardsize. */
    struct StaticData
    {
        /** Dimensions. */
        int width, height;

        /** The set of all valid cells/moves. Assumed to be in the
            following order: special moves, edges, interior cells. */
        std::vector<HexPoint> points;

        /** Will probably always be zero. 
            @todo remove? */
        int all_index;

        /** Index in points where edges start. */
        int locations_index;

        /** Index in points where interior cells start. */
        int cells_index;

        /** All valid moves/cells. */
        bitset_t valid;
        
        /** All valid locations. */
        bitset_t locations;

        /** All valid interior cells. */
        bitset_t cells;

        /** Neigbour lists for each location and radius. */
        std::vector<HexPoint> 
        neighbours[BITSETSIZE][Pattern::MAX_EXTENSION+1];
    };

    int init();
    void computeNeighbours(StaticData& data);
    void computePointList(StaticData& data);
    void createIterators(StaticData& data);
    void computeValid(StaticData& data);

    /** Data for each boardsize. */
    static std::vector<StaticData> s_data;

    //-----------------------------------------------------------------------

    /** Width of board in cells. */
    int m_width;

    /** Height of board in cells. */
    int m_height;

    /** Index into s_data for this boardsize. */
    int m_index;
};

inline int ConstBoard::width() const
{ 
    return m_width; 
}

inline int ConstBoard::height() const
{ 
    return m_height;
}

inline bitset_t ConstBoard::getCells() const
{
    return s_data[m_index].cells;
}

inline bitset_t ConstBoard::getLocations() const
{
    return s_data[m_index].locations;
}

inline bitset_t ConstBoard::getValid() const
{
    return s_data[m_index].valid;
}

inline bool ConstBoard::isCell(HexPoint cell) const
{
    return s_data[m_index].cells.test(cell);
}

inline bool ConstBoard::isCell(const bitset_t& bs) const
{
    return BitsetUtil::IsSubsetOf(bs, s_data[m_index].cells);
}

inline bool ConstBoard::isLocation(HexPoint cell) const
{
    return s_data[m_index].locations.test(cell);
}

inline bool ConstBoard::isLocation(const bitset_t& bs) const
{
    return BitsetUtil::IsSubsetOf(bs, s_data[m_index].locations);
}

inline bool ConstBoard::isValid(HexPoint cell) const
{
    return s_data[m_index].valid.test(cell);
}

inline bool ConstBoard::isValid(const bitset_t& bs) const
{
    return BitsetUtil::IsSubsetOf(bs, s_data[m_index].valid);
}

inline BoardIterator ConstBoard::const_cells() const
{
    return BoardIterator(&s_data[m_index].points[s_data[m_index].cells_index]);
}

inline BoardIterator ConstBoard::const_locations() const
{ 
    return BoardIterator(&s_data[m_index].points[s_data[m_index].locations_index]);
}

inline BoardIterator ConstBoard::const_valid() const
{ 
    return BoardIterator(&s_data[m_index].points[s_data[m_index].all_index]);
}

inline BoardIterator ConstBoard::const_nbs(HexPoint cell) const
{
    HexAssert(isLocation(cell));
    return BoardIterator(s_data[m_index].neighbours[cell][1]);
}

inline BoardIterator ConstBoard::const_nbs(HexPoint cell, int radius) const
{
    HexAssert(isLocation(cell));
    HexAssert(0 <= radius && radius <= Pattern::MAX_EXTENSION);
    return BoardIterator(s_data[m_index].neighbours[cell][radius]);
}

//----------------------------------------------------------------------------

#endif  // CONSTBOARD_HPP
