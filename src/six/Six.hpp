//----------------------------------------------------------------------------
// $Id$
//----------------------------------------------------------------------------

#ifndef SIX_H
#define SIX_H

#include "HexProgram.hpp"

class Six : public HexProgram
{
public:
    Six();
    ~Six();

    virtual void Initialize(int argc, char** argv);

protected:
    virtual void AddDefaultSettings();
    virtual void RegisterCmdLineArguments();

    void Register(const std::string& command,
                  ArgType argtype, 
                  const std::string& usage, 
                  ArgCallback<Six>::Method method);

    //------------------------------------------------------------
    // Cmd-line options

    bool Expert(const std::string& arg);

};

#endif // SIX_H

//----------------------------------------------------------------------------
