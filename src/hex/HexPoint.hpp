//----------------------------------------------------------------------------
// $Id: HexPoint.hpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#ifndef HEXPOINT_HPP
#define HEXPOINT_HPP

#include <map>
#include <string>
#include <vector>
#include <utility>

#include "Bitset.hpp"
#include "HexAssert.hpp"
#include "HexColor.hpp"

//----------------------------------------------------------------------------

/** @page hexpoints HexPoints 

    There are three types of HexPoints: special, edges, and interior.
    Special points encode special moves that do not correspond to a
    physical location on a hex board. These are: INVALID_POINT,
    RESIGN, and SWAP_PIECES.  Edge points (NORTH, SOUTH, EAST, WEST)
    denote the edges of the board.  Internal points are the interior
    points of the board, the number of which is controlled by the
    constants MAX_WIDTH and MAX_HEIGHT.

    HexPoints are laid out in memory as follows:

    @verbatim

      0   INVALID_POINT
      1   RESIGN          
      2   SWAP_PIECES     
      3   NORTH
      4   EAST
      5   SOUTH
      6   WEST
      7   1st interior point
      8   2nd interior point
      ...
      ...
      ... FIRST_INVALID
    @endverbatim

    It is assumed that the special points (i.e. SWAP_PIECES and
    RESIGN) come immediately before the edge points (i.e. NORTH to
    WEST) which come immediately before the interior points.

    The interior points are laid out as follows.  The first MAX_WIDTH
    interior points get the name 'a1, b1, c1, ... , L1', where L is
    letter MAX_WIDTH-1 letters after 'a'. The next MAX_WIDTH points
    get a '2' suffix, then a '3' suffix, and so on, until MAX_HEIGHT
    is reached.

    This encoding allows an 11x11 hex board to fit into 128 bits if
    MAX_WIDTH = 11 and MAX_HEIGHT = 11.
*/

//----------------------------------------------------------------------------

/** @name Maximum dimensions.

    If you are going to change either of these constants, make sure to
    synchronize the printed names in HexPointData with the enumerated
    interior cell constants below.
*/
// @{ 

/** The maximum width of a valid ConstBoard. */
static const int MAX_WIDTH  = 11;

/** The maximum height of a valid ConstBoard. */
static const int MAX_HEIGHT = 11;

// @}

//----------------------------------------------------------------------------

/** A location on a Hex board.  A HexPoint's neighbours can be
    calculated only when considering what size board the point is in.
    See ConstBoard for an example board layout.

    @note The order of these points if very important. There are
    several pieces of code that rely on this ordering
    (StoneBoard::GetBoardID() is one of them). Change this only if you
    know what you are doing!
*/
typedef enum
{
    /** Dummy point. */
    INVALID_POINT = 0,

    /** Point used to denote a resign move by a player. 

        @todo since this is not really a point on a board, should we
        create a HexMove class and put this there? */
    RESIGN = 1,

    /** Point used to denote a swap move by a player. 

        @todo since this is not really a point on a board, should 
        we create a HexMove class and put this there?  */
    SWAP_PIECES = 2,

    /** The top edge. */
    NORTH = 3,

    /** The right edge. */
    EAST  = 4,

    /** The bottom edge. */
    SOUTH = 5,

    /** The left edge. */
    WEST  = 6,

    /** @name Interior cells. */
    // @{

    HEX_CELL_A1, HEX_CELL_B1, HEX_CELL_C1, HEX_CELL_D1, HEX_CELL_E1,
    HEX_CELL_F1, HEX_CELL_G1, HEX_CELL_H1, HEX_CELL_I1, HEX_CELL_J1,
    HEX_CELL_K1,

    HEX_CELL_A2, HEX_CELL_B2, HEX_CELL_C2, HEX_CELL_D2, HEX_CELL_E2,
    HEX_CELL_F2, HEX_CELL_G2, HEX_CELL_H2, HEX_CELL_I2, HEX_CELL_J2,
    HEX_CELL_K2,

    HEX_CELL_A3, HEX_CELL_B3, HEX_CELL_C3, HEX_CELL_D3, HEX_CELL_E3,
    HEX_CELL_F3, HEX_CELL_G3, HEX_CELL_H3, HEX_CELL_I3, HEX_CELL_J3,
    HEX_CELL_K3,

    HEX_CELL_A4, HEX_CELL_B4, HEX_CELL_C4, HEX_CELL_D4, HEX_CELL_E4,
    HEX_CELL_F4, HEX_CELL_G4, HEX_CELL_H4, HEX_CELL_I4, HEX_CELL_J4,
    HEX_CELL_K4,

    HEX_CELL_A5, HEX_CELL_B5, HEX_CELL_C5, HEX_CELL_D5, HEX_CELL_E5,
    HEX_CELL_F5, HEX_CELL_G5, HEX_CELL_H5, HEX_CELL_I5, HEX_CELL_J5,
    HEX_CELL_K5,

    HEX_CELL_A6, HEX_CELL_B6, HEX_CELL_C6, HEX_CELL_D6, HEX_CELL_E6,
    HEX_CELL_F6, HEX_CELL_G6, HEX_CELL_H6, HEX_CELL_I6, HEX_CELL_J6,
    HEX_CELL_K6,

    HEX_CELL_A7, HEX_CELL_B7, HEX_CELL_C7, HEX_CELL_D7, HEX_CELL_E7,
    HEX_CELL_F7, HEX_CELL_G7, HEX_CELL_H7, HEX_CELL_I7, HEX_CELL_J7,
    HEX_CELL_K7,

    HEX_CELL_A8, HEX_CELL_B8, HEX_CELL_C8, HEX_CELL_D8, HEX_CELL_E8,
    HEX_CELL_F8, HEX_CELL_G8, HEX_CELL_H8, HEX_CELL_I8, HEX_CELL_J8,
    HEX_CELL_K8,

    HEX_CELL_A9, HEX_CELL_B9, HEX_CELL_C9, HEX_CELL_D9, HEX_CELL_E9,
    HEX_CELL_F9, HEX_CELL_G9, HEX_CELL_H9, HEX_CELL_I9, HEX_CELL_J9,
    HEX_CELL_K9,

    HEX_CELL_A10, HEX_CELL_B10, HEX_CELL_C10, HEX_CELL_D10, HEX_CELL_E10,
    HEX_CELL_F10, HEX_CELL_G10, HEX_CELL_H10, HEX_CELL_I10, HEX_CELL_J10,
    HEX_CELL_K10,

    HEX_CELL_A11, HEX_CELL_B11, HEX_CELL_C11, HEX_CELL_D11, HEX_CELL_E11,
    HEX_CELL_F11, HEX_CELL_G11, HEX_CELL_H11, HEX_CELL_I11, HEX_CELL_J11,
    HEX_CELL_K11,
    
    // @}
    
    /** The invalid HexPoint. */
    FIRST_INVALID

} HexPoint;

