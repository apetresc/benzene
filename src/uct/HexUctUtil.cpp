//----------------------------------------------------------------------------
/** @file HexUctUtil.cpp
*/
//----------------------------------------------------------------------------

#include "SgSystem.h"
#include "Hex.hpp"
#include "HexUctUtil.hpp"

#include <iomanip>
#include <iostream>
#include "SgBWSet.h"
#include "SgPointSet.h"
#include "SgProp.h"
#include "SgUctSearch.h"

//----------------------------------------------------------------------------

void GoGuiGfxStatus(const SgUctSearch& search, std::ostream& out)
{
    const SgUctTree& tree = search.Tree();
    const SgUctNode& root = tree.Root();
    int abortPercent = static_cast<int>(search.AbortedStat().Mean() * 100);
    out << "TEXT N=" << root.MoveCount()
        << " V=" << std::setprecision(2) << root.Mean()
        << " Len=" << static_cast<int>(search.GameLengthStat().Mean())
        << " Tree=" << std::setprecision(1) << search.MovesInTreeStat().Mean()
        << "/" << static_cast<int>(search.MovesInTreeStat().Max())
        << " Abrt=" << abortPercent << '%'
        << " Gm/s=" << static_cast<int>(search.GamesPerSecond()) << '\n';
}

void HexUctUtil::GoGuiGfx(const SgUctSearch& search, SgBlackWhite toPlay,
                          std::ostream& out)
{
    const SgUctTree& tree = search.Tree();
    const SgUctNode& root = tree.Root();
    out << "VAR";
    int bitShift = ComputeMaintainVCBitShift();
    std::vector<const SgUctNode*> bestValueChild;
    bestValueChild.push_back(search.FindBestChild(root));
    for (int i=0; i<4; i++) {
	if (bestValueChild[i] == 0) break;
	SgPoint move = bestValueChild[i]->Move();
	if (0 == (i % 2))
	    out << ' ' << (toPlay == SG_BLACK ? 'B' : 'W') << ' '
		<< HexUctUtil::MoveString(move, bitShift);
	else
	    out << ' ' << (toPlay == SG_WHITE ? 'B' : 'W') << ' '
		<< HexUctUtil::MoveString(move, bitShift);
	bestValueChild.push_back(search.FindBestChild(*(bestValueChild[i])));
    }
    out << "\n";
    out << "INFLUENCE";
    for (SgUctChildIterator it(tree, root); it; ++it)
    {
        const SgUctNode& child = *it;
        if (child.MoveCount() == 0)
            continue;
        float value = search.InverseEval(child.Mean());
        // Scale to [-1,+1], black positive
        double influence = value * 2 - 1;
        if (toPlay == SG_WHITE)
            influence *= -1;
        SgPoint move = child.Move();
        out << ' ' << HexUctUtil::MoveString(move, bitShift) << ' ' 
            << std::fixed << std::setprecision(2) << influence;
    }
    out << '\n'
        << "LABEL";
    int numChildren = 0, numZeroExploration = 0, numSmallExploration = 0;
    for (SgUctChildIterator it(tree, root); it; ++it)
    {
        const SgUctNode& child = *it;
        size_t count = child.MoveCount();
	numChildren++;
	if (count < 10)
	    numSmallExploration++;
        if (count == 0)
	    numZeroExploration++;
	out << ' ' << HexUctUtil::MoveString(child.Move(), bitShift)
	    << ' ' << count;
    }
    out << '\n';
    GoGuiGfxStatus(search, out);

    out << numSmallExploration << " root children minimally explored with "
	<< numZeroExploration << " zeroes of " << numChildren << " total."
        << '\n';
}

int HexUctUtil::ComputeMaintainVCBitShift()
{
    int bitShift = hex::settings.get_int("uct-max-maintainable-vcs");
    HexAssert(bitShift < 32);
    while ((1 << bitShift) < FIRST_INVALID) 
    {
	bitShift++;
    }
    return bitShift;
}

int HexUctUtil::ComputeMaxNumMoves()
{
    if (!hex::settings.get_bool("uct-maintain-vcs"))
	return FIRST_INVALID;
    
    return (1 << (ComputeMaintainVCBitShift() + 1));
}

std::string HexUctUtil::MoveString(SgMove sgmove, int maintainVCBitShift)
{
    HexPoint move = static_cast<HexPoint>(sgmove);

    // Simple process if just HexPoint
    HexAssert(move >= 0);
    if (move < FIRST_INVALID)
	return HexPointUtil::toString(move);
    
    // Ensure is valid VC maintenance move
    HexAssert(maintainVCBitShift > 0);
    bitset_t moveAsBitset(move);
    HexAssert(moveAsBitset.test(maintainVCBitShift));
    moveAsBitset.flip(maintainVCBitShift);
    
    // Extract the VCs maintained, and print preceded by "M"
    std::ostringstream os;
    os << "M" << (uint) moveAsBitset.to_ulong();
    return os.str();
}

SgBlackWhite HexUctUtil::ToSgBlackWhite(HexColor c)
{
    if (c == BLACK)
	return SG_BLACK;
    HexAssert(c == WHITE);
    return SG_WHITE;
}

//----------------------------------------------------------------------------

