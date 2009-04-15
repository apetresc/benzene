//----------------------------------------------------------------------------
// $Id: Pattern.hpp 1536 2008-07-09 22:47:27Z broderic $
//----------------------------------------------------------------------------

#ifndef PATTERN_H
#define PATTERN_H

#include "Hex.hpp"
#include "RingGodel.hpp"

//----------------------------------------------------------------------------

/** @page patternencoding Pattern Encoding

    Each pattern is a type, followed by a colon, followed by six
    slices, followed by an optional weight (the weight parameter is
    used only for certain types of patterns).
    
    @verbatim

       pattern = type : slice; slice; slice; slice; slice; slice; weight
    @endverbatim

    The six slices form a fan around the center cell. If the pattern
    is rotated 60 degrees, the first slice will map onto the second
    slice, the second onto the third, etc.  This allows the patterns
    to be easily rotated on the hex board.

    Each slice extends out by MAX_EXTENSION cells. If MAX_EXTENSION=7,
    then the slices would be laid out like this:

    @verbatim
                                  | 
                                  |   slice 1     27  
                      slice 2     |            20 26
                                  |         14 19 25
                                  |       9 13 18 24
                                  |     5 8 12 17 23 <-- slice 0
                                      2 4 7 11 16 22
                 21 15 10 6 3 1 0 * 0 1 3 6 10 15 21
                 22 16 11 7 4 2  
     slice 3 --> 23 17 12 8 5    |
                 24 18 13 9      |   
                 25 19 14        |     slice 5
                 26 20           |
                 27     slice 4  |
                                 |
    @endverbatim
    
    Each slice is composed of five comma separated features. 

    @verbatim

       slice = feature, feature, feature, feature, feature
    @endverbatim

    Each feature is a 32-bit integer used as a bitmask where the set
    bits denote cells int which that feature is "on".

    - CELLS the cells used in the slice.  
    - BLACK the black stones in the slice.
    - WHITE the white stones in the slice. 
    - MARKED1 first set of marked cells in the slice. 
    - MARKED1 second set of marked cells in the slice.

    All features must be a subset of CELLS. BLACK, WHITE, MARKED1 and
    MARKED2 must all be pairwise disjoint.

    For example, let s be a slice in which CELLS=7, BLACK=4, WHITE=1,
    MARKED1=0, and MARKED2=0. Then this slice uses cells 0, 1 and 2;
    cell 0 contains a white stone, cell 1 is empty, and cell 2
    contains a black stone.
*/

/** Patterns on a Hex board. 

    Patterns are centered around a cell, and are encoded such that they
    can be rotated with minimal computation.  

    These patterns can only be detected on a PatternBoard. 

    Used by:
       - ICEngine to find inferior cells.  
       - UCT during the random playout phase. 

    @see @ref patternencoding
*/
class Pattern
{
public:

    /** This sets how far out patterns are allowed to extend. Value
        must be >= 1 and <= 7. */
    static const int MAX_EXTENSION = 3;

    //-----------------------------------------------------------------------

    /** Pattern encodes a move. */
    static const int HAS_MOVES1    = 0x01;
    static const int HAS_MOVES2    = 0x02;

    /** Pattern has a weight (used by MOHEX patterns). */
    static const int HAS_WEIGHT    = 0x04;

    //-----------------------------------------------------------------------

    /** @name Pattern Types.
        The pattern type typically denotes the status of the cell at
        the center of the pattern.
    */

    // @{

    /** Unknown type. Set in Pattern(), but should not appear in a
        defined pattern. */
    static const char UNKNOWN = ' ';
    
    /** Marks that the cell the pattern is centered on is dead. */
    static const char DEAD = 'd';

    /** Marks that the cell the pattern is centered on is
        captured. Captured patterns denote a strategy to make this
        cell and any cells in MARKED1 as captured.
    */
    static const char CAPTURED = 'c';

    /** Marks a permanently inferior cell. MARKED1 holds its
        carrier. */
    static const char PERMANENTLY_INFERIOR = 'p';

    /** Marks a dominated cell. MARKED1 holds its killer. */
    static const char DOMINATED = '!';

    /** Marks a vulnerable cell. MARKED1 holds its killer, and MARKED2
        holds its carrier. */
    static const char VULNERABLE = 'v';

    /** A mohex pattern.  These patterns are used during the random
        playout phase of an UCT search. */
    static const char MOHEX = 'm';

    /** A mohex pattern. These patterns are used during the random
        playout phase of an UCT search. */
    static const char SHIFT = 's';

    // @} // @name

    //-----------------------------------------------------------------------

    /** Number of triangular slices.  Each slice is rooted at a
        neighbour of the center cell.  Should be 6 (one for each
        direction in HexDirection). */
    static const int NUM_SLICES = 6;

    /** Info stored in each slice. */
    static const int FEATURE_CELLS   = 0;
    static const int FEATURE_BLACK   = 1;
    static const int FEATURE_WHITE   = 2;
    static const int FEATURE_MARKED1 = 3;
    static const int FEATURE_MARKED2 = 4;
    static const int NUM_FEATURES    = 5;

    /** A slice is simply an array of features. */
    typedef int slice_t[NUM_FEATURES];

    //------------------------------------------------------------

    /** Creates an empty pattern. Type is set to UNKNOWN. */
    Pattern();

    /** Returns a string of the pattern in encoded form. 
        @see @ref patternencoding
    */
    std::string serialize() const;

    /** Parses a pattern from an encoded string. 
        @see @ref patternencoding

        @return true if pattern was parsed without error.
    */
    bool unserialize(std::string code);
    
