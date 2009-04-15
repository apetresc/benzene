//----------------------------------------------------------------------------
// $Id: HexUctPolicy.cpp 1674 2008-09-18 23:25:45Z broderic $
//----------------------------------------------------------------------------

#include "Hex.hpp"
#include "Misc.hpp"
#include "PatternBoard.hpp"
#include "HexUctPolicy.hpp"

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
{
    patternHeuristic = hex::settings.get_bool("uct-rollout-patterns");
    localBiasHeuristic = hex::settings.get_bool("uct-rollout-local-bias");
    deadHeuristic = hex::settings.get_bool("uct-rollout-dead");
    shiftHeuristic = hex::settings.get_bool("uct-rollout-shift");
    
    pattern_check_percent = hex::settings.get_int("uct-pattern-check-percent");
    pattern_update_radius = hex::settings.get_int("uct-rollout-update-radius");

    local_bias_percent = hex::settings.get_int("uct-local-bias-percent");
    local_bias_radius = hex::settings.get_int("uct-local-bias-radius");
}

//----------------------------------------------------------------------------

HexUctSharedPolicy::HexUctSharedPolicy()
    : m_statistics(),
      m_config()
{
    hex::log << hex::fine << "--- HexUctSharedPolicy" << hex::endl;
}

HexUctSharedPolicy::~HexUctSharedPolicy()
{
}

void HexUctSharedPolicy::LoadPatterns(const std::string& config_dir)
{
    std::string file = hex::settings.get("uct-pattern-file");
    if (file[0] == '*') 
        file = config_dir + file.substr(1);
    LoadPlayPatterns(file);

    file = hex::settings.get("uct-dead-pattern-file");
    if (file[0] == '*') 
        file = config_dir + file.substr(1);
    LoadDeadPatterns(file);
    
    file = hex::settings.get("uct-shift-pattern-file");
    if (file[0] == '*') 
        file = config_dir + file.substr(1);
    LoadShiftPatterns(file);
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

void HexUctSharedPolicy::LoadDeadPatterns(const std::string& filename)
{
    std::vector<Pattern> deadPatterns;
    Pattern::LoadPatternsFromFile(filename.c_str(), deadPatterns);
    hex::log << hex::info
             << "HexUctSharedPolicy: Read " << deadPatterns.size()
	     << " dead patterns from '" << filename << "'."
             << hex::endl;

    for (uint i=0; i<deadPatterns.size(); i++) {
        Pattern ip = deadPatterns[i];
        
        switch(ip.getType()) {
        case Pattern::DEAD:
            m_dead_patterns.push_back(ip);
            break;

        default:
            hex::log << hex::warning;
            hex::log << "Pattern type = " << ip.getType() << hex::endl;
            HexAssert(false);
        }
    }

    // create the hashed pattern sets for fast checking
    m_hash_dead_patterns.hash(m_dead_patterns);
}

void HexUctSharedPolicy::LoadShiftPatterns(const std::string& filename)
{
    std::vector<Pattern> shiftPatterns;
    Pattern::LoadPatternsFromFile(filename.c_str(), shiftPatterns);
    hex::log << hex::info
             << "HexUctSharedPolicy: Read " << shiftPatterns.size()
	     << " shift patterns from '" << filename << "'."
             << hex::endl;

    for (uint i=0; i<shiftPatterns.size(); i++) {
        Pattern sp = shiftPatterns[i];
        
        switch(sp.getType()) {
        case Pattern::SHIFT:
            m_shift_patterns[BLACK].push_back(sp);
            sp.flipColors();
            m_shift_patterns[WHITE].push_back(sp);
            break;

        default:
            hex::log << hex::warning;
            hex::log << "Pattern type = " << sp.getType() << hex::endl;
            HexAssert(false);
        }
    }

    // create the hashed pattern sets for fast checking
    for (BWIterator color; color; ++color) {
        m_hash_shift_patterns[*color].hash(m_shift_patterns[*color]);
    }
}

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
        os << std::setw(12)<<m_patterns[BLACK][i].getName() << ": "
           << std::setw(10)<<stats.pattern_counts[BLACK][&m_patterns[BLACK][i]]<<" "
           << std::setw(10)<<stats.pattern_counts[WHITE][&m_patterns[WHITE][i]]<<" " 
           << std::setw(10)<<stats.pattern_picked[BLACK][&m_patterns[BLACK][i]]<<" "
           << std::setw(10)<<stats.pattern_picked[WHITE][&m_patterns[WHITE][i]]
           << std::endl;
    }

    os << "     ------------------------------------------------------" 
       << std::endl;


    if (Config().shiftHeuristic) {
        for (unsigned i=0; i<m_shift_patterns[BLACK].size(); ++i) {
            os << std::setw(12)<<m_shift_patterns[BLACK][i].getName() << ": "
               << std::setw(10)<<stats.pattern_counts[BLACK][&m_shift_patterns[BLACK][i]]<<" "
               << std::setw(10)<<stats.pattern_counts[WHITE][&m_shift_patterns[WHITE][i]]<<" " 
               << std::setw(10)<<stats.pattern_picked[BLACK][&m_shift_patterns[BLACK][i]]<<" "
               << std::setw(10)<<stats.pattern_picked[WHITE][&m_shift_patterns[WHITE][i]]
               << std::endl;
        }
        
        os << "     ------------------------------------------------------" 
           << std::endl;
    }
    
    if (Config().deadHeuristic) {

        for (unsigned i=0; i<m_dead_patterns.size(); ++i) {
            os << std::setw(12)<<m_dead_patterns[i].getName() << ": "
               << std::setw(10)<<stats.pattern_counts[BLACK][&m_dead_patterns[i]]<<" "
               << std::setw(10)<<stats.pattern_counts[WHITE][&m_dead_patterns[i]]<<" " 
               << std::endl;
        }
    
        os << "     ------------------------------------------------------" 
           << std::endl;
    }

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

    os << std::setw(12) << "Local Bias" << ": " 
       << std::setw(10) << stats.local_bias_moves << " " 
       << std::setw(10) << std::setprecision(3) << 
        stats.local_bias_moves*100.0/stats.total_moves << "%"
       << std::endl;
    os << std::setw(12) << "Dead" << ": " 
       << std::setw(10) << stats.dead_moves << " " 
       << std::setw(10) << std::setprecision(3) << 
        stats.dead_moves*100.0/stats.total_moves << "%"
       << std::endl;
    os << std::setw(12) << "Shift" << ": " 
       << std::setw(10) << (stats.shift_moves-stats.shift_pattern) << " " 
       << std::setw(10) << std::setprecision(3) << 
        (stats.shift_moves-stats.shift_pattern)*100.0/stats.total_moves << "%"
       << std::setw(10) << stats.shift_pattern << " "
       << std::setw(10) << std::setprecision(3) << 
        stats.shift_pattern*100/stats.total_moves << "%"
       << std::endl;
    
    return os.str();
}

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
    HexUctPolicyStatistics& stats = m_shared->Statistics();

    // patterns applied probabilistically (if heuristic is turned on)
    if (config.patternHeuristic 
        && PercentChance(config.pattern_check_percent, m_random))
    {
        move = GeneratePatternMove(brd, toPlay, lastMove);
    }
    
    // select random move if invalid point from pattern heuristic
    if (move == INVALID_POINT) {
	stats.random_moves++;
	
	// bias random move locally if heuristic enabled
	if (config.localBiasHeuristic 
            && PercentChance(config.local_bias_percent, m_random))
        {
	    // @todo If bias proves worthwhile, need to optimize this
	    // within-radius-random-empty-neighbour selection somehow
	    std::vector<HexPoint> v;
	    for (BoardIterator i=brd.const_nbs(lastMove, 
                                 config.local_bias_radius);
		 i; 
                 ++i) 
            {
		if (brd.isEmpty(*i))
		    v.push_back(*i);
	    }
	    if (!v.empty()) {
                stats.local_bias_moves++;
		move = v[m_random.Int(v.size())];
	    }
	}
	if (move == INVALID_POINT) {
	    move = GenerateRandomMove(brd);
	}
    } else {
	pattern_move = true;
        stats.pattern_moves++;
    }
    
    // check if move is dead; if so, play a random move
    if (config.deadHeuristic) {
        for (int iteration=0; ; ++iteration) {

            // if this is the last empty cell on the board, abort now, even if
            // it is dead. 
            if ((brd.getEmpty() & brd.getCells()).count() == 1)
                break;

            PatternHits hits;
            brd.matchPatternsOnCell(m_shared->DeadPatterns(), move, 
                                    PatternBoard::STOP_AT_FIRST_HIT, &hits);

            if (hits.size()) {
                pattern_move = false;
                stats.pattern_counts
                    [toPlay][hits[0].pattern()]++;

                if (!iteration) stats.dead_moves++;

                brd.playMove(toPlay, move);
		HexAssert(brd.updateRadius() == config.pattern_update_radius);
		if (config.pattern_update_radius == 1)
		    brd.updateRingGodel(move);
		else
		    brd.update(move);
                brd.absorb(move);
                move = GenerateRandomMove(brd);
                
            } else {
                break;
            }
        }
    }
    
    // we may want to alter the generated move if it matches a shift pattern
    if (config.shiftHeuristic) {
	HexPoint shiftMove = GenerateShiftMove(brd, toPlay, move);
	if (shiftMove != INVALID_POINT) {
	    //@bug This will become the next random move - is this bad??
	    // (seems to bias it so opponent will usually have to play it...)
	    m_moves.push_back(move);
	    move = shiftMove;
	    stats.shift_moves++;
            if (pattern_move) stats.shift_pattern++;
	}
    }
    
    HexAssert(brd.isEmpty(move));
    stats.total_moves++;
    return move;
}

