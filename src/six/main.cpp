//----------------------------------------------------------------------------
// $Id: main.cpp 1470 2008-06-17 22:04:32Z broderic $
//----------------------------------------------------------------------------

#include "SgSystem.h"

#include "config.h"

#include "carrier.h"
#include "misc.h"

#include "Six.hpp"
#include "HtpSixEngine.hpp"

//static const char *version = "v" VERSION;

// static KCmdLineOptions options[] =
// {
//     { "black ", I18N_NOOP("Set black player. Possible values are:\n"
//                           "\"human\", \"beginner\", \"intermediate\",\n"
//                           "\"advanced\", \"expert\"."), ""},
//     { "white ", I18N_NOOP("Set white player. Possible values are:\n"
//                           "\"human\", \"beginner\", \"intermediate\",\n"
//                           "\"advanced\", \"expert\"."), ""},
//   { "pbem-filter", I18N_NOOP("Import PBEM game from standard input."), 0 },
//     { "+[FILE]", I18N_NOOP("Start with the game loaded from FILE."), 0 },
//     { 0, 0, 0 }
// };

int main(int argc, char **argv)
{
    // Six specific initializations
    Carrier::init();  

    // start it up
    Six program;
    program.Initialize(argc, argv);

    HtpSixEngine gh(std::cin, std::cout, program);
    gh.MainLoop();

    program.Shutdown();
    return 0;
}

//----------------------------------------------------------------------------
