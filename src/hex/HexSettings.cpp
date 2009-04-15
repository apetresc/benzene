//----------------------------------------------------------------------------
// $Id: HexSettings.cpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#include "Hex.hpp"
#include "HexSettings.hpp"

//----------------------------------------------------------------------------

HexSettings::HexSettings()
    : m_settings(1, Settings())
{
}

HexSettings::~HexSettings()
{
}

//----------------------------------------------------------------------------

void HexSettings::SetCurrentAsDefaults()
{
    m_defaults = Current();
}

void HexSettings::Push()
{
    Settings old = Current();
    m_settings.push_back(Settings());
    Current() = old;
}

void HexSettings::Pop()
{
    HexAssert(m_settings.size() >= 2);
    m_settings.pop_back();
}

//----------------------------------------------------------------------------

void HexSettings::Clear()
{
    Current() = Settings();
}

void HexSettings::RevertToDefaults()
{
    Clear();
    Current() = Defaults();
}

void HexSettings::LoadFile(const std::string& filename)
{
    std::ifstream is(filename.c_str());
    if (!is) {
        std::cerr << "Could not load settings from '" << filename << "'"
                  << std::endl;
        return;
    }

    char line[4096];
    while (is.getline(line, 4096)) {
        std::istringstream iss;
        iss.str(line);

        std::string name, value;
        if (!(iss >> name)) 
            continue;
        
        std::string ss(line);
        std::string::size_type a = ss.find('\"');
        if (a == std::string::npos) continue;
        std::string::size_type b = ss.rfind('\"', ss.length()-1);
        if (b == std::string::npos) continue;
        if (b < a+1) continue;
        value = ss.substr(a+1, b-a-1);
        
        Current().put(name, value);
    }
}

//----------------------------------------------------------------------------



