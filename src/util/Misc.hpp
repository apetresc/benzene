//----------------------------------------------------------------------------
// $Id: Misc.hpp 1888 2009-02-01 01:04:41Z broderic $
//----------------------------------------------------------------------------

#include <sstream>
#include "Types.hpp"

//----------------------------------------------------------------------------

/** Misc. Utilities. */
namespace MiscUtil
{
    /** Converts a word to an array of bytes. */
    void WordToBytes(unsigned word, byte* out);

    /** Converts an array of four bytes into a word. */
    unsigned BytesToWord(const byte* bytes);

    /** Returns the number of bytes need to hold the given number of
        bits. Equal to (bits + 7)/8.
     */
    int NumBytesToHoldBits(int bits);

    /** Prints a vector with a space between elements. */
    template<typename TYPE>
    std::string PrintVector(const std::vector<TYPE>& v);
};

/** Prints a vector with a space between elements. */
template<typename TYPE>
std::string MiscUtil::PrintVector(const std::vector<TYPE>& v)
{
    std::ostringstream is;
    for (std::size_t i=0; i<v.size(); ++i) {
        if (i) is << ' ';
        is << v[i];
    }
    return is.str();
} 

//----------------------------------------------------------------------------
