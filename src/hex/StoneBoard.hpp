//----------------------------------------------------------------------------
// $Id: StoneBoard.hpp 1657 2008-09-15 23:32:09Z broderic $
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
    contribute to the board hash, unplayed stones do not.     
    @see Hash(), setPlayed(). 
    
    @note you MUST call StoneBoard::startNewGame() before playing any
    moves.
*/
class StoneBoard : public ConstBoard
{
public:

    /** Constructs a square board. */
    explicit StoneBoard(unsigned size);

    /** Constructs a rectangular board. */
    StoneBoard(unsigned width, unsigned height);

    /** Destructor. */
    virtual ~StoneBoard();

    //-----------------------------------------------------------------------

    /** Returns zobrist hash for the current board state, but this
        hash depends only on played cells; unplayed cells do not
        contribute to the hash. Changing the color of an unplayed cell
        does not change the hash for the state. Methods that change
        the color of played cells will always compute a new hash for
        the state.

        @see setPlayed()
    */
    hash_t Hash() const;

    /** Returns the BoardID for the current board state, looking only
        at the played cells. The second method converts the BoardID to a
	std::string and returns that instead (for printing & hand book).
    */
    BoardID GetBoardID() const;
    std::string GetBoardIDString() const;

    /** Sets the board to the state encoded by id. */
    void SetState(const BoardID& id);

    //-----------------------------------------------------------------------

    /** @name Methods defined on cells and edges. 
    
        All methods accept and return edge and interior cells only.
        Note that playMove() can play a move like (BLACK, RESIGN), but
        getBlack() will not return a bitset with the RESIGN move set.
    */
    // @{

    /** Returns the set of BLACK stones. */
    bitset_t getBlack() const;
    
    /** Returns the set of WHITE stones. */
    bitset_t getWhite() const;

    bitset_t getColor(HexColor color) const;

    bitset_t getEmpty() const;
    bitset_t getOccupied() const;
    bool isColor(HexPoint cell, HexColor color) const;
    bool isEmpty(HexPoint cell) const;
    bool isOccupied(HexPoint cell) const;

    // @}

    /** @name Methods defined on all valid moves. */
    // @{ 

    bool isBlack(HexPoint cell) const;

    bool isWhite(HexPoint cell) const;

    HexColor getColor(HexPoint cell) const;

    /** Returns the set of played cells. */
    bitset_t getPlayed() const;

    /** Returns the set of all legal moves. */
    bitset_t getLegal() const;

    /** Returns true if cell has been played. */
    bool isPlayed(HexPoint cell) const;

    /** Returns true if cell is a legal move. */
    bool isLegal(HexPoint cell) const;

    // @}
    
    /** Number of played stones on the interior of the board. 
        Similar to:
        @code
            num bits = (getOccupied() & getPlayed() & getCells()).count();
        @endcode
    */
    int numStones() const;

    /** Computes whose turn it is on the given board, assuming FIRST_TO_PLAY
        moves first and alternating play. 
    */
    HexColor WhoseTurn() const;

    /** Two boards are equal if their dimensions match and their sets
        of black, white, and played stones are all equal. */
    bool operator==(const StoneBoard& other) const;

    /** Returns true if the boards differ. See operator==().  */
    bool operator!=(const StoneBoard& other) const;

    //-----------------------------------------------------------------------

    /** Returns a BoardIterator iterating over all stones on the board
        belonging to the given colorset. */
    const BoardIterator& Stones(HexColorSet colorset) const;
    const BoardIterator& Stones(HexColor color) const;

    //-----------------------------------------------------------------------

    /** Adds the set cells of cells in b as stones of color
        color. Does not modify hash. */
    void addColor(HexColor color, const bitset_t& b);

    /** Removes the set of cells in b from the set of color's
        stones. Does not modify hash.  */
    void removeColor(HexColor color, const bitset_t& b);

    /** Sets the color of the given cell/bitset cells. Does not modify
        hash. */
    virtual void setColor(HexColor color, HexPoint cell);
    virtual void setColor(HexColor color, const bitset_t& bs);
    
    /** Sets the set of played stones.  These stones, and only these
        stones, will contribute to the board hash. Hash is recomputed.
        @see Hash(). */
    void setPlayed(const bitset_t& p);

    //-----------------------------------------------------------------------

    /** Clears the board and plays the edge stones.  This method MUST
        be called before any user moves can be played. */
    virtual void startNewGame();

    /** Plays a move of the given color to the board. Adds cell to the
        set of played stones. Updates the board hash. 
        @param color must BLACK or WHITE 
        @param cell Any valid move, include RESIGN and SWAP_PIECES.
    */
    void playMove(HexColor color, HexPoint cell);

    /** Removes the move from the board, setting cell to EMPTY. Hash
        is updated. */
    void undoMove(HexPoint cell);

    /** Rotates the board by 180' about the center. Hash is updated*/
    void rotateBoard();

    /** Mirrors the board in the diagonal joining acute corners. Note
	that this method requires the board to be square. Hash is
	updated. */
    void mirrorBoard();

    //-----------------------------------------------------------------------

    /** Returns a string representation of the board. */
    virtual std::string print() const;

    /** Returns a string representation of the board with the cells
        marked in the given bitset denoted by a '*'. */
    virtual std::string printBitset(const bitset_t& b) const;

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

    bitset_t m_played;
    bitset_t m_stones[BLACK_AND_WHITE];

    mutable bool m_stones_calculated;
    mutable std::vector<HexPoint> m_stones_list[NUM_COLOR_SETS];
    mutable BoardIterator m_stones_iter[NUM_COLOR_SETS];

    /** Hash of the current state. This is a hash only of the moves
        played, and so does not include fillin stones. 
        
        @see Hash()
    */
    ZobristHash m_hash;
};

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