//--------------------------------------------------------------------------

HexPoint HexUctPolicy::PickRandomPatternMove(const PatternBoard& brd, 
                                             const HashedPatternSet& patterns, 
                                             HexColor toPlay, 
                                             HexPoint lastMove)
{
    if (lastMove == INVALID_POINT)
	return INVALID_POINT;
    
    int num = 0;
    int patternIndex[MAX_VOTES];
    HexPoint patternMoves[MAX_VOTES];

    PatternHits hits;
    brd.matchPatternsOnCell(patterns, lastMove, 
                            PatternBoard::MATCH_ALL, &hits);

    for (unsigned i=0; i<hits.size(); ++i) 
    {
        // record that this pattern hit
        m_shared->Statistics().pattern_counts[toPlay][hits[i].pattern()]++;
            
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
    m_shared->Statistics().pattern_picked
        [toPlay][hits[patternIndex[i]].pattern()]++;
    return patternMoves[i];
}

HexPoint HexUctPolicy::GeneratePatternMove(const PatternBoard& brd, 
                                           HexColor toPlay, 
                                           HexPoint lastMove)
{
    return PickRandomPatternMove(brd, m_shared->PlayPatterns(toPlay),
                                 toPlay, lastMove);
}

HexPoint HexUctPolicy::GenerateShiftMove(const PatternBoard& brd,
					 HexColor toPlay, 
					 HexPoint curMove)
{
    return PickRandomPatternMove(brd, m_shared->ShiftPatterns(toPlay),
                                 toPlay, curMove);
}

//----------------------------------------------------------------------------
