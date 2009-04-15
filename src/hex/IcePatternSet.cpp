//----------------------------------------------------------------------------
// $Id: IcePatternSet.cpp 1536 2008-07-09 22:47:27Z broderic $
//----------------------------------------------------------------------------

#include "IcePatternSet.hpp"

//----------------------------------------------------------------------------

IcePatternSet::IcePatternSet()
{
}

IcePatternSet::~IcePatternSet()
{
}

void IcePatternSet::LoadPatterns(const std::string& file)
{
    std::vector<Pattern> patterns;
    Pattern::LoadPatternsFromFile(file.c_str(), patterns);

    hex::log << hex::info << "IcePatternSet: "
             << "Read " << patterns.size() << " patterns " 
             << "from '" << file << "'."
             << hex::endl;
    
    for (uint i=0; i<patterns.size(); i++) {
        Pattern p(patterns[i]);

        switch(p.getType()) {
        case Pattern::DEAD:
            m_dead.push_back(p);
            break;

        case Pattern::CAPTURED:
            // WHITE is first!!
            m_captured[WHITE].push_back(p);  
            p.flipColors();
            m_captured[BLACK].push_back(p);
            break;

        case Pattern::PERMANENTLY_INFERIOR:
            // WHITE is first!!
            m_permanently_inferior[WHITE].push_back(p);  
            p.flipColors();
            m_permanently_inferior[BLACK].push_back(p);
            break;

        case Pattern::VULNERABLE:
            m_vulnerable[BLACK].push_back(p);
            p.flipColors();
            m_vulnerable[WHITE].push_back(p);
            break;

        case Pattern::DOMINATED:
            m_dominated[BLACK].push_back(p);
            p.flipColors();
            m_dominated[WHITE].push_back(p);
            break;

        default:
            hex::log << hex::severe;
            hex::log << "Pattern type = " << p.getType() << hex::endl;
            HexAssert(false);
        }
    }

    m_hashed_dead.hash(m_dead);
    for (BWIterator it; it; ++it) {
        m_hashed_captured[*it].hash(m_captured[*it]);
        m_hashed_permanently_inferior[*it].hash(m_permanently_inferior[*it]);
        m_hashed_vulnerable[*it].hash(m_vulnerable[*it]);
        m_hashed_dominated[*it].hash(m_dominated[*it]);
    }
}

//----------------------------------------------------------------------------
