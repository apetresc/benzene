//----------------------------------------------------------------------------
// $Id: Hex.hpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#ifndef HEX_HPP
#define HEX_HPP

//----------------------------------------------------------------------------
// c++ and stl stuff

#include <set>
#include <map>
#include <list>
#include <vector>
#include <queue>
#include <bitset>
#include <string>
#include <utility>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <functional>

/** Marks a parameter as unusable and suppresses compiler warning to
    that it is unused. */
#ifdef UNUSED
#elif defined(__GNUC__)
# define UNUSED(x) UNUSED_ ## x __attribute__((unused))
#elif defined(__LCLINT__)
# define UNUSED(x) /*@unused@*/ x
#else
# define UNUSED(x) x
#endif

//----------------------------------------------------------------------------
// stuff from src/util/

#include "Types.hpp"
#include "Hash.hpp"
#include "Logger.hpp"
#include "Bitset.hpp"

//----------------------------------------------------------------------------
// hex stuff

#include "HexVersion.hpp"
#include "HexAssert.hpp"
#include "HexColor.hpp"
#include "HexPoint.hpp"
#include "HexSettings.hpp"

namespace hex
{
    /** Global settings; used throughout the hex system. */
    extern HexSettings settings;

    /** Master logger. */
    extern Logger log;
};

//----------------------------------------------------------------------------

#endif // HEX_HPP
