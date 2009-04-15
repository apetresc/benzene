//----------------------------------------------------------------------------
// $Id: HexProgram.hpp 1470 2008-06-17 22:04:32Z broderic $
//----------------------------------------------------------------------------

#ifndef HEXPROGRAM_HPP
#define HEXPROGRAM_HPP

#include <map>
#include <string>

typedef enum { 
    NO_ARGUMENTS, 
    BOOL_ARGUMENT, 
    INT_ARGUMENT, 
    DOUBLE_ARGUMENT, 
    STRING_ARGUMENT
} ArgType;

//----------------------------------------------------------------------------

/** Abstract base class for command handlers. */
class ArgCallbackBase
{
public:
    virtual ~ArgCallbackBase() throw() {};
    virtual bool operator()(const std::string&) = 0;
};


//----------------------------------------------------------------------------

template<class PROGRAM>
class ArgCallback : public ArgCallbackBase
{
public:
    /** Signature of the member function. */
    typedef bool (PROGRAM::*Method)(const std::string&);

    ArgCallback(PROGRAM* instance, ArgCallback<PROGRAM>::Method method);

    ~ArgCallback() throw();

    /** Execute the member function. */
    bool operator()(const std::string&);

private:
    PROGRAM* m_instance;
    Method m_method;
};

template<class PROGRAM>
ArgCallback<PROGRAM>::ArgCallback(PROGRAM* instance,
                                  ArgCallback<PROGRAM>::Method method)
    : m_instance(instance),
      m_method(method)
{
}

template<class PROGRAM>
ArgCallback<PROGRAM>::~ArgCallback() throw()
{
#ifndef NDEBUG
    m_instance = 0;
#endif
}

template<class PROGRAM>
bool ArgCallback<PROGRAM>::operator()(const std::string& cmd)
{
    return (m_instance->*m_method)(cmd);
}

//----------------------------------------------------------------------------

/** Program for playing Hex. 
    Parses command-line arguments and initializes the Hex system.
*/
class HexProgram
{
public:

    /** Creates a hex program of the given name, version, build# and date.
        Initalizes the Hex library. */
    HexProgram(std::string name, std::string version, 
               std::string build, std::string date);

    /** Destroys a hex program. */
    virtual ~HexProgram();

    /** Parses cmd-line arguments, starts up Hex system, etc. */
    virtual void Initialize(int argc, char **argv);

    /** Shuts down the program and the Hex system. */
    virtual void Shutdown();

    /** Prints all registered cmd-line arguments and their usage. */
    void Usage() const;

    /** Returns the name of the program. */
    std::string getName() const;

    /** Returns the version string of the program. */
    std::string getVersion() const;

    /** Returns the build number of the program. */
    std::string getBuild() const;

    /** Returns the build date of the program. */
    std::string getDate() const;

protected:

    /** Adds the programs default settings to the global list
        of settings. */
    virtual void AddDefaultSettings();

    /** Registers all command-line arguments. */
    virtual void RegisterCmdLineArguments();

    void Register(const std::string& command,
                  ArgType argtype, 
                  const std::string& usage, 
                  ArgCallbackBase* callback);

    //----------------------------------------------------------------------

    /** Cmd-line options. */
    bool Version(const std::string& arg);
    bool Help(const std::string& arg);
    bool Log(const std::string& arg);
    bool Quiet(const std::string& arg);
    bool Verbose(const std::string& arg);

    //----------------------------------------------------------------------

    std::string m_name;
    std::string m_version;
    std::string m_build;
    std::string m_date;

    std::string m_executable_name;
    std::string m_executable_path;

private:

    void InitializeHexSystem();

    /** Process cmd line arguments. */
    bool ProcessCmd(const std::string& str, 
                    std::string& name, 
                    std::string& value) const;
    
    void ProcessCmdLineArguments(int argc, char **argv);

    /** Returns true if the command is registered. */
    bool IsRegistered(const std::string& command) const;
    
    /** Registers a command-line argument with this program. */
    void Register(const std::string& command, 
                  ArgType type, 
                  const std::string& usage, 
                  ArgCallback<HexProgram>::Method method);

    //----------------------------------------------------------------------

    typedef std::map<std::string,ArgCallbackBase*> CallbackMap;
    typedef std::map<std::string,ArgType> ArgTypeMap;
    typedef std::map<std::string,std::string> UsageMap;

    CallbackMap m_callbacks;
    ArgTypeMap m_argtypes;
    UsageMap m_usages;
};

inline std::string HexProgram::getName() const
{
    return m_name;
}

inline std::string HexProgram::getVersion() const
{
    return m_version;
}

inline std::string HexProgram::getBuild() const
{
    return m_build;
}

inline std::string HexProgram::getDate() const
{
    return m_date;
}

//----------------------------------------------------------------------------

#endif // HEXPROGRAM_HPP
