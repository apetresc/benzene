//----------------------------------------------------------------------------
/** @file HexUctUtil.hpp
    
*/
//----------------------------------------------------------------------------

#ifndef HEXUCTUTIL_H
#define HEXUCTUTIL_H

#include "SgBlackWhite.h"
#include "SgPoint.h"
#include "SgUctSearch.h"


//----------------------------------------------------------------------------

/** General utility functions used in GoUct.
    These functions are used in GoUct, but should not depend on other classes
    in GoUct to avoid cyclic dependencies.
*/
namespace HexUctUtil
{
    /** Print information about search as Gfx commands for GoGui.
        Can be used for GoGui live graphics during the search or GoGui
        analyze command type "gfx" after the search (see http://gogui.sf.net).
        The following information is output:
        - Move values as influence
        - Move counts as labels
        - Move with best value marked with circle
        - Best response marked with triangle
        - Move with highest count marked with square (if different from move
          with best value)
        - Status line text:
          - N = Number games
          - V = Value of root node
          - Len = Average simulation sequence length
          - Tree = Average/maximum moves of simulation sequence in tree
          - Abrt = Percentage of games aborted (due to maximum game length)
          - Gm/s = Simulations per second
        @param search The search containing the tree and statistics
        @param toPlay The color toPlay at the root node of the tree
        @param out The stream to write the gfx commands to
    */
    void GoGuiGfx(const SgUctSearch& search, SgBlackWhite toPlay,
                  std::ostream& out);
    
    /** RAVE is more efficient if we know the max number of moves we
	can have. Simply returns FIRST_INVALID. */
    int ComputeMaxNumMoves();
    
    /** Method used to print SgMoves during UCT. */
    std::string MoveString(SgMove move);
    
    /** Converts a HexColor to SgBlackWhite (Note: cannot be EMPTY). */
    SgBlackWhite ToSgBlackWhite(HexColor c);
}

//----------------------------------------------------------------------------

#endif // GOUCTUTIL_H

