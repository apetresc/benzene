//----------------------------------------------------------------------------
// $Id: UofAProgram.hpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#ifndef UOFAPROGRAM_HPP
#define UOFAPROGRAM_HPP

#include "Hex.hpp"
#include "HexProgram.hpp"
#include "ICEngine.hpp"
#include "Solver.hpp"
#include "HexPlayer.hpp"
#include "HexUctPolicy.hpp"

//----------------------------------------------------------------------------

/**
   HexProgram with stuff particular to our set of Programs. 
*/
class UofAProgram : public HexProgram
{
public:

    /** Creates a hex program of the given name, version, build# and date.
        Initalizes the Hex library. */
    UofAProgram(std::string name, std::string version, 
                std::string build, std::string date);

    /** Destroys program. */
    virtual ~UofAProgram();

    //----------------------------------------------------------------------

    /** Parses cmd-line arguments, starts up Hex system, etc. */
    virtual void Initialize(int argc, char **argv);

    /** Adds a player to the list of players. Returns true on success,
        false on failure (duplicate player). */
    virtual bool AddPlayer(HexPlayer* player);
    
    /** Shuts down the program. */
    virtual void Shutdown();

    //----------------------------------------------------------------------

    /** Returns a string containing the path to the config directory. 
        This can be used to get the correct path to files in the /bin/config/
        directory. */
    std::string ConfigDirectory() const;

    //----------------------------------------------------------------------

    /** Returns pointer to the Inferior-Cell engine. */
    ICEngine* ICE();

    /** Returns pointer to solver engine. */
    Solver* GetSolver();

    /** Returns pointer to currently selected player. 
        This is controlled by the setting "player-name".
        This method looks through the list of known players
        for a player with that name.  Returns 0 if player
        is not found. */
    HexPlayer* Player();

    //----------------------------------------------------------------------

protected:

    /** Adds the programs default settings to the global list
        of settings. */
    virtual void AddDefaultSettings();

    /** Registers all command-line arguments. */
    virtual void RegisterCmdLineArguments();

    //----------------------------------------------------------------------

    /** Tries to find a player with the given name in the list of
        players.  Returns pointer to player on success, NULL on
        failure. */
    HexPlayer* FindPlayer(const std::string& name);

    //----------------------------------------------------------------------

    /** Cmd-line options. */
    bool Boardsize(const std::string& arg);
    bool Samples(const std::string& arg);
    bool UseSuperICE(const std::string& arg);

    //----------------------------------------------------------------------

    ICEngine* m_ice;
    Solver* m_solver;

    /** List of known players. */
    std::vector<HexPlayer*> m_player;

private:

    /** Instantiates the engines needed by this program. */
    void InitializeEngines();
    void ShutdownEngines();

    void InitializePlayers();
    void ShutdownPlayers();
    
    /** Returns true if the command is registered. */
    bool IsRegistered(const std::string& command) const;
    
    /** Registers a command-line argument with this program. */
    void Register(const std::string& command, 
                  ArgType type, 
                  const std::string& usage, 
                  ArgCallback<UofAProgram>::Method method);

};

inline ICEngine* UofAProgram::ICE() 
{
    return m_ice;
}

inline Solver* UofAProgram::GetSolver()
{
    return m_solver;
}

//----------------------------------------------------------------------------

#endif // UOFAPROGRAM_HPP