    /** Returns the pattern's flags. */
    int getFlags() const;

    /** Returns the pattern's name. */
    std::string getName() const;

    /** Sets the name of this pattern. */
    void setName(const std::string& s);

    /** Returns the pattern's type. */
    char getType() const;
    
    /** Returns the list of (slice, bit) pairs for moves defined in the
        marked field (f = 0 or 1). */  
    const std::vector<std::pair<int, int> >& getMoves1() const;
    const std::vector<std::pair<int, int> >& getMoves2() const;

    /** Gets the weight for this pattern. */
    int getWeight() const;

    /** Gets the extension radius of this pattern. */
    int extension() const;

    /** Returns pointer to pattern's slice data. */
    const slice_t* getData() const;

    /** Returns the ring godel of this pattern rotated
        by angle slices. */
    const PatternRingGodel& RingGodel(int angle) const;

    /** Flip the pattern's colors. */
    void flipColors();

    /** Mirrors pattern along x/y diagonal. */
    void mirror(); 

    /** Parses patterns from a given file.

        Pattern names are assumed to come before the encoding and are
        between '[' and '/' characters (this comes from Jack's
        pattern file format).

        A mirrored copy of a pattern is stored if two names are
        encountered before the pattern string.  No checking is done
        to determine if a mirror is really necessary.

        The pattern encoding is detected by any character in the first
        column and is assumed to occupy exactly a single line.

        So, to create your own pattern file, use something like this:

        |  ...
        |
        | [name1/]
        | [name2/]
        | pattern encoding;
        |
        |  ...

        Notice the names are between [ and / symbols and do not start
        in the first column.  The pattern encoding, however, does
        start in the first column (and is the ONLY thing starting in the 
        first column).

        This is temporary. :-) 

        If you want, you can add a picture of the pattern.  Here is 
        an example pattern from our pattern file:


        |
        |              B B   
        |             B * !                         [31/0]
        |   
        |             B * ! 
        |              B B                          [31m/0]
        |   
        |!:1,0,0,1;1,1,0,0;1,1,0,0;1,1,0,0;0,0,0,0;0,0,0,0;
        |


        This defines a pattern with name "31" and its mirror "31m".  
    */ 
    static void LoadPatternsFromFile(const char *filename,
                                     std::vector<Pattern>& out);

private:

    /** Compute the ring godel codes. */
    void compute_ring_godel();

    /** Pattern type. */
    char m_type;

    /** Name of the pattern. */
    std::string m_name;

    /** Flags. */
    int m_flags;

    /** (slice, bit) pairs of cells in FEATURE_MARKED1. */
    std::vector<std::pair<int, int> > m_moves1;

    /** (slice, bit) pairs of cells in FEATURE_MARKED2. */
    std::vector<std::pair<int, int> > m_moves2;

    /** MoHex pattern weight. */
    int m_weight;

    /** Data for each slice. */
    slice_t m_slice[NUM_SLICES];

    /** How far out the pattern extends. */
    int m_extension;

    /** One RingGodel for each rotation of the pattern. */
    PatternRingGodel m_ring_godel[NUM_SLICES];
};

/** Sends output of serialize() to the stream. */
inline std::ostream& operator<<(std::ostream &os, const Pattern& p)
{
    os << p.serialize();
    return os;
}

inline std::string Pattern::getName() const
{
    return m_name;
}

inline int Pattern::getFlags() const
{
    return m_flags;
}

inline char Pattern::getType() const
{
    return m_type;
}

inline const Pattern::slice_t* Pattern::getData() const
{
    return m_slice;
}

inline void Pattern::setName(const std::string& s) 
{
    m_name = s; 
}

inline const std::vector<std::pair<int, int> >& Pattern::getMoves1() const
{
    HexAssert(m_flags & HAS_MOVES1);
    return m_moves1;
}

inline const std::vector<std::pair<int, int> >& Pattern::getMoves2() const
{
    HexAssert(m_flags & HAS_MOVES2);
    return m_moves2;
}

inline int Pattern::getWeight() const
{
    HexAssert(m_flags & HAS_WEIGHT);
    return m_weight;
}

inline int Pattern::extension() const
{
    return m_extension;
}

inline const PatternRingGodel& Pattern::RingGodel(int angle) const
{
    return m_ring_godel[angle];
}

//----------------------------------------------------------------------------

/** Vector of patterns. */
typedef std::vector<Pattern> PatternSet;

//----------------------------------------------------------------------------

/** Utilities on Patterns. */
namespace PatternUtil
{
    /** Computes how far out this godel code extends from the center point
        of the pattern. */
    int GetExtensionFromGodel(int godel);
}

//----------------------------------------------------------------------------

/** A (pattern, angle) pair. */
class RotatedPattern
{
public:
    RotatedPattern(const Pattern* pat, int angle);
    const Pattern* pattern() const;
    int angle() const;

private:
    const Pattern* m_pattern;
    int m_angle;
};

inline RotatedPattern::RotatedPattern(const Pattern* pat, int angle)
    : m_pattern(pat), 
      m_angle(angle)
{
}

inline const Pattern* RotatedPattern::pattern() const
{
    return m_pattern;
}

inline int RotatedPattern::angle() const
{
    return m_angle;
}

//----------------------------------------------------------------------------

/** List of RotatedPatterns. */
typedef std::vector<RotatedPattern> RotatedPatternList;

//----------------------------------------------------------------------------

#endif // PATTERN_H
