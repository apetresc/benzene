//----------------------------------------------------------------------------
// $Id: HexInit.cpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#include "SgSystem.h"
#include "SgProp.h"
#include "SgRandom.h"

#include "Hex.hpp"
#include "Time.hpp"
#include "HexException.hpp"
#include "HexProp.hpp"
#include "ZobristHash.hpp"

//----------------------------------------------------------------------------

LogHandler* g_StdErrHandler;
std::ofstream g_logfile;
LogHandler* g_LogFileHandler;

/** Global hex namespace: settings, logger, etc. */
namespace hex 
{
    HexSettings settings;

    Logger log("hex");
}

//----------------------------------------------------------------------------

void HexInitLog()
{
    LogLevel cerrLevel = 
        LogLevelUtil::fromString(hex::settings.get("log-cerr-level"));
    LogLevel logLevel =
        LogLevelUtil::fromString(hex::settings.get("log-file-level"));
    std::string logName = hex::settings.get("log-file-name");

    // create the std::cerr handler and add it to global log
    g_StdErrHandler = new LogHandler(std::cerr);
    hex::log.AddHandler(g_StdErrHandler, cerrLevel);

    // create the log file
    g_logfile.open(logName.c_str());
    if (!g_logfile) {
        hex::log << hex::warning << "Could not open log file ('" << logName 
                 << "') for writing!  No log file will be used." << hex::endl;
        g_LogFileHandler = 0;
    } else {

        g_LogFileHandler = new LogHandler(g_logfile);
        hex::log.AddHandler(g_LogFileHandler, logLevel);
    }
}

void HexInitRandom()
{
    int seed = hex::settings.get_int("seed");
    hex::log << hex::config << "seed = " << seed << hex::endl;
    if (seed == -1) {
        seed = time(NULL);
        hex::log << hex::config << "Set seed to time(NULL). seed = " << seed 
                 << hex::endl;
    }
    SgRandom::SetSeed(seed);
}

void HexShutdownLog()
{
    if (g_logfile) g_logfile << "--- HexLogShutdown" << std::endl;

    if (g_logfile) g_logfile << "Flushing log..." << std::endl;    
    hex::log.Flush();

    if (g_logfile) g_logfile << "Removing handlers..." << std::endl;
    hex::log.RemoveHandler(g_StdErrHandler);
    hex::log.RemoveHandler(g_LogFileHandler);

    if (g_logfile) g_logfile << "Deleting handlers..." << std::endl;
    if (g_StdErrHandler) delete g_StdErrHandler;
    if (g_LogFileHandler) delete g_LogFileHandler;

    if (g_logfile) 
        g_logfile << "Flushing and closing this stream..." << std::endl;

    g_logfile.flush();
    g_logfile.close();
}

//----------------------------------------------------------------------

void HexShutdown()
{
    hex::log << hex::config;
    hex::log << "============ HexShutdown =============" << hex::endl;

    HexShutdownLog();
}


//----------------------------------------------------------------------

void HexAssertShutdown(const char* assertion, const char* file, int line,
                       const char* function)
{
    std::ostringstream os;
    os << file << ":" << line << ": " << function << ": "
       << "Assertion `" << assertion << "' failed.";
    hex::log << hex::severe << os.str() << hex::endl;
    
    HexShutdown();

    abort();
}

//----------------------------------------------------------------------
