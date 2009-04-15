//----------------------------------------------------------------------------
// $Id: HexProgram.cpp 1699 2008-09-24 23:55:56Z broderic $
//----------------------------------------------------------------------------

#include "SgSystem.h"

#include "Hex.hpp"
#include "HexInit.hpp"
#include "HexProp.hpp"
#include "HexProgram.hpp"
#include "RingGodel.cpp"
#include "BoardUtils.hpp"
#include "Resistance.hpp"

//----------------------------------------------------------------------------

HexProgram::HexProgram(std::string name, 
                       std::string version, 
                       std::string build,
                       std::string date)
    : m_name(name),
      m_version(version),
      m_build(build),
      m_date(date)
{
}

HexProgram::~HexProgram()
{
    // free the cmd-line callbacks
    typedef CallbackMap::iterator Iterator;
    for (Iterator i = m_callbacks.begin(); i != m_callbacks.end(); ++i)
    {
        delete i->second;
#ifndef NDEBUG
        i->second = 0;
#endif
    }
}

//----------------------------------------------------------------------------

void HexProgram::AddDefaultSettings() 
{
    hex::settings.put("seed",  "-1");

    hex::settings.put("log-cerr-level",          LogLevelUtil::toString(INFO));
    hex::settings.put("log-file-level",          LogLevelUtil::toString(FINE));
    hex::settings.put("log-file-name",            "default.log");

    // These settings are here because Six needs to know about them and
    // Six does not load bin/config/defaults!
    hex::settings.put("game-default-boardsize",                      "11");
    hex::settings.put("game-allow-swap",                          "false");
    hex::settings.put("game-total-time",                           "1800");
}

void HexProgram::RegisterCmdLineArguments()
{
    Register("help", NO_ARGUMENTS, 
             "Displays this usage information.",
             &HexProgram::Help);

    Register("version", NO_ARGUMENTS, 
             "Displays version information.",
             &HexProgram::Version);

    Register("quiet", NO_ARGUMENTS,
             "Suppresses log output to stderr.\n"
             "Same as 'log-cerr-level=off'",
             &HexProgram::Quiet);

    Register("verbose", NO_ARGUMENTS,
             "Echos all log messages to stderr.\n"
             "Same as 'log-cerr-level=all'",
             &HexProgram::Verbose);

    Register("log", STRING_ARGUMENT,
             "Sets the name of the logfile to %s.\n"
             "Equivalent to 'log-file-name'.",
             &HexProgram::Log);

}

void HexProgram::InitializeHexSystem()
{
    HexInitLog();

    hex::log << hex::config;
    
    hex::log << m_name;
    hex::log << " v" << m_version;
    hex::log << " build " << m_build;
    hex::log << " " << m_date << ".";
    hex::log << hex::endl;

    hex::log << "============ InitializeHexSystem ============" << hex::endl;

    SgProp::Init();
    HexInitProp();
    HexInitRandom();
    BoardUtils::Initialize();
    ResistanceUtil::Initialize();
}

void HexProgram::Initialize(int argc, char **argv)
{
    // store the name of the executable
    m_executable_name = argv[0];
    
    // determine the executable directory
    {
        std::string path = m_executable_name;    
        std::string::size_type loc = path.rfind('/', path.length()-1);
        if (loc == std::string::npos) {
            path = "";
        } else {
            path = path.substr(0, loc);
            path.append("/");
        }
        m_executable_path = path;
    }

    AddDefaultSettings();
    RegisterCmdLineArguments();
    ProcessCmdLineArguments(argc, argv);
    InitializeHexSystem();
}

//----------------------------------------------------------------------------

void HexProgram::Shutdown()
{
    HexShutdown();
}

//----------------------------------------------------------------------------

bool HexProgram::ProcessCmd(const std::string& str, 
                            std::string& name, 
                            std::string& value) const
{
    std::cerr << "Processing: '" << str << "'" << std::endl;
    if (str[0] == '-' && str[1] == '-') {
        std::string::size_type eq = str.find('=');
        if (eq == std::string::npos) {
            name = str.substr(2);
            value = "";
        } else {
            name = str.substr(2, eq-2);
            value = str.substr(eq+1, std::string::npos);
        }
        return true;
    }
    return false;
}