/** The value of the first special HexPoint. */
static const HexPoint FIRST_SPECIAL = RESIGN;

/** The value of the first edge HexPoint. */
static const HexPoint FIRST_EDGE = NORTH;

/** The value of the first interior cell; this should always be A1. */
static const HexPoint FIRST_CELL = HEX_CELL_A1;
    
//---------------------------------------------------------------------------

/** A map of points to points. */
typedef std::map<HexPoint, HexPoint> PointToPoint;

/** Pair of HexPoints. */
typedef std::pair<HexPoint, HexPoint> HexPointPair;

/** Set of HexPoints. */
typedef std::set<HexPoint> HexPointSet;

/** Map of HexPoints to bitsets. */
typedef std::map<HexPoint, bitset_t> PointToBitset;

/** A sequence of HexPoints. */
typedef std::vector<HexPoint> MoveSequence;

//---------------------------------------------------------------------------

/**
   Delta arrays.
 
   On a hex board, we can travel only in the following six directions:
   EAST, NORTH_EAST, NORTH, WEST, SOUTH_WEST, SOUTH.
 
   @verbatim 
             | /  
             |/
         --- o ---
            /|
           / |
   @endverbatim
*/
enum HexDirection 
{ 
    DIR_EAST=0, 
    DIR_NORTH_EAST, 
    DIR_NORTH, 
    DIR_WEST, 
    DIR_SOUTH_WEST, 
    DIR_SOUTH, 
    NUM_DIRECTIONS 
};

//----------------------------------------------------------------------------

/** Utilities on HexPoints: converting to/from strings, testing for edges, 
    converting to/from x/y coordinates, etc. */
namespace HexPointUtil
{
    /** Converts a HexPoint to a string. */
    std::string toString(HexPoint p);

    /** Converts a pair of HexPoints to a string. */
    std::string toString(const HexPointPair& p);

    /** Returns a space separated output of points in lst. */
    std::string ToPointListString(const std::vector<HexPoint>& lst);

    /** Returns a string representation of the bitset's set bits. */
    std::string ToPointListString(const bitset_t& b);

    /** Returns the HexPoint with the given name; INVALID_POINT otherwise. */
    HexPoint fromString(const std::string& name);

    /** Returns true if this point is a swap move. */
    bool isSwap(HexPoint c);

    /** Returns true if this point is an edge point. */
    bool isEdge(HexPoint c);

    /** Returns true if this point is an interior cell. */
    bool isInteriorCell(HexPoint c);

    /** Returns the edge opposite the given edge. */
    HexPoint oppositeEdge(HexPoint edge);

    /** Returns the edge to the left of the given edge. */
    HexPoint leftEdge(HexPoint edge);

    /** Returns the edge to the right of the given edge. */
    HexPoint rightEdge(HexPoint edge);

