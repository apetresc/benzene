//----------------------------------------------------------------------------
// $Id: Misc.cpp 1888 2009-02-01 01:04:41Z broderic $
//----------------------------------------------------------------------------

#include "Misc.hpp"

//----------------------------------------------------------------------------

void MiscUtil::WordToBytes(unsigned word, byte* out)
{
    for (int i=0; i<4; i++) {
        out[i] = static_cast<byte>(word & 0xff);
        word>>=8;
    }
}

unsigned MiscUtil::BytesToWord(const byte* bytes)
{
    unsigned ret=0;
    for (int i=3; i>=0; i--) {
        ret <<= 8;
        ret |= bytes[i];
    }
    return ret;
}

int MiscUtil::NumBytesToHoldBits(int bits)
{
    return (bits+7)/8;
}

//----------------------------------------------------------------------------