void HexProgram::ProcessCmdLineArguments(int argc, char **argv)
{
    // parse the arguments
    bool good = true;
    for (int index = 1; good && index < argc; ) {
        
        std::string cmd = argv[index++];
        std::string name, value;
        if (!ProcessCmd(cmd, name, value)) {
            std::cout << "Malformed command! '" << cmd << "'" << std::endl;
            good = false;
            break;
        }

        if (IsRegistered(name)) {
            if (value == "" && m_argtypes[name] != NO_ARGUMENTS) {
                if (index >= argc) {
                    std::cout << "Missing argument!" << std::endl;
                    good = false;
                    break;
                }
                value = argv[index++];
            } 
            else if (value != "" && m_argtypes[name] == NO_ARGUMENTS) {
                std::cout << "--" << name << " takes no arguments!"
                          << std::endl;
                good = false;
                break;
            }

            ArgCallbackBase* callback = m_callbacks[name];
            good = (*callback)(value);

        } 
        else if (hex::settings.defined(name)) {
            if (value == "") {
                if (index >= argc) {
                    std::cout << "Missing argument!" << std::endl;
                    good = false;
                    break;
                }
                value = argv[index++];
            }
            hex::settings.put(name, value);
        } 
        else {
            std::cout << "Invalid command: '" + cmd + "'" << std::endl;
            good = false;
        }
    }

    if (!good) {
        Usage();
        Shutdown();
        exit(1);
    }
}

void HexProgram::Usage() const
{
    std::cout << std::endl
              << "Usage: " 
              << std::endl 
              << "       " << m_executable_name << " [OPTIONS] [SETTINGS]" 
              << std::endl << std::endl;
    
    std::cout << "[OPTIONS] is any number of the following:"
              << std::endl << std::endl;
    
    UsageMap::const_iterator it;
    for (it = m_usages.begin(); it != m_usages.end(); ++it) {
        std::string name = "--" + it->first;
        
        ArgTypeMap::const_iterator argt = m_argtypes.find(it->first);
        if (argt != m_argtypes.end()) {
            if (argt->second == STRING_ARGUMENT)
                name += " %s";
            else if (argt->second == INT_ARGUMENT)
                name += " %i";
            else if (argt->second == DOUBLE_ARGUMENT)
                name += " %f";
            else if (argt->second == BOOL_ARGUMENT)
                name += " %b";
        } else {
            std::cout << "Warning!! '" << it->first 
                      << "' does not appear in argtype!!" << std::endl;
        }
        std::cout << std::setw(32) << name << "    ";

        std::string::size_type index = 0;
        std::string usage = it->second;
        while (index != std::string::npos) {
            index = usage.find('\n');
            std::cout << usage.substr(0, index) << std::endl;
            if (index != std::string::npos) {
                usage = usage.substr(index+1);
                std::cout << std::setw(32) << " " << "    ";
            }
        }
        std::cout << std::endl;
    }

    std::cout << std::endl 
              << "[SETTINGS] is any number of [name]=[value] pairs:"
              << std::endl << std::endl;

    std::cout << std::setw(32) << "Name" << "    " 
              << "Current value" << std::endl;

    const Settings::SettingsMap& lst = hex::settings.GetSettings();
    Settings::SettingsMap::const_iterator si = lst.begin();
    for (; si != lst.end(); ++si) {
        std::string name = "--" + si->first;
        std::cout << std::setw(32) << name << "    \"" 
                  << si->second << "\"" << std::endl;
    }

    std::cout << std::endl;
}


bool HexProgram::IsRegistered(const std::string& command) const
{
    return (m_callbacks.find(command) != m_callbacks.end());
}


void HexProgram::Register(const std::string& command,
                          ArgType argtype, 
                          const std::string& usage, 
                          ArgCallbackBase* callback)
{
    CallbackMap::iterator pos = m_callbacks.find(command);
    if (pos != m_callbacks.end()) {
//         delete pos->second;
//         m_callbacks.erase(pos);
        //hex::log << hex::severe << "Duplicate cmd-line option!" << hex::endl;
        //HexAssert(false);
    }
    m_callbacks[command] = callback;
    m_argtypes[command] = argtype;
    m_usages[command] = usage;
}


void HexProgram::Register(const std::string& command,
                          ArgType argtype, 
                          const std::string& usage, 
                          ArgCallback<HexProgram>::Method method)
{
    Register(command, argtype, usage, 
             new ArgCallback<HexProgram>(this, method));
}

//----------------------------------------------------------------------------

bool HexProgram::Version(const std::string& UNUSED(arg))
{
    std::cout << m_name;
    std::cout << " v" << m_version;
    std::cout << " build " << m_build;
    std::cout << " " << m_date << "." << std::endl;
    
    Shutdown();
    exit(0);
    return true;
}

bool HexProgram::Help(const std::string& UNUSED(arg))
{
    Usage();
    Shutdown();
    exit(0);
    return true;
}

bool HexProgram::Log(const std::string& arg)
{
    hex::settings.put("log-file-name", arg);
    return true;
}

bool HexProgram::Quiet(const std::string& UNUSED(arg))
{
    hex::settings.put("log-cerr-level", LogLevelUtil::toString(OFF));
    return true;
}

bool HexProgram::Verbose(const std::string& UNUSED(arg))
{
    hex::settings.put("log-cerr-level", LogLevelUtil::toString(ALL));    
    return true;
}


//----------------------------------------------------------------------------
