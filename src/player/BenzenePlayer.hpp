//----------------------------------------------------------------------------
// $Id: BenzenePlayer.hpp 1877 2009-01-29 00:57:27Z broderic $
//----------------------------------------------------------------------------

#ifndef BENZENEPLAYER_HPP
#define BENZENEPLAYER_HPP

#include "ICEngine.hpp"
#include "HexBoard.hpp"
#include "HexEval.hpp"
#include "HexPlayer.hpp"

//----------------------------------------------------------------------------

/** Base class for all UofA players. */
class BenzenePlayer: public HexPlayer
{
public:

    /** Constructs a player with the given ICEngine. */
    explicit BenzenePlayer();

    /** Destructor. */
    virtual ~BenzenePlayer();

    /** Generates a move from this board position. If the game is
        already over (somebody has won), returns RESIGN.

        Derived UofA players that use different search algorithms
        should not extend this method, but the protected virtual
        method search() below.

        Classes deriving from UofAFunctinality should extend the 
        pre_search() and post_search() methods only.  
        @see BenzenePlayerFunctionality.

        This method does the following:

        1 - if state is terminal (game over, vc/fill-in win/loss),
            returns "appropriate" move. Otherwise, continues to 
            step 2. 
        2 - Calls pre_search().
            If pre_search() returns INVALID_POINT, continues to 
            step 3. Otherwise, returns point returned by pre_search().
        3 - calls search() 
        4 - calls post_search() with move returned by search(). 
        5 - returns move returned by post_search(). 
       

        @param brd HexBoard to do work on. Board position is set to
               the board position as that of the game board. 
        @param game_state Game history up to this position.
        @param color Color to move in this position.
        @param time_remaining Time in minutes remaining in game.
        @param score Return score of move here. 
    */
    HexPoint genmove(HexBoard& brd, 
                     const Game& game_state, HexColor color,
                     double time_remaining, double& score);

    //----------------------------------------------------------------------

    /** Performs various checks before the actual search. An example
        usage of this method would be to check an opening book for the
        current state and to abort the call to search() if found.
        
        If pre_search() is successful, the genmove() algorithm returns
        the move pre_search() returns. If unsuccessfull, search() is
        called. Default implementation does nothing.
        
        @param consider Moves to consider in this state. Can be
        modified. Passed into search().
                     
        @return INVALID_POINT on failure, otherwise a valid move on
        success.
    */
    virtual HexPoint pre_search(HexBoard& brd, const Game& game_state,
				HexColor color, bitset_t& consider,
                                double time_remaining, double& score);

    /** Generates a move in the given gamestate.  Derived classes
        should extend this method. Score can be stored in score.
        
        @param consider Moves to consider in this state. 
        @return The move to play.
    */
    virtual HexPoint search(HexBoard& brd, const Game& game_state,
			    HexColor color, const bitset_t& consider,
                            double time_remaining, double& score);
    
    /** This method performs post processing on the move returned by
        search().  An example usage might be to check that the move
        returned is not dominated, and if it is, return the killer
        instead.  Default implementation does nothing.

        @param move The move returned by search(). 
        @return The modified move that will be played instead.  
    */
    virtual HexPoint post_search(HexPoint move, HexBoard& brd, 
                                 HexColor color, double time_remaining, 
                                 double& score);

private:

    /** Finds inferior cells, builds vcs. Sets moves to consider to
        all empty cells. 
        
        @return INVALID_POINT if a non-terminal state, otherwise the
        move to play in the terminal state.
    */
    HexPoint init_search(HexBoard& brd, HexColor color, 
                         bitset_t& consider,
                         double time_remaining, double& score);

};

//----------------------------------------------------------------------------

/** Abstract base class for classes adding functionality to
    BenzenePlayer. */
class BenzenePlayerFunctionality : public BenzenePlayer
{
public:

    /** Constructor.
        @param player The player to extend. 
    */
    explicit BenzenePlayerFunctionality(BenzenePlayer* player);

    /** Destructor. */
    virtual ~BenzenePlayerFunctionality();

    /** Returns pointer the player it is extending. */
    BenzenePlayer* PlayerExtending();

    /** Returns name of player it is extending. */
    std::string name() const;

    /** Extends BenzenePlayer::pre_search(). If this implementation
        fails, pre_search() should call pre_search() of player it is
        extending.  In this way, multiple functionalities can be
        chained together.  If you want to simply constrain the moves
        to consider, alter it, and return player->pre_search().
    */
    virtual HexPoint pre_search(HexBoard& brd, const Game& game_state,
				HexColor color, bitset_t& consider,
                                double time_remaining, double& score);
    
    /** Extends BenzenePlayer::post_search(). If this implementation
        fails, postsearch() should call post_search() of player it is
        extending.  In this way, multiple functionalities can be
        chained together.
    */
    virtual HexPoint post_search(HexPoint move, HexBoard& brd, 
                                 HexColor color, double time_remaining, 
                                 double& score);
    
protected:

    /** Calls search() method of player it is extending. */
    HexPoint search(HexBoard& brd, const Game& game_state,
		    HexColor color, const bitset_t& consider,
                    double time_remaining, double& score);

    BenzenePlayer* m_player;
};

inline 
BenzenePlayerFunctionality::BenzenePlayerFunctionality(BenzenePlayer* player)
    : m_player(player)
{
}

inline BenzenePlayerFunctionality::~BenzenePlayerFunctionality()
{
}

inline BenzenePlayer* BenzenePlayerFunctionality::PlayerExtending()
{
    return m_player;
}

inline std::string BenzenePlayerFunctionality::name() const
{
    return m_player->name();
}

inline HexPoint 
BenzenePlayerFunctionality::pre_search(HexBoard& UNUSED(brd), 
                                       const Game& UNUSED(game_state),
                                       HexColor UNUSED(color), 
                                       bitset_t& UNUSED(consider),
                                       double UNUSED(time_remaining), 
                                       double& UNUSED(score))
{
    return INVALID_POINT;
}

inline HexPoint 
BenzenePlayerFunctionality::search(HexBoard& brd, const Game& game_state,
                                   HexColor color, const bitset_t& consider,
                                   double time_remaining, double& score)
{
    return m_player->search(brd, game_state, color, consider,
			    time_remaining, score);
}

inline HexPoint 
BenzenePlayerFunctionality::post_search(HexPoint move, 
                                        HexBoard& UNUSED(brd), 
                                        HexColor UNUSED(color), 
                                        double UNUSED(time_remaining), 
                                        double& UNUSED(score))
{
    return move;
}

//----------------------------------------------------------------------------

#endif // BENZENEPLAYER_HPP
