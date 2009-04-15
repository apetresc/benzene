//----------------------------------------------------------------------------
// $Id: HexSettings.hpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#ifndef HEXSETTINGS_HPP
#define HEXSETTINGS_HPP

#include <vector>
#include "Settings.hpp"

//----------------------------------------------------------------------------

/** Handles a stack of settings. */
class HexSettings
{
public:

    HexSettings();

    ~HexSettings();

    //------------------------------------------------------------------------

    /** Sets the current settings as the set of default settings. */
    void SetCurrentAsDefaults();

    /** Returns a copy of the default settings. */
    Settings Defaults() const;

    //------------------------------------------------------------------------

    /** Pushes a copy of the current settings onto the stack. */
    void Push();

    /** Pops the current level of settings off the stack. */
    void Pop();

    //------------------------------------------------------------------------

    /** Wipes all settings. */
    void Clear();

    /** Loads settings from file. */
    void LoadFile(const std::string& file);

    /** Reverts the current settings to those in defaults file. */
    void RevertToDefaults();

    //------------------------------------------------------------------------

    bool defined(const std::string& name) const;

    void put(const std::string& name, const std::string& value);
    void put_bool(const std::string& name, bool value);
    void put_int(const std::string& name, int value);
    void put_double(const std::string& name, double value);

    std::string get(const std::string& name);
    bool get_bool(const std::string& name);
    int get_int(const std::string& name);
    double get_double(const std::string& name);

    const Settings::SettingsMap& GetSettings() const;

private:

    Settings& Current();
    const Settings& Current() const;

    Settings m_defaults;

    std::vector<Settings> m_settings;
};

inline Settings& HexSettings::Current()
{
    return m_settings.back();
}

inline const Settings& HexSettings::Current() const
{
    return m_settings.back();
}

inline Settings HexSettings::Defaults() const
{
    return m_defaults;
}

inline const Settings::SettingsMap& HexSettings::GetSettings() const
{
    return Current().get_settings();
}

inline bool HexSettings::defined(const std::string& name) const
{
    return Current().defined(name);
}    

inline std::string HexSettings::get(const std::string& name)
{
    return Current().get(name);
}

inline bool HexSettings::get_bool(const std::string& name)
{
    return Current().get_bool(name);
}

inline int HexSettings::get_int(const std::string& name)
{
    return Current().get_int(name);
}

inline double HexSettings::get_double(const std::string& name)
{
    return Current().get_double(name);
}

inline void HexSettings::put(const std::string& name, const std::string& value)
{
    return Current().put(name, value);
}

inline void HexSettings::put_bool(const std::string& name, bool value)
{
    return Current().put_bool(name, value);
}

inline void HexSettings::put_int(const std::string& name, int value)
{
    return Current().put_int(name, value);
}

inline void HexSettings::put_double(const std::string& name, double value)
{
    return Current().put_double(name, value);
}

//----------------------------------------------------------------------------

#endif // HEXSETTINGS_HPP
