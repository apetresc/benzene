//----------------------------------------------------------------------------
// $Id: HexProp.cpp 1470 2008-06-17 22:04:32Z broderic $
//----------------------------------------------------------------------------

#include "SgSystem.h"
#include "SgProp.h"
#include "HexProp.hpp"

//----------------------------------------------------------------------------

void HexInitProp()
{
    SgProp* moveProp = new SgPropMove(0);
    SG_PROP_MOVE_BLACK 
        = SgProp::Register(moveProp, "B", fMoveProp + fBlackProp);
    SG_PROP_MOVE_WHITE
        = SgProp::Register(moveProp, "W", fMoveProp + fWhiteProp);
}

//----------------------------------------------------------------------------

SgProp* HexPropUtil::AddMoveProp(SgNode* node, SgMove move, 
                                 SgBlackWhite player)
{
    SG_ASSERT_BW(player);
    SgProp* moveProp = node->AddMoveProp(move, player);
    return moveProp;
}

//----------------------------------------------------------------------------
