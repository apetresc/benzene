//----------------------------------------------------------------------------
// $Id: StoneBoard.hpp 1786 2008-12-14 01:55:45Z broderic $
//----------------------------------------------------------------------------

#ifndef STONEBOARD_H
#define STONEBOARD_H

#include "Hex.hpp"
#include "ConstBoard.hpp"
#include "ZobristHash.hpp"

//---------------------------------------------------------------------------

/** Packed representation of a board-stat. Useful for storing
    board positions in databases, opening books, etc. */
typedef std::vector<byte> BoardID;

//---------------------------------------------------------------------------

/** Tracks played stone information.

    Each cell on the board is assigned a HexColor, and so every cell
    is either EMPTY, BLACK, or WHITE.

    Each cell is also marked as 'played' or 'unplayed'. A cell should
    be marked as played when it corresponds to an actual move played
    in a game; that is, not a fill-in move. This means it is possible
    for a cell to be BLACK or WHITE and not played.  Played stones
    contribute to the board hash and id, unplayed stones do not.     
    @see Hash(), GetBoardID(), setPlayed(). 
    
    @note You MUST call StoneBoard::startNewGame() before playing any
    moves.
*/
class StoneBoard
{
public:

    /** Constructs a square board. */
    explicit StoneBoard(unsigned size);

    /** Constructs a rectangular board. */
    StoneBoard(unsigned width, unsigned height);

    /** Destructor. */
    virtual ~StoneBoard();

    //-----------------------------------------------------------------------

    /** Returns reference to ConstBoard. */
    const ConstBoard& Const() const;

    /** Same as Const().width() */
    int width() const;

    /** Same as Const().height() */
    int height() const;

    /** Returns zobrist hash for the current board state, which
        depends only on played cells; unplayed cells do not contribute
        to the hash. Changing the color of an unplayed cell does not
        change the hash for the state. Methods that change the color
        of played cells internally will always compute a new hash
        automatically. */
    hash_t Hash() const;

    /** Returns BoardID for the current board state, looking only at
        the played cells. */
    BoardID GetBoardID() const;

    /** Returns BoardID as a string. 
        @todo Moves this out of here? */
    std::string GetBoardIDString() const;

    /** Sets the board to the state encoded by id. 
        Note this state will have no unplayed stones, so the code:
        @code
            brd.SetState(brd.GetBoardID());
        @endcode
        will remove all unplayed stones.
    */
    void SetState(const BoardID& id);

    /** Number of played stones on the interior of the board. 
        Similar to:
        @code
            num bits = (getOccupied() & getPlayed() & getCells()).count();
        @endcode
    */
    int numStones() const;

    /** Computes whose turn it is on the given board, assuming FIRST_TO_PLAY
        moves first and alternating play. */
    HexColor WhoseTurn() const;

    /** Returns a string representation of the board. */
    virtual std::string print() const;

    /** Returns a string representation of the board with the cells
        marked in the given bitset denoted by a '*'. */
    virtual std::string printBitset(const bitset_t& b) const;

    //-----------------------------------------------------------------------

    /** @name Methods defined on cells and edges. 
    
        All methods accept and return edge and interior cells only.
        Note that playMove() (which is not in this section) can play a
        move like (BLACK, RESIGN), but getBlack() (which is in this
        section) will not return a bitset with the RESIGN move set.

        @todo Why did we agree on this? :)
    */
    // @{

    /** Returns a bitset with all valid board cells. 
        Shorthand for Const().getCells(). */
    bitset_t getCells() const;

    /** Returns a bitset with all valid board locations (cells and
	edges). Same as Const().getLocations(). */
    bitset_t getLocations() const;

    /** Returns true if cell is a valid cell on this board. Same as
        Const().isCell(). */
    bool isCell(HexPoint cell) const;

    /** Returns true if bs encodes a set of valid cells. Same as
        Const().isCell(). */
    bool isCell(const bitset_t& bs) const;
    
