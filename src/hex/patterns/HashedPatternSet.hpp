//----------------------------------------------------------------------------
// $Id: HashedPatternSet.hpp 1536 2008-07-09 22:47:27Z broderic $
//----------------------------------------------------------------------------

#ifndef HASHED_PATTERN_HPP
#define HASHED_PATTERN_HPP

#include "Pattern.hpp"
#include "RingGodel.hpp"

//----------------------------------------------------------------------------

/** Hashes patterns by ring godel; use for fast checking.  
    
    For each valid ring godel, a list of RotatedPatterns is
    pre-computed from the given PatternSet.  This allows PatternBoard
    to check if a set of patterns matches a cell extremely quickly;
    especially if the patterns have a max extension of one, since in
    that case no checking is actually required!
*/
class HashedPatternSet
{
public:

    /** Creates empty set of hashed patterns. */
    HashedPatternSet();

    /** Destructor. */
    ~HashedPatternSet();

    /** Hashes the given patterns. */
    void hash(const PatternSet& patterns);

    /** Returns list of rotated patterns for godel. */
    const RotatedPatternList& ListForGodel(const RingGodel& godel) const;

private:

    /** Will contain a RotatedPatternList for each of
        RingGodel::ValidGodels(). */
    std::vector<RotatedPatternList> m_godel_list;
};

//----------------------------------------------------------------------------

#endif // HASHED_PATTERN_HPP
