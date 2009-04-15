//----------------------------------------------------------------------------
// $Id: PlayerFactory.hpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#ifndef PLAYERFACTORY_HPP
#define PLAYERFACTORY_HPP

#include "UofAPlayer.hpp"

//----------------------------------------------------------------------------

/** Creates players. */
namespace PlayerFactory
{
    /** Creates player with default functionality. 
        Executes in the following order:

        pre_search(): 
	    1) SwapCheck (can short-circuit)
            2) EndgameCheck (can short-circuit)
            3) LadderCheck (no short-circuit)

        post_search(): 

     */
    UofAPlayer* CreatePlayer(UofAPlayer* player);

    /** Creates player with default functionality plus book check.
        Executes in the following order:

        pre_search(): 
           1) SwapCheck (can short-circuit)
           2) EndgameCheck (can short-circuit)
           3) HandBookCheck (can short-circuit)
           4) BookCheck (can short-circuit)
           5) LadderCheck (no short-circuit)

        post_search():
           
    */
    UofAPlayer* CreatePlayerWithBook(UofAPlayer* player);
    
    /** Creates player with auto-responses to vulnerable oppt moves. 
        Executes in the following order:
        
        pre_search():
            1) VulPreCheck

        post_search():
        
    */
    UofAPlayer* CreateTheoryPlayer(UofAPlayer* player);

} // namespace PlayerFactory

//----------------------------------------------------------------------------

#endif // PLAYERFACTORY_HPP