    /** Returns true if cell is a location on this board. Same as
        Const().isLocation(). */
    bool isLocation(HexPoint cell) const;

    /** Returns true if bs encodes a set of valid locations. Same as
        Const().isLocation(). */
    bool isLocation(const bitset_t& bs) const;
    
    /** Returns the set of BLACK stones. */
    bitset_t getBlack() const;
    
    /** Returns the set of WHITE stones. */
    bitset_t getWhite() const;

    /** Returns color's stones. */
    bitset_t getColor(HexColor color) const;

    /** Returns all empty cells. */
    bitset_t getEmpty() const;

    /** Returns all occupied (not empty) cells. */
    bitset_t getOccupied() const;

    /** Returns true if cell is of color. */
    bool isColor(HexPoint cell, HexColor color) const;

    /** Returns true if cell is empty. */
    bool isEmpty(HexPoint cell) const;

    /** Returns true if cell is occupied (not empty). */
    bool isOccupied(HexPoint cell) const;

    /** Returns true if p1 is adjacent to p2. Iterates over neighbour
        list of p1, so not O(1). Same as Const().Adjacent(). */
    bool Adjacent(HexPoint p1, HexPoint p2) const;

    /** Returns the distance between two valid HexPoints. Same as
        Const().Adjacent(). */
    int Distance(HexPoint x, HexPoint y) const;

    // @}

    //-----------------------------------------------------------------------

    /** @name Methods defined on all valid moves. */
    // @{ 

    /** Returns a bitset of cells comprising all valid moves (this
	includes swap and resign). Same as Const().getValid(). */
    bitset_t getValid() const;

    /** Returns true if cell is a valid move on this board. Same as
        Const().isValid(). Note that occupied cells will be reported
        as valid, but they are not legal. */
    bool isValid(HexPoint cell) const;

    /** Returns true if bs encodes a set of valid moves. Same as
        Const().isValid(). Note that occupied cells will be reported
        as valid, but they are not legal. */
    bool isValid(const bitset_t& bs) const;

    /** Returns true if cell is BLACK. */
    bool isBlack(HexPoint cell) const;

    /** Returns true if cell is WHITE. */
    bool isWhite(HexPoint cell) const;

    /** Retruns color of cell. */
    HexColor getColor(HexPoint cell) const;

    /** Returns the set of played cells. */
    bitset_t getPlayed() const;

    /** Returns the set of all legal moves; ie, moves that can be
        played from this state. */
    bitset_t getLegal() const;

    /** Returns true if cell has been played. */
    bool isPlayed(HexPoint cell) const;

    /** Returns true if cell is a legal move. */
    bool isLegal(HexPoint cell) const;

    // @}
    
    //-----------------------------------------------------------------------

    /** @name Methods not modifying Hash() or BoardID() */
    // @{

    /** Adds the cells in b as stones of color. Does not modify
        hash. */
    void addColor(HexColor color, const bitset_t& b);

    /** Sets cells in b to EMPTY. Does not modify hash.  */
    void removeColor(HexColor color, const bitset_t& b);

    /** Sets color of cell. Does not modify hash. */
    virtual void setColor(HexColor color, HexPoint cell);

    /** Sets color of cells in bitset. Does not modify hash. */
    virtual void setColor(HexColor color, const bitset_t& bs);
    
    // @}

    //-----------------------------------------------------------------------

    /** @name Methods modifying Hash() and BoardID() */
    // @{

    /** Clears the board and plays the edge stones. This method MUST
        be called before any user moves can be played. */
    virtual void startNewGame();

    /** Sets the played stones. These stones, and only these stones,
        will contribute to the board hash and board id. Hash is
        recomputed.  @see Hash(). */
    void setPlayed(const bitset_t& p);

