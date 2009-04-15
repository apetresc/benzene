
#include "SgSystem.h"

#include "HexUctInit.hpp"
#include "PatternBoard.hpp"

//----------------------------------------------------------------------------

HexUctPriorKnowledge::HexUctPriorKnowledge(const HexUctState& state) :
    m_state(state),
    m_use_good(hex::settings.get_bool("uct-use-init-good")),
    m_use_bad(hex::settings.get_bool("uct-use-init-bad"))
{
}

HexUctPriorKnowledge::~HexUctPriorKnowledge()
{
}

void HexUctPriorKnowledge::ProcessPosition()
{
    // Get previous move and current board state.
    HexPoint prevMove = m_state.GetLastMovePlayed();
    HexColor toPlay = m_state.GetColorToPlay();
    const PatternBoard& brd = m_state.Board();
    
    // Among all cells, compute which moves are bad (dead/vulnerable)
    if (m_use_bad) {
	m_badResponses =
	    brd.matchPatternsOnBoard(brd.getEmpty(),
				     m_hash_bad_patterns[toPlay], NULL);
	//hex::log << hex::severe << "BAD:" << brd.printBitset(m_badResponses) << hex::endl;
    }
    
    // Among cells close to the last move played, determine which of
    // these are likely to be a strong response.
    if (m_use_good) {
	m_goodResponses.reset();
	if (prevMove == INVALID_POINT) return;
	
	PatternHits hits;
	brd.matchPatternsOnCell(m_hash_good_patterns[toPlay], prevMove,
				PatternBoard::MATCH_ALL, &hits);
	for (unsigned i=0; i<hits.size(); ++i) {
	    HexPoint p = hits[i].moves1()[0];
	    m_goodResponses.set(p);
	}
	//hex::log << hex::severe << "GOOD:" << brd.printBitset(m_goodResponses) << hex::endl;
    }
}

void HexUctPriorKnowledge::InitializeMove(SgMove move, float& value,
					  std::size_t& count)
{
    count = 0;
    
    if (move < 0 || move >= BITSETSIZE) {
	// If not a HexPoint, cannot analyze, so do not initialize.
	return;
    } else if (m_use_bad && m_badResponses.test(move)) {
	// If the move is bad, prevents its exploration.
	value = 0.0f;
	count = 11222;
    } else if (m_use_good && m_goodResponses.test(move)) {
	// If the response is likely to be good, ensure it is explored.
	value = 1.0f;
	count = 7;
    }
}

void HexUctPriorKnowledge::LoadPatterns(const std::string& config_dir)
{
    std::string file;
    file = hex::settings.get("uct-init-good-pattern-file");
    if (file[0] == '*')
        file = config_dir + file.substr(1);
    LoadGoodPatterns(file);
    
    file = hex::settings.get("uct-init-bad-pattern-file");
    if (file[0] == '*')
        file = config_dir + file.substr(1);
    LoadBadPatterns(file);
}

void HexUctPriorKnowledge::LoadGoodPatterns(const std::string& filename)
{
    std::vector<Pattern> patterns;
    Pattern::LoadPatternsFromFile(filename.c_str(), patterns);
    hex::log << hex::info
             << "HexUctPriorKnowledge: Read " << patterns.size()
	     << " patterns from '" << filename << "'."
             << hex::endl;
    
    // Can only load patterns once!
    HexAssert(m_good_patterns[BLACK].empty());
    
    for (uint i=0; i<patterns.size(); i++) {
        Pattern p = patterns[i];
        
        switch(p.getType()) {
        case Pattern::MOHEX:
            m_good_patterns[BLACK].push_back(p);
            p.flipColors();
            m_good_patterns[WHITE].push_back(p);
            break;
        default:
            hex::log << hex::warning;
            hex::log << "Pattern type = " << p.getType() << hex::endl;
            HexAssert(false);
        }
    }
    
    // create the hashed pattern sets for fast checking
    for (BWIterator color; color; ++color) {
        m_hash_good_patterns[*color].hash(m_good_patterns[*color]);
    }
}

void HexUctPriorKnowledge::LoadBadPatterns(const std::string& filename)
{
    std::vector<Pattern> patterns;
    Pattern::LoadPatternsFromFile(filename.c_str(), patterns);
    hex::log << hex::info
             << "HexUctPriorKnowledge: Read " << patterns.size()
	     << " patterns from '" << filename << "'."
             << hex::endl;
    
    // Can only load patterns once!
    HexAssert(m_bad_patterns[BLACK].empty());
    
    for (uint i=0; i<patterns.size(); i++) {
        Pattern p = patterns[i];
        
        switch(p.getType()) {
        case Pattern::MOHEX:
            m_bad_patterns[BLACK].push_back(p);
            p.flipColors();
            m_bad_patterns[WHITE].push_back(p);
            break;
        default:
            hex::log << hex::warning;
            hex::log << "Pattern type = " << p.getType() << hex::endl;
            HexAssert(false);
        }
    }
    
    // create the hashed pattern sets for fast checking
    for (BWIterator color; color; ++color) {
        m_hash_bad_patterns[*color].hash(m_bad_patterns[*color]);
    }
}

//----------------------------------------------------------------------------

HexUctPriorKnowledgeFactory::HexUctPriorKnowledgeFactory(const std::string&
							 config_dir)
    : m_config_dir(config_dir)
{
}

HexUctPriorKnowledgeFactory::~HexUctPriorKnowledgeFactory()
{
}

SgUctPriorKnowledge*
HexUctPriorKnowledgeFactory::Create(SgUctThreadState& state)
{
    HexUctPriorKnowledge* hupk = new
	HexUctPriorKnowledge(dynamic_cast<const HexUctState&>(state));
    if (hupk)
	hupk->LoadPatterns(m_config_dir);
    return hupk;
}

//----------------------------------------------------------------------------
