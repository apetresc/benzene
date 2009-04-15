//----------------------------------------------------------------------------
// $Id: HexUctPolicy.cpp 1897 2009-02-05 01:43:36Z broderic $
//----------------------------------------------------------------------------

#include "Hex.hpp"
#include "Misc.hpp"
#include "PatternBoard.hpp"
#include "HexUctPolicy.hpp"

#include <boost/filesystem/path.hpp>

//----------------------------------------------------------------------------

namespace 
{

/** Shuffle a vector with the given random number generator. 
    @todo Refactor this out somewhere.
*/
template<typename T>
void ShuffleVector(std::vector<T>& v, SgRandom& random)
{
    for (int i=v.size()-1; i>0; --i) {
        int j = random.Int(i+1);
        std::swap(v[i], v[j]);
    }
}

/** Returns true 'percent' of the time. */
bool PercentChance(int percent, SgRandom& random)
{
    if (percent >= 100) return true;
    unsigned int threshold = random.PercentageThreshold(percent);
    return random.RandomEvent(threshold);
}

} // annonymous namespace

//----------------------------------------------------------------------------

HexUctPolicyConfig::HexUctPolicyConfig()
    : patternHeuristic(true),
      pattern_update_radius(1),
      pattern_check_percent(100)
{
}

//----------------------------------------------------------------------------

HexUctSharedPolicy::HexUctSharedPolicy()
    : m_config()
#if COLLECT_PATTERN_STATISTICS
    , m_statistics()
#endif
{
    hex::log << hex::fine << "--- HexUctSharedPolicy" << hex::endl;
    LoadPatterns();
}

HexUctSharedPolicy::~HexUctSharedPolicy()
{
}

void HexUctSharedPolicy::LoadPatterns()
{
    using namespace boost::filesystem;
    path p = path(ABS_TOP_SRCDIR) / "share" / "mohex-patterns.txt";
    p.normalize();
    LoadPlayPatterns(p.native_file_string());
}

void HexUctSharedPolicy::LoadPlayPatterns(const std::string& filename)
{
    std::vector<Pattern> patterns;
    Pattern::LoadPatternsFromFile(filename.c_str(), patterns);
    hex::log << hex::info
             << "HexUctSharedPolicy: Read " << patterns.size()
	     << " patterns from '" << filename << "'."
             << hex::endl;

    // can only load patterns once!
    HexAssert(m_patterns[BLACK].empty());

    for (uint i=0; i<patterns.size(); i++) {
        Pattern p = patterns[i];
        
        switch(p.getType()) {
        case Pattern::MOHEX:
            m_patterns[BLACK].push_back(p);
            p.flipColors();
            m_patterns[WHITE].push_back(p);
            break;

        default:
            hex::log << hex::warning;
            hex::log << "Pattern type = " << p.getType() << hex::endl;
            HexAssert(false);
        }
    }

    // create the hashed pattern sets for fast checking
    for (BWIterator color; color; ++color) {
        m_hash_patterns[*color].hash(m_patterns[*color]);
    }
}

#if COLLECT_PATTERN_STATISTICS
std::string HexUctSharedPolicy::DumpStatistics()
{
    std::ostringstream os;

    os << std::endl;
    os << "Pattern statistics:" << std::endl;
    os << std::setw(12) << "Name" << "  "
       << std::setw(10) << "Black" << " "
       << std::setw(10) << "White" << " "
       << std::setw(10) << "Black" << " "
       << std::setw(10) << "White" << std::endl;

    os << "     ------------------------------------------------------" 
       << std::endl;

    HexUctPolicyStatistics& stats = Statistics();
    for (unsigned i=0; i<m_patterns[BLACK].size(); ++i) {
        os << std::setw(12) << m_patterns[BLACK][i].getName() << ": "
           << std::setw(10) << stats.pattern_counts[BLACK]
            [&m_patterns[BLACK][i]] << " "
           << std::setw(10) << stats.pattern_counts[WHITE]
            [&m_patterns[WHITE][i]] << " " 
           << std::setw(10) << stats.pattern_picked[BLACK]
            [&m_patterns[BLACK][i]] << " "
           << std::setw(10) << stats.pattern_picked[WHITE]
            [&m_patterns[WHITE][i]]
           << std::endl;
    }

    os << "     ------------------------------------------------------" 
       << std::endl;

    os << std::endl;
    os << std::setw(12) << "Pattern" << ": " 
       << std::setw(10) << stats.pattern_moves << " "
       << std::setw(10) << std::setprecision(3) << 
        stats.pattern_moves*100.0/stats.total_moves << "%" 
       << std::endl;
    os << std::setw(12) << "Random" << ": " 
       << std::setw(10) << stats.random_moves << " "
       << std::setw(10) << std::setprecision(3) << 
        stats.random_moves*100.0/stats.total_moves << "%"  
       << std::endl;
    os << std::setw(12) << "Total" << ": " 
       << std::setw(10) << stats.total_moves << std::endl;

    os << std::endl;
    
    return os.str();
}
#endif

