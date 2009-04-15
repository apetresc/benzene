//----------------------------------------------------------------------------
// $Id: HexSgUtil.hpp 1470 2008-06-17 22:04:32Z broderic $
//----------------------------------------------------------------------------

#ifndef HEXSGUTIL_HPP
#define HEXSGUTIL_HPP

#include "SgBlackWhite.h"
#include "SgPoint.h"
#include "SgProp.h"
#include "SgNode.h"

#include "Hex.hpp"
#include "StoneBoard.hpp"

//----------------------------------------------------------------------------

/** Utilities to convert from/to Hex and Sg. */
namespace HexSgUtil
{
    /** Converts a HexPoint to a SgPoint. */
    SgPoint HexPointToSgPoint(HexPoint p, int height);

    /** Converts from from a SgPoint to a HexPoint. */
    HexPoint SgPointToHexPoint(SgPoint p, int height);

    /** Converts a HexColor to SgBlackWhite,  color must not equal
        EMPTY. */
    SgBlackWhite HexColorToSgColor(HexColor color);

    /** Converts a SgBlackWhite to a HexColor. */
    HexColor SgColorToHexColor(SgBlackWhite player);

    /** Convert a bitset to an SgList of points. */
    SgList<SgPoint> BitsetToSgList(const bitset_t& b, int height);

    //------------------------------------------------------------------------

    /** Adds the move to the sg node; does proper conversions. */
    void AddMoveToNode(SgNode* node, HexColor color, 
                       HexPoint cell, int height);

    /** Returns true if node constains any of the following
        properties: propAddBlack, propAddWhite, propAddEmpty,
        propPlayer. */
    bool NodeHasSetupInfo(SgNode* node);

    /** Set the position setup properties of this node to encode the 
        given board.*/
    void SetPositionInNode(SgNode* root, 
                           const StoneBoard& brd, 
                           HexColor color);

    /** Puts the position in the given vectors. */
    void GetSetupPosition(const SgNode* node, 
                          int height,
                          std::vector<HexPoint>& black,
                          std::vector<HexPoint>& white,
                          std::vector<HexPoint>& empty);

    /** Sets node's position in the board. */
    void SetPositionInBoard(const SgNode* node, StoneBoard& brd);

    //------------------------------------------------------------------------

    /** Write the given tree to a sgf file. Returns true on success,
        false otherwise. */
    bool WriteSgf(SgNode* tree, const std::string& application, 
                  const char* filename, int boardsize);

}

//----------------------------------------------------------------------------

#endif // HEXSGUTIL_HPP