    /** Plays a move of the given color to the board. Adds cell to the
        set of played stones. Updates the board hash. 
        @param color must BLACK or WHITE 
        @param cell Any valid move, include RESIGN and SWAP_PIECES.
    */
    void playMove(HexColor color, HexPoint cell);

    /** Removes the move from the board, setting cell to EMPTY. Hash
        is updated. */
    void undoMove(HexPoint cell);

    /** Rotates the board by 180' about the center. Hash is
        updated. */
    void rotateBoard();

    /** Mirrors the board in the diagonal joining acute corners. Note
	that this method requires the board to be square. Hash is
	updated. */
    void mirrorBoard();

    // @}

    //-----------------------------------------------------------------------

    /** @name Operators */
    // @{

    /** Two boards are equal if their dimensions match and their sets
        of black, white, and played stones are all equal. */
    bool operator==(const StoneBoard& other) const;

    /** Returns true if the boards differ. See operator==().  */
    bool operator!=(const StoneBoard& other) const;

    // @}

    //-----------------------------------------------------------------------

    /** @name Iterators */
    // @{

    /** Returns iterator to the interior board cells. */
    BoardIterator Interior() const;
    
    /** Returns iterator to the board cells, starting on the outer
        edges. */
    BoardIterator EdgesAndInterior() const;

    /** Returns iterator that runs over all valid moves. */
    BoardIterator AllValid() const;

    /** Returns iterator to the first neighbour of cell. 
        @see @ref cellneighbours */
    BoardIterator ConstNbs(HexPoint cell) const;

    /** Returns iterator to the neighbourhood extending outward by
        radius cells of cell. */
    BoardIterator ConstNbs(HexPoint cell, int radius) const;

    /** Returns iterator over all stones in colorset. */
    const BoardIterator& Stones(HexColorSet colorset) const;

    /** Returns iterator over all stones of color. */

    const BoardIterator& Stones(HexColor color) const;

    // @}

    //-----------------------------------------------------------------------

protected:

    /** Removes all stones from the board. Derived classes should
        extend this method if needed. */
    virtual void clear();

    /** Notifies derived classes that the board was modified. */
    virtual void modified();

private:

    void init();

    void computeHash();

    bool BlackWhiteDisjoint();

    //----------------------------------------------------------------------

    ConstBoard* m_const;

    bitset_t m_played;

    bitset_t m_stones[BLACK_AND_WHITE];

    mutable bool m_stones_calculated;

    mutable std::vector<HexPoint> m_stones_list[NUM_COLOR_SETS];

    mutable BoardIterator m_stones_iter[NUM_COLOR_SETS];

    /** @see Hash() */
    ZobristHash m_hash;
};

inline const ConstBoard& StoneBoard::Const() const
{
    return *m_const;
}

inline int StoneBoard::width() const
{
    return m_const->width();
}

inline int StoneBoard::height() const
{
    return m_const->height();
}

inline bitset_t StoneBoard::getCells() const
{
    return m_const->getCells();
}

inline bitset_t StoneBoard::getLocations() const
{
    return m_const->getLocations();
}

inline bitset_t StoneBoard::getValid() const
{
    return m_const->getValid();
}

inline bool StoneBoard::isCell(HexPoint cell) const
{
    return m_const->isCell(cell);
}

inline bool StoneBoard::isCell(const bitset_t& bs) const
{
    return m_const->isCell(bs);
}

inline bool StoneBoard::isLocation(HexPoint cell) const
{
    return m_const->isLocation(cell);
}

inline bool StoneBoard::isLocation(const bitset_t& bs) const
{
    return m_const->isLocation(bs);
}

inline bool StoneBoard::isValid(HexPoint cell) const
{
    return m_const->isValid(cell);
}

inline bool StoneBoard::isValid(const bitset_t& bs) const
{
    return m_const->isValid(bs);
}

inline bool StoneBoard::Adjacent(HexPoint p1, HexPoint p2) const
{
    return m_const->Adjacent(p1, p2);
}

