//----------------------------------------------------------------------------
// $Id: BoardUtils.hpp 1699 2008-09-24 23:55:56Z broderic $
//----------------------------------------------------------------------------

#ifndef BOARDUTILS_HPP
#define BOARDUTILS_HPP

#include "Hex.hpp"
#include "HexBoard.hpp"

//----------------------------------------------------------------------------

/** Utilities on Boards. */
namespace BoardUtils
{
    /** Must be called before any of these functions can be used! */
    void Initialize();

    //-------------------------------------------------------------------------

    /** Returns a random empty cell or INVALID_POINT if the board is
        full. */
    HexPoint RandomEmptyCell(const StoneBoard& brd);

    //-------------------------------------------------------------------------

    /** Returns true if p1 is connected to p2 on the bitset; p1 and p2
        are assumed to be inside the bitset.  */
    bool ConnectedOnBitset(const ConstBoard& brd, 
                           const bitset_t& carrier,
                           HexPoint p1, HexPoint p2);

    /** This second variation is identical to the first except that it
	adds some edges not on the board.  */
    bool ConnectedOnBitset(const ConstBoard& brd, 
                           const bitset_t& carrier,
                           HexPoint p1, HexPoint p2,
			   const std::vector<HexPointPair>& addedEdges);
    
    /** Returns a subset of carrier: the points that are reachable
        from start. Flow will enter and leave cells in carrier, and
        enter but not leave cells in stopset. */
    bitset_t ReachableOnBitset(const ConstBoard& brd, 
                               const bitset_t& carrier,
                               const bitset_t& stopset, 
                               HexPoint start);
    /** Same as first variant but uses added edges in addition to
        normal board adjacencies. Flow will enter and leave cells in
        carrier, and enter but not leave cells in stopset. */
    bitset_t ReachableOnBitset(const ConstBoard& brd, 
                               const bitset_t& carrier,
                               const bitset_t& stopset, 
                               HexPoint start,
			       const std::vector<HexPointPair>& addedEdges);

    //-------------------------------------------------------------------------

    /** Returns true if there is a combinatorial decomposition for color
	where the opposing color edges are VC-connected. The VC's carrier
	will be stored in captured. */
    bool FindCombinatorialDecomposition(const HexBoard& brd, HexColor color, 
                                        bitset_t& captured);

    /** Returns true if there is a combinatorial decomposition for color
	that splits the board (i.e. touches both edges of the opposite color).
	Group that splits the board is stored in group.  If there is a VC from
	that group to an edge, the carrier will be stored in captured. */
    bool FindSplittingDecomposition(const HexBoard& brd, HexColor color, 
				    HexPoint& group, bitset_t& captured);

    //----------------------------------------------------------------------

    /** Dumps all cells outside the consider set and the remove set
        in a format the gui expects. */
    std::string GuiDumpOutsideConsiderSet(const StoneBoard& brd, 
                                          const bitset_t& consider,
                                          const bitset_t& remove);
}

//----------------------------------------------------------------------------

#endif // BOARDUTILS_HPP
