//----------------------------------------------------------------------------
// $Id: htpbenzene.cpp 1470 2008-06-17 22:04:32Z broderic $
//----------------------------------------------------------------------------

#include "SgSystem.h"

#include "Benzene.hpp"
#include "HtpUofAEngine.hpp"

int main(int argc, char** argv)
{
    Benzene program;
    program.Initialize(argc, argv);
    
    HtpUofAEngine gh(std::cin, std::cout, program);
    gh.MainLoop();

    program.Shutdown();
    return 0;
}

//----------------------------------------------------------------------------
