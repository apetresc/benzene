//----------------------------------------------------------------------------
// $Id: Six.cpp 1426 2008-06-04 21:18:22Z broderic $
//----------------------------------------------------------------------------

#include "SgSystem.h"

#include "Hex.hpp"
#include "Six.hpp"
#include "buildinfo.h"

#include "sixplayer.h"

// NOTE: this is updated on each build because 'buildinfo.h' forces
// this file to be recompiled.
const char* build_date = __DATE__;

//----------------------------------------------------------------------------

Six::Six() 
    : HexProgram("Six", "0.5.3", BUILD_NUMBER, build_date)
{
}

Six::~Six()
{
}

//----------------------------------------------------------------------------

void Six::AddDefaultSettings()
{
    HexProgram::AddDefaultSettings();

    hex::settings.put("skill-level",         "expert");
}

void Six::Register(const std::string& command,
                   ArgType argtype, 
                   const std::string& usage, 
                   ArgCallback<Six>::Method method)
{
    HexProgram::Register(command, argtype, usage, 
                         new ArgCallback<Six>(this, method));
}

void Six::RegisterCmdLineArguments()
{
    HexProgram::RegisterCmdLineArguments();

    Register("expert", NO_ARGUMENTS,
             "Expert level play.",
             &Six::Expert);
}

void Six::Initialize(int argc, char** argv)
{
    HexProgram::Initialize(argc, argv);
}

//----------------------------------------------------------------------------

bool Six::Expert(const std::string& arg)
{
    hex::settings.put("play-level", "expert");
    return true;
}

//----------------------------------------------------------------------------
