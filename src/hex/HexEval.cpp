//----------------------------------------------------------------------------
// $Id: HexEval.cpp 1797 2008-12-15 02:32:52Z broderic $
//----------------------------------------------------------------------------

#include "Hex.hpp"
#include "HexEval.hpp"
#include "HexBoard.hpp"

//----------------------------------------------------------------------------

bool HexEvalUtil::IsValidEval(HexEval ev)
{
    return (ev >= IMMEDIATE_LOSS && ev <= IMMEDIATE_WIN);
}

bool HexEvalUtil::IsWin(HexEval ev)
{
    HexAssert(IsValidEval(ev));
    return ev >= WIN_THRESHOLD;
}

int HexEvalUtil::PlyToWin(HexEval ev)
{
    HexAssert(IsValidEval(ev));
    return (int)(IMMEDIATE_WIN - ev);
}

bool HexEvalUtil::IsLoss(HexEval ev)
{
    HexAssert(IsValidEval(ev));
    return ev <= LOSS_THRESHOLD;
}

int HexEvalUtil::PlyToLoss(HexEval ev)
{
    HexAssert(IsValidEval(ev));
    return (int)(ev - IMMEDIATE_LOSS);
}

bool HexEvalUtil::IsWinOrLoss(HexEval ev)
{
    return IsWin(ev) || IsLoss(ev);
}

//----------------------------------------------------------------------------
