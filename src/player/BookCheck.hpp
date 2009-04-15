//----------------------------------------------------------------------------
// $Id: BookCheck.hpp 1536 2008-07-09 22:47:27Z broderic $
//----------------------------------------------------------------------------

#ifndef BOOKCHECK_HPP
#define BOOKCHECK_HPP

#include "UofAPlayer.hpp"
#include "OpeningBook.hpp"

//----------------------------------------------------------------------------

/** Checks book before search. */
class BookCheck : public UofAPlayerFunctionality
{
public:

    /** Adds book check to the given player. */
    BookCheck(UofAPlayer* player);

    /** Destructor. */
    virtual ~BookCheck();

    /** Checks the book for the current state if "player-use-book" is
        true. If state found in book, returns book move. Otherwise
        calls player's pre_search() and returns the move it returns.
    */
    virtual HexPoint pre_search(HexBoard& brd, const Game& game_state,
				HexColor color, bitset_t& consider,
                                double time_remaining, double& score);
private:

    //----------------------------------------------------------------------

    /** Stores the names and alpha-values of all opening books. */
    void LoadOpeningBooks();
    
    /** Uses the opening books to determine a response (if possible).
        @return INVALID_POINT on failure, valid move on success.
    */
    HexPoint BookResponse(HexBoard& brd, HexColor color);
    
    void ComputeBestChild(StoneBoard& brd, HexColor color,
			  const OpeningBook& book, HexPoint& move,
			  float& score);
    
    std::vector<std::string> m_openingBookFiles;
    std::vector<double> m_openingBookAlphas;
    float m_openingBookDepthAdjustment;
    bool m_openingBookLoaded;
    int m_openingBookMinDepth;
};

//----------------------------------------------------------------------------

#endif // BOOKCHECK_HPP
