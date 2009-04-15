//----------------------------------------------------------------------------
// $Id: Misc.hpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

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
};

//----------------------------------------------------------------------------