    /** Returns a color's first edge.  NORTH for VERTICAL_COLOR
        and EAST for !VERTICAL_COLOR. */ 
    HexPoint colorEdge1(HexColor color);

    /** Returns a color's second edge.  SOUTH for VERTICAL_COLOR
        and WEST for !VERTICAL_COLOR. */
    HexPoint colorEdge2(HexColor color);

    /** Returns true if cell is one of color's edges. */
    bool isColorEdge(HexPoint cell, HexColor color);

    /** Converts cell into its x and y components, where
        @code
        x = (cell-FIRST_CELL) % MAX_WIDTH;
        y = (cell-FIRST_CELL) / MAX_WIDTH;
        @endcode
        
        Does not handle cases where cell is an edge.
        ConstBoard has a method for that.
    */
    void pointToCoords(HexPoint cell, int& x, int& y);

    /** Returns the HexPoint corresponding to the given x and y coords. 
        @code 
        point =  FIRST_CELL + (y*MAX_WIDTH) + x;
        @endcode
    */
    HexPoint coordsToPoint(int x, int y);

    /** Returns the change in the x-coordinate when travelling in the
        given direction. */
    int DeltaX(int dir);

    /** Returns the change in the y-coordinate when travelling in the
        given direction. */
    int DeltaY(int dir);

} // namespace

//----------------------------------------------------------------------------

inline bool HexPointUtil::isSwap(HexPoint c)
{
    return (c==SWAP_PIECES);
}

inline bool HexPointUtil::isEdge(HexPoint c)
{ 
    return (c==NORTH || c==SOUTH || c==EAST || c==WEST); 
}

inline bool HexPointUtil::isInteriorCell(HexPoint c)
{
    return (FIRST_CELL <= c && c < FIRST_INVALID);
}

inline HexPoint HexPointUtil::oppositeEdge(HexPoint edge)
{
    HexAssert(isEdge(edge));
    if (edge == NORTH) return SOUTH;
    if (edge == SOUTH) return NORTH;
    if (edge == EAST)  return WEST;
    HexAssert(edge == WEST);
    return EAST;
}

inline HexPoint HexPointUtil::leftEdge(HexPoint edge)
{
    HexAssert(isEdge(edge));
    if (edge == NORTH) return EAST;
    if (edge == SOUTH) return WEST;
    if (edge == EAST)  return SOUTH;
    HexAssert(edge == WEST);
    return NORTH;
}

inline HexPoint HexPointUtil::rightEdge(HexPoint edge)
{
    HexAssert(isEdge(edge));
    if (edge == NORTH) return WEST;
    if (edge == SOUTH) return EAST;
    if (edge == EAST)  return NORTH;
    HexAssert(edge == WEST);
    return SOUTH;
}

inline HexPoint HexPointUtil::colorEdge1(HexColor color)
{
    HexAssert(HexColorUtil::isBlackWhite(color));
    return (color == VERTICAL_COLOR) ? NORTH : EAST;
}

inline HexPoint HexPointUtil::colorEdge2(HexColor color)
{
    HexAssert(HexColorUtil::isBlackWhite(color));
    return (color == VERTICAL_COLOR) ? SOUTH : WEST;
}

inline bool HexPointUtil::isColorEdge(HexPoint cell, HexColor color)
{
    HexAssert(HexColorUtil::isBlackWhite(color));
    return (cell == colorEdge1(color) || cell == colorEdge2(color));
}

inline void HexPointUtil::pointToCoords(HexPoint cell, int& x, int& y)
{
    HexAssert(FIRST_CELL <= cell && cell < FIRST_INVALID);
    x = (cell-FIRST_CELL) % MAX_WIDTH;
    y = (cell-FIRST_CELL) / MAX_WIDTH;
}

inline HexPoint HexPointUtil::coordsToPoint(int x, int y)
{
    HexAssert(0 <= x && x < MAX_WIDTH);
    HexAssert(0 <= y && y < MAX_HEIGHT);
    return static_cast<HexPoint>(FIRST_CELL + (y*MAX_WIDTH) + x);
}

inline int HexPointUtil::DeltaX(int dir)
{
    HexAssert(DIR_EAST <= dir < NUM_DIRECTIONS);
    static const int dx[] = {1,  1,  0, -1, -1, 0};
    return dx[dir];
}

inline int HexPointUtil::DeltaY(int dir)
{
    HexAssert(DIR_EAST <= dir < NUM_DIRECTIONS);
    static const int dy[] = {0, -1, -1,  0,  1, 1};
    return dy[dir];
}

//----------------------------------------------------------------------------

/** Extends standard output operator to handle HexPoints. */
inline std::ostream& operator<<(std::ostream& os, HexPoint p)
{
    return os << HexPointUtil::toString(p);
}

//----------------------------------------------------------------------------

#endif // HEXPOINT_HPP