//----------------------------------------------------------------------------

HexUctPolicy::HexUctPolicy(HexUctSharedPolicy* shared)
    : m_shared(shared)
{
}

HexUctPolicy::~HexUctPolicy()
{
}

//----------------------------------------------------------------------------

void HexUctPolicy::InitializeForRollout(const StoneBoard& brd)
{
    BitsetUtil::BitsetToVector(brd.getEmpty(), m_moves);
    ShuffleVector(m_moves, m_random);
}

HexPoint HexUctPolicy::GenerateRandomMove(const PatternBoard& brd)
{
    HexPoint ret = INVALID_POINT;
    while (true) {
	HexAssert(!m_moves.empty());
        ret = m_moves.back();
        m_moves.pop_back();
        if (brd.isEmpty(ret))
            break;
    }
    return ret;
}

HexPoint HexUctPolicy::GenerateMove(PatternBoard& brd, 
                                    HexColor toPlay, 
                                    HexPoint lastMove)
{
    HexPoint move = INVALID_POINT;
    bool pattern_move = false;
    const HexUctPolicyConfig& config = m_shared->Config();
#if COLLECT_PATTERN_STATISTICS
    HexUctPolicyStatistics& stats = m_shared->Statistics();
#endif

    // patterns applied probabilistically (if heuristic is turned on)
    if (config.patternHeuristic 
        && PercentChance(config.pattern_check_percent, m_random))
    {
        move = GeneratePatternMove(brd, toPlay, lastMove);
    }
    
    // select random move if invalid point from pattern heuristic
    if (move == INVALID_POINT) 
    {
#if COLLECT_PATTERN_STATISTICS
	stats.random_moves++;
#endif
        move = GenerateRandomMove(brd);
    } 
    else 
    {
	pattern_move = true;
#if COLLECT_PATTERN_STATISTICS
        stats.pattern_moves++;
#endif
    }
    
    HexAssert(brd.isEmpty(move));
#if COLLECT_PATTERN_STATISTICS
    stats.total_moves++;
#endif

    return move;
}

//--------------------------------------------------------------------------

HexPoint HexUctPolicy::PickRandomPatternMove(const PatternBoard& brd, 
                                             const HashedPatternSet& patterns, 
                                             HexColor UNUSED(toPlay),
                                             HexPoint lastMove)
{
    if (lastMove == INVALID_POINT)
	return INVALID_POINT;
    
    int num = 0;
    int patternIndex[MAX_VOTES];
    HexPoint patternMoves[MAX_VOTES];

    PatternHits hits;
    brd.matchPatternsOnCell(patterns, lastMove, PatternBoard::MATCH_ALL, hits);

    for (unsigned i=0; i<hits.size(); ++i) 
    {
#if COLLECT_PATTERN_STATISTICS
        // record that this pattern hit
        m_shared->Statistics().pattern_counts[toPlay][hits[i].pattern()]++;
#endif
            
        // number of entries added to array is equal to the pattern's weight
        for (int j=0; j<hits[i].pattern()->getWeight(); ++j) 
        {
            patternIndex[num] = i;
            patternMoves[num] = hits[i].moves1()[0];
            num++;
            HexAssert(num < MAX_VOTES);
        }
    }
    
    // abort if no pattern hit
    if (num == 0) 
        return INVALID_POINT;
    
    // select move at random (biased according to number of entries)
    int i = m_random.Int(num);

#if COLLECT_PATTERN_STATISTICS
    m_shared->Statistics().pattern_picked
        [toPlay][hits[patternIndex[i]].pattern()]++;
#endif

    return patternMoves[i];
}

HexPoint HexUctPolicy::GeneratePatternMove(const PatternBoard& brd, 
                                           HexColor toPlay, 
                                           HexPoint lastMove)
{
    return PickRandomPatternMove(brd, m_shared->PlayPatterns(toPlay),
                                 toPlay, lastMove);
}

//----------------------------------------------------------------------------
