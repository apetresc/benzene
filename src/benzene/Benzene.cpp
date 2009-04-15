//----------------------------------------------------------------------------
// $Id: Benzene.cpp 624 2007-08-22 21:49:54Z broderic $
//----------------------------------------------------------------------------

#include "SgSystem.h"

#include "Benzene.hpp"
#include "buildinfo.h"

// NOTE: this is updated on each build because 'buildinfo.h' forces
// this file to be recompiled.
const char* build_date = __DATE__;

//----------------------------------------------------------------------------

Benzene::Benzene() 
    : UofAProgram("Benzene", HEX_VERSION, BUILD_NUMBER, build_date)
{
}

Benzene::~Benzene()
{
}

//----------------------------------------------------------------------------

void Benzene::AddDefaultSettings()
{
    UofAProgram::AddDefaultSettings();
}

void Benzene::RegisterCmdLineArguments()
{
    UofAProgram::RegisterCmdLineArguments();
}

void Benzene::Initialize(int argc, char** argv)
{
    UofAProgram::Initialize(argc, argv);
}

//----------------------------------------------------------------------------
