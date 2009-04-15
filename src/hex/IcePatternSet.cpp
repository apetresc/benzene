//----------------------------------------------------------------------------
// $Id: IcePatternSet.cpp 1855 2009-01-21 02:22:43Z broderic $
//----------------------------------------------------------------------------

#include "IcePatternSet.hpp"

//----------------------------------------------------------------------------

IcePatternSet::IcePatternSet()
{
}

IcePatternSet::~IcePatternSet()
{
}

void IcePatternSet::LoadPatterns(const boost::filesystem::path& file)
{
    boost::filesystem::path normalizedFile = file;
    normalizedFile.normalize();
    std::string nativeFile = normalizedFile.native_file_string();

    std::vector<Pattern> patterns;
    Pattern::LoadPatternsFromFile(nativeFile.c_str(), patterns);

    hex::log << hex::fine << "IcePatternSet: "
             << "Read " << patterns.size() << " patterns " 
             << "from '" << nativeFile << "'."
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
