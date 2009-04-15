//----------------------------------------------------------------------------
// $Id: HexUctPolicy.hpp 1657 2008-09-15 23:32:09Z broderic $ 
//----------------------------------------------------------------------------

#ifndef HEXUCTPOLICY_H
#define HEXUCTPOLICY_H

#include "SgSystem.h"
#include "SgRandom.h"

#include "HexUctSearch.hpp"

//----------------------------------------------------------------------------

/** Configuration options for policies. */
struct HexUctPolicyConfig
{
    bool patternHeuristic;
    bool localBiasHeuristic;
    bool deadHeuristic;
    bool shiftHeuristic;
 
    int pattern_update_radius;
    int pattern_check_percent;

    int local_bias_percent;
    int local_bias_radius;

    /** Constructor reads settings from hex::settings. */
    HexUctPolicyConfig();
};

/** Statistics over all threads. */
struct HexUctPolicyStatistics
{
    int total_moves;
    int random_moves;
    int pattern_moves;
    int local_bias_moves;
    int dead_moves;
    int shift_moves;
    int shift_pattern;

    /** @todo Are these threadsafe? */
    std::map<const Pattern*, int> pattern_counts[BLACK_AND_WHITE];
    std::map<const Pattern*, int> pattern_picked[BLACK_AND_WHITE];

    HexUctPolicyStatistics()
        : total_moves(0),
          random_moves(0),
          pattern_moves(0),
          local_bias_moves(0),
          dead_moves(0),
          shift_moves(0),
          shift_pattern(0)
    { }
};

/** Policy information shared amoung all threads. */
class HexUctSharedPolicy
{
public:

    /** Constructor. */
    HexUctSharedPolicy();

    /** Destructor. */
    ~HexUctSharedPolicy();

    //----------------------------------------------------------------------

    /** Loads patterns; config_dir is the location of bin/config/. */
    void LoadPatterns(const std::string& config_dir);

    /** Returns set of patterns used to guide playouts. */
    const HashedPatternSet& PlayPatterns(HexColor color) const;

    /** Returns set of patterns used in DeadHeuristic(). */
    const HashedPatternSet& DeadPatterns() const;

    /** Returns set of patterns used in ShiftHeuristic(). */
    const HashedPatternSet& ShiftPatterns(HexColor color) const;

    /** Returns a string containing formatted statistics
        information. */
    std::string DumpStatistics();

    //----------------------------------------------------------------------
    
    /** Returns reference to current statistics so that threads can
        update this information. */        
    HexUctPolicyStatistics& Statistics();

    /** Returns constant reference to configuration settings
        controlling all policies. */
    const HexUctPolicyConfig& Config() const;
    
private:

    //--------------------------------------------------------------------- 

    HexUctPolicyStatistics m_statistics;

    //----------------------------------------------------------------------
    
    HexUctPolicyConfig m_config;

    //----------------------------------------------------------------------

    /** Loads patterns for use in rollouts. */
    void LoadPlayPatterns(const std::string& filename);

    /** Load patterns for fillin during rollouts. */
    void LoadDeadPatterns(const std::string& filename);
    
    /** Load patterns for shifting generated moves during rollouts. */
    void LoadShiftPatterns(const std::string& filename);

    std::vector<Pattern> m_patterns[BLACK_AND_WHITE];
    HashedPatternSet m_hash_patterns[BLACK_AND_WHITE];
    std::vector<Pattern> m_dead_patterns;
    HashedPatternSet m_hash_dead_patterns;
    std::vector<Pattern> m_shift_patterns[BLACK_AND_WHITE];
    HashedPatternSet m_hash_shift_patterns[BLACK_AND_WHITE];
};

inline HexUctPolicyStatistics& HexUctSharedPolicy::Statistics()
{
    return m_statistics;
}

inline const HexUctPolicyConfig& HexUctSharedPolicy::Config() const
{
    return m_config;
}

inline const HashedPatternSet& 
HexUctSharedPolicy::PlayPatterns(HexColor color) const
{
    return m_hash_patterns[color];
}

inline const HashedPatternSet& 
HexUctSharedPolicy::DeadPatterns() const
{
    return m_hash_dead_patterns;
}

inline const HashedPatternSet& 
HexUctSharedPolicy::ShiftPatterns(HexColor color) const
{
    return m_hash_shift_patterns[color];
}

//----------------------------------------------------------------------------

/** Generates moves during the random playout phase of UCT search.
   
    Uses local configuration and pattern data in HexUctSharedPolicy.

    Everything in this class must be thread-safe. 
*/
class HexUctPolicy : public HexUctSearchPolicy
{
public:

    /** Constructor. Creates a policy. */
    HexUctPolicy(HexUctSharedPolicy* shared);

    /* Destructor. */
    virtual ~HexUctPolicy();

    /** Implementation of SgUctSearch::GenerateRandomMove().
        - Pattern move (if enabled)
        - Purely random
        - If playing into a cell hit by an dead pattern, 
          generate another purely random move. 
    */
    virtual HexPoint GenerateMove(PatternBoard& brd, 
                                  HexColor color, 
                                  HexPoint lastMove);

    //------------------------------------------------------------

    /** Initializes the moves to generate from the empty cells on the
        given board. Should be called with the boardstate before any
        calls to GenerateMove(). */
    void InitializeForRollout(const StoneBoard& brd);

private:

    static const int MAX_VOTES = 1024;

    //----------------------------------------------------------------------

    /** Randomly picks a pattern move from the set of patterns that hit
        the last move, weighted by the pattern's weight. 
        If no pattern matches, returns INVALID_POINT. */
    HexPoint PickRandomPatternMove(const PatternBoard& brd, 
                                   const HashedPatternSet& patterns, 
                                   HexColor toPlay, 
                                   HexPoint lastMove);

    /** Uses PickRandomPatternMove() with the shared PlayPatterns(). */
    HexPoint GeneratePatternMove(const PatternBoard& brd, HexColor color, 
                                 HexPoint lastMove);

    /** Uses PickRandomPatternMove() to see if the curMove we have selected
	can be moved to a provably better option and, if so, returns one
	of the pattern-responses chosen at random. If no pattern matches,
	returns INVALID_POINT. */
    HexPoint GenerateShiftMove(const PatternBoard& brd, HexColor color,
			       HexPoint curMove);

    //----------------------------------------------------------------------

    /** Selects random move among the empty cells on the board. */
    HexPoint GenerateRandomMove(const PatternBoard& brd);

    //----------------------------------------------------------------------

    HexUctSharedPolicy* m_shared;

    std::vector<HexPoint> m_moves;

    /** Generator for this policy. */
    SgRandom m_random;
};

//----------------------------------------------------------------------------

#endif // HEXUCTPOLICY_H
