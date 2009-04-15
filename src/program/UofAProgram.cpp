//----------------------------------------------------------------------------
// $Id: UofAProgram.cpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#include "SgSystem.h"

#include "Hex.hpp"
#include "HexProp.hpp"
#include "UofAProgram.hpp"

#include "MoHexPlayer.hpp"
#include "WolvePlayer.hpp"
#include "PerfectPlayer.hpp"
#include "HandicapPlayer.hpp"
#include "PlayerFactory.hpp"

//----------------------------------------------------------------------------

UofAProgram::UofAProgram(std::string name, 
                         std::string version, 
                         std::string build,
                         std::string date)
    : HexProgram(name, version, build, date),
      m_ice(0),
      m_solver(0)
{
}

UofAProgram::~UofAProgram()
{
}

//----------------------------------------------------------------------------

void UofAProgram::AddDefaultSettings() 
{
    HexProgram::AddDefaultSettings();

    // Some prelimenary default settings to allow us to read
    // more defaults from the config file. 
    hex::settings.put("config-data-directory",                     "config/");
    hex::settings.put("config-defaults",                         "*defaults");

    std::string path = m_executable_path;
    hex::settings.put("config-executable-path",            m_executable_path);
    std::string config_dir = hex::settings.get("config-data-directory");
    path.append(config_dir);
    hex::settings.put("config-data-path",                               path);
    std::string file = hex::settings.get("config-defaults");
    if (file[0] == '*') file = path + file.substr(1);

    hex::settings.LoadFile(file);
    hex::settings.SetCurrentAsDefaults();
}

void UofAProgram::Register(const std::string& command,
                          ArgType argtype, 
                          const std::string& usage, 
                          ArgCallback<UofAProgram>::Method method)
{
    HexProgram::Register(command, argtype, usage, 
                         new ArgCallback<UofAProgram>(this, method));
}

void UofAProgram::RegisterCmdLineArguments()
{
    HexProgram::RegisterCmdLineArguments();

    Register("samples", INT_ARGUMENT,
             "Sets the number of games played in uct search.\n"
             "Equivalent to:\n"
             " --uct-use-timelimit=false\n"
             " --uct-scale-num-games-to-size=false\n"
             " --uct-num-games=%i",
             &UofAProgram::Samples);
    
    Register("boardsize", INT_ARGUMENT,
             "Sets the size of the board to %i.\n"
             "Equivalent to 'game-default-boardsize=%i'.",
             &UofAProgram::Boardsize);

    Register("use-super-ice", NO_ARGUMENTS, 
             "Turns on all ICE features\n"
             "(Currently DOES NOT turn on permanently inferior).\n"
             "Equivalent to:\n"
             " --ice-find-presimplicial-pairs=true\n"
             " --ice-find-all-pattern-killers=true\n"
             " --ice-find-all-pattern-dominators=true\n"
             " --ice-hand-coded-enabled=true\n"
             " --ice-backup-opp-dead=true\n"
             " --ice-iterative-dead-regions=true\n"
             " --ice-three-sided-dead-regions=true\n",
             &UofAProgram::UseSuperICE);
}

std::string UofAProgram::ConfigDirectory() const
{
    std::string config_dir = hex::settings.get("config-data-directory");
    std::string config_path = m_executable_path;
    config_path.append(config_dir);
    return config_path;
}

void UofAProgram::InitializeEngines()
{
    hex::log << hex::config;
    hex::log << "============= InitializeEngines =============" << hex::endl;
    std::string config_dir = ConfigDirectory();
    m_ice = new ICEngine(config_dir);
    m_solver = new Solver();
}

void UofAProgram::InitializePlayers()
{
    hex::log << hex::config;
    hex::log << "============= InitializePlayers =============" << hex::endl;
    AddPlayer(PlayerFactory::CreatePlayerWithBook(new MoHexPlayer()));
    AddPlayer(PlayerFactory::CreatePlayerWithBook(new WolvePlayer()));
    AddPlayer(PlayerFactory::CreatePlayerWithBook(new PerfectPlayer(m_solver)));
    AddPlayer(PlayerFactory::CreateTheoryPlayer(new HandicapPlayer(m_ice)));
}

void UofAProgram::Initialize(int argc, char** argv)
{
    HexProgram::Initialize(argc, argv);
    InitializeEngines();
    InitializePlayers();
}

//----------------------------------------------------------------------------
// Shutdown stuff

void UofAProgram::ShutdownPlayers()
{
    hex::log << hex::fine << "UofAProgram::ShutdownPlayers:" << hex::endl;
    while (!m_player.empty()) {
        HexPlayer* player = m_player.back();
        m_player.pop_back();

        hex::log << player->name() << "..." << hex::endl;
        delete player;
    }
}

void UofAProgram::ShutdownEngines()
{
    hex::log << hex::fine << "UofAProgram::ShutdownEngines:" << hex::endl;    
    hex::log << "solver..." << hex::endl;
    delete m_solver;
    hex::log << "ice..." << hex::endl;
    delete m_ice;
}

void UofAProgram::Shutdown()
{
    ShutdownPlayers();
    ShutdownEngines();
    HexProgram::Shutdown();
}

//----------------------------------------------------------------------------
// Player stuff

HexPlayer* UofAProgram::FindPlayer(const std::string& name)
{
    for (unsigned i=0; i<m_player.size(); ++i) {
        if (m_player[i]->name() == name) {
            return m_player[i];
        }
    }
    return 0;
}

bool UofAProgram::AddPlayer(HexPlayer* player)
{
    if (FindPlayer(player->name())) {
        return false;
    }
    m_player.push_back(player);
    return true;
}

HexPlayer* UofAProgram::Player()
{
    std::string name =  hex::settings.get("player-name");
    HexPlayer* player = FindPlayer(name);
    if (player == NULL) {
        hex::log << hex::severe << "Could not find player '" 
                 << name << "'" << hex::endl;
    }
    HexAssert(player);
    return player;
}

//----------------------------------------------------------------------------
// Cmd-line argument helpers

bool UofAProgram::Boardsize(const std::string& arg)
{
    hex::settings.put("game-default-boardsize", arg);
    return true;
}

bool UofAProgram::Samples(const std::string& arg)
{
    // @todo Ensure argument is valid!
    hex::settings.put("uct-use-timelimit", "false");
    hex::settings.put("uct-scale-num-games-to-size", "false");
    hex::settings.put("uct-num-games", arg);
    return true;
}

bool UofAProgram::UseSuperICE(const std::string& arg)
{
    
    /** @todo Turn on permanently inferior? */
    hex::settings.put("ice-find-permanently-inferior",   "false");

    hex::settings.put("ice-find-presimplicial-pairs",    "true");
    hex::settings.put("ice-find-all-pattern-killers",    "true");
    hex::settings.put("ice-find-all-pattern-dominators", "true");
    hex::settings.put("ice-hand-coded-enabled",          "true");
    hex::settings.put("ice-backup-opp-dead",             "true");
    hex::settings.put("ice-iterative-dead-regions",      "true");
    hex::settings.put("ice-three-sided-dead-regions",    "true");
    return true;
}

//----------------------------------------------------------------------------
