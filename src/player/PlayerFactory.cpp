//----------------------------------------------------------------------------
// $Id: PlayerFactory.cpp 1715 2008-10-28 22:13:37Z broderic $
//----------------------------------------------------------------------------

#include "PlayerFactory.hpp"
#include "BookCheck.hpp"
#include "EndgameCheck.hpp"
#include "HandBookCheck.hpp"
#include "LadderCheck.hpp"
#include "SolverCheck.hpp"
#include "SwapCheck.hpp"
#include "VulPreCheck.hpp"

/** @file

    Various factory methods for creating players. 
*/

//----------------------------------------------------------------------------

UofAPlayer* PlayerFactory::CreatePlayer(UofAPlayer* player)
{
    return 
        new SwapCheck
        (new EndgameCheck
	 (new SolverCheck 
	  (new LadderCheck(player))));
}

UofAPlayer* PlayerFactory::CreatePlayerWithBook(UofAPlayer* player)
{
    return 
        new SwapCheck
        (new EndgameCheck
	 (new HandBookCheck
          (new BookCheck
	   (new SolverCheck
	    (new LadderCheck(player))))));
}

UofAPlayer* PlayerFactory::CreateTheoryPlayer(UofAPlayer* player)
{
    return new VulPreCheck(player);
}

//----------------------------------------------------------------------------
