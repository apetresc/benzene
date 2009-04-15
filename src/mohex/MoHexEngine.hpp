//----------------------------------------------------------------------------
// $Id: MoHexEngine.hpp 1878 2009-01-29 03:20:00Z broderic $
//----------------------------------------------------------------------------

#ifndef MOHEXENGINE_HPP
#define MOHEXEENGINE_HPP

#include "BenzeneHtpEngine.hpp"

//----------------------------------------------------------------------------

/** Htp engine for MoHex. */
class MoHexEngine : public BenzeneHtpEngine
{
public:

    MoHexEngine(std::istream& in, std::ostream& out, 
                int boardsize, BenzenePlayer& player);
    
    ~MoHexEngine();

    /** @name Command Callbacks */
    // @{

    // The callback functions are documented in the cpp file
    void MoHexParam(HtpCommand& cmd);

    // @} // @name

#if GTPENGINE_PONDER
    virtual void Ponder();
    virtual void InitPonder();
    virtual void StopPonder();
#endif

protected:

private:

    void RegisterCmd(const std::string& name,
                     GtpCallback<MoHexEngine>::Method method);

};

//----------------------------------------------------------------------------

#endif // MOHEXENGINE_HPP
