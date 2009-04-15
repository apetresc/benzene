//----------------------------------------------------------------------------
// $Id: IcePatternSet.hpp 1536 2008-07-09 22:47:27Z broderic $
//----------------------------------------------------------------------------

#ifndef ICE_PATTERN_SET_HPP
#define ICE_PATTERN_SET_HPP

#include "Pattern.hpp"
#include "HashedPatternSet.hpp"

//----------------------------------------------------------------------------

/** Collection of Patterns and HashedPatterns for use in ICE. */
class IcePatternSet
{
public:

    /** Creates an empty set of patterns. */
    IcePatternSet();

    /** Destructor. */
    ~IcePatternSet();

    /** Loads the patterns from the given file. */
    void LoadPatterns(const std::string& file);

    /** @name Pattern Access Methods */
    // @{
    const HashedPatternSet& HashedDead() const;
    const HashedPatternSet& HashedCaptured(HexColor color) const;
    const HashedPatternSet& HashedPermInf(HexColor color) const;
    const HashedPatternSet& HashedVulnerable(HexColor color) const;
    const HashedPatternSet& HashedDominated(HexColor color) const;
    // @}

private:

    /** @name Patterns 
        Patterns for each type of Inferior cell. */
    // @{
    std::vector<Pattern> m_dead;  
    std::vector<Pattern> m_captured[BLACK_AND_WHITE];
    std::vector<Pattern> m_permanently_inferior[BLACK_AND_WHITE];
    std::vector<Pattern> m_vulnerable[BLACK_AND_WHITE];
    std::vector<Pattern> m_dominated[BLACK_AND_WHITE];
    // @}

    /** @name Hashed Paterns
        HashedPatternSets for each type of inferior cell. */
    // @{
    HashedPatternSet m_hashed_dead;
    HashedPatternSet m_hashed_captured[BLACK_AND_WHITE];
    HashedPatternSet m_hashed_permanently_inferior[BLACK_AND_WHITE];
    HashedPatternSet m_hashed_vulnerable[BLACK_AND_WHITE];
    HashedPatternSet m_hashed_dominated[BLACK_AND_WHITE];
    // @}
};

inline const HashedPatternSet& IcePatternSet::HashedDead() const
{
    return m_hashed_dead;
}

inline const HashedPatternSet& 
IcePatternSet::HashedCaptured(HexColor color) const
{
    return m_hashed_captured[color];
}

inline const HashedPatternSet& 
IcePatternSet::HashedPermInf(HexColor color) const
{
    return m_hashed_permanently_inferior[color];
}

inline const HashedPatternSet& 
IcePatternSet::HashedVulnerable(HexColor color) const
{
    return m_hashed_vulnerable[color];
}

inline const HashedPatternSet& 
IcePatternSet::HashedDominated(HexColor color) const
{
    return m_hashed_dominated[color];
}

//----------------------------------------------------------------------------

#endif // ICE_PATTERN_SET_HPP