inline int StoneBoard::Distance(HexPoint x, HexPoint y) const
{
    return m_const->Distance(x, y);
}

inline BoardIterator StoneBoard::Interior() const
{
    return m_const->Interior();
}

inline BoardIterator StoneBoard::EdgesAndInterior() const
{ 
    return m_const->EdgesAndInterior();
}

inline BoardIterator StoneBoard::AllValid() const
{ 
    return m_const->AllValid();
}

inline BoardIterator StoneBoard::ConstNbs(HexPoint cell) const
{
    return m_const->ConstNbs(cell);
}

inline BoardIterator StoneBoard::ConstNbs(HexPoint cell, int radius) const
{
    return m_const->ConstNbs(cell, radius);
}

inline hash_t StoneBoard::Hash() const
{
    return m_hash.hash();
}

inline bitset_t StoneBoard::getBlack() const
{
    return m_stones[BLACK] & getLocations();
}

inline bitset_t StoneBoard::getWhite() const
{
    return m_stones[WHITE] & getLocations();
}

inline bitset_t StoneBoard::getColor(HexColor color) const
{
    HexAssert(HexColorUtil::isValidColor(color));
    if (color == EMPTY) return getEmpty();
    return m_stones[color] & getLocations();
}

inline bitset_t StoneBoard::getEmpty() const
{
    return getLocations() - getOccupied();
}

inline bitset_t StoneBoard::getOccupied() const
{
    return (getBlack() | getWhite()) & getLocations();
}

inline bool StoneBoard::isBlack(HexPoint cell) const    
{
    HexAssert(isValid(cell));
    return m_stones[BLACK].test(cell);
}

inline bool StoneBoard::isWhite(HexPoint cell) const    
{
    HexAssert(isValid(cell));
    return m_stones[WHITE].test(cell);
}

inline bool StoneBoard::isColor(HexPoint cell, HexColor color) const
{
    HexAssert(HexColorUtil::isBlackWhite(color));
    HexAssert(isLocation(cell));
    return m_stones[color].test(cell);
}

inline bool StoneBoard::isEmpty(HexPoint cell) const
{
    HexAssert(isLocation(cell));
    return !isOccupied(cell);
}

inline bool StoneBoard::isOccupied(HexPoint cell) const
{
    HexAssert(isLocation(cell));
    return (isBlack(cell) || isWhite(cell));
}

inline bitset_t StoneBoard::getPlayed() const 
{
    return m_played;
}

inline bool StoneBoard::isPlayed(HexPoint cell) const
{
    HexAssert(isValid(cell));
    return m_played.test(cell);
}

inline int StoneBoard::numStones() const
{
    return (getOccupied() & getPlayed() & getCells()).count();
}

inline bool StoneBoard::operator==(const StoneBoard& other) const
{
    return (width() == other.width() && 
            height() == other.height() && 
            m_stones[BLACK] == other.m_stones[BLACK] && 
            m_stones[WHITE] == other.m_stones[WHITE] && 
            m_played == other.m_played);
}

inline bool StoneBoard::operator!=(const StoneBoard& other) const
{
    return !(operator==(other));
}

inline HexColor StoneBoard::WhoseTurn() const
{
    int first = (getColor(FIRST_TO_PLAY) & getPlayed() & getCells()).count();
    int second = (getColor(!FIRST_TO_PLAY) & getPlayed() & getCells()).count();
    return (first > second) ? !FIRST_TO_PLAY : FIRST_TO_PLAY;
}

inline const BoardIterator& StoneBoard::Stones(HexColor color) const
{
    return Stones(HexColorSetUtil::Only(color));
}

/** Prints board to output stream. */
inline std::ostream& operator<<(std::ostream &os, const StoneBoard& b)
{
    os << b.print();
    return os;
}

//----------------------------------------------------------------------------

#endif // STONEBOARD_HPP
