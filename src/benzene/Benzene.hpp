//----------------------------------------------------------------------------
// $Id: Benzene.hpp 633 2007-08-23 21:45:54Z broderic $
//----------------------------------------------------------------------------

#ifndef BENZENE_H
#define BENZENE_H

#include "UofAProgram.hpp"

//----------------------------------------------------------------------------

/** Benzene hex playing/solving program. */
class Benzene : public UofAProgram
{
public:

    Benzene();
    virtual ~Benzene();

    virtual void Initialize(int argc, char** argv);

protected:
    virtual void AddDefaultSettings();
    virtual void RegisterCmdLineArguments();
};

#endif // BENZENE_H

//----------------------------------------------------------------------------
