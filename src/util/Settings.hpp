//----------------------------------------------------------------------------
// $Id: Settings.hpp 1061 2007-12-17 22:45:16Z broderic $
//----------------------------------------------------------------------------

#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <string>
#include <map>
#include <sstream>
#include <iostream>

//----------------------------------------------------------------------------

/** A map of (string, string) pairs containing various configuration 
    settings. */
class Settings
{
public:
    bool defined(const std::string& name) const;

    void put(const std::string& name, const std::string& value);
    void put_bool(const std::string& name, bool value);
    void put_int(const std::string& name, int value);
    void put_double(const std::string& name, double value);

    std::string get(const std::string& name);
    bool get_bool(const std::string& name);
    int get_int(const std::string& name);
    double get_double(const std::string& name);

    typedef std::map<std::string, std::string> SettingsMap;
    const SettingsMap& get_settings() const;

private:
    SettingsMap m_settings;
};

inline const Settings::SettingsMap& Settings::get_settings() const
{
    return m_settings;
}

inline bool Settings::defined(const std::string& name) const
{
    SettingsMap::const_iterator pos = m_settings.find(name);
    if (pos == m_settings.end())
        return false;
    return true;
}

inline void Settings::put(const std::string& name, const std::string& value)
{
    //std::cerr << "put: '" << name << "' as '" << value << "'" << std::endl;
    m_settings[name] = value;
}

inline void Settings::put_bool(const std::string& name, bool value)
{
    put(name, (value) ? "true" : "false");
}

inline void Settings::put_int(const std::string& name, int value)
{
    std::ostringstream os;
    os << value;
    put(name, os.str());
}

inline void Settings::put_double(const std::string& name, double value)
{
    std::ostringstream os;
    os << value;
    put(name, os.str());
}

inline std::string Settings::get(const std::string& name) 
{
    return m_settings[name];
}

inline bool Settings::get_bool(const std::string& name) 
{
    if (defined(name)) {
        std::string ret = m_settings[name];
        return (ret == "true") ? true : false;
    } 
    return false;
}

inline int Settings::get_int(const std::string& name) 
{
    if (defined(name)) {
        std::istringstream is(m_settings[name]);
        int ret;
        is >> ret;
        return ret;
    } 
    return 0;
}

inline double Settings::get_double(const std::string& name)
{
    if (defined(name)) {
        std::istringstream is(m_settings[name]);
        double ret;
        is >> ret;
        return ret;
    } 
    return 0.0f;
}

#endif // SETTINGS_HPP

//----------------------------------------------------------------------------
