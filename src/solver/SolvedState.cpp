//----------------------------------------------------------------------------
// $Id: SolvedState.cpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#include "Misc.hpp"
#include "SolvedState.hpp"

//----------------------------------------------------------------------------

void SolvedState::CheckCollision(const SolvedState& other) const
{
    CheckCollision(other.hash, other.black, other.white);
}

void SolvedState::CheckCollision(hash_t hash, 
                                 const bitset_t& black,
                                 const bitset_t& white) const
{
    if (this->hash == hash && 
        (this->black != black || this->white != white))
    {
        hex::log << hex::severe;
        hex::log << "HASH COLLISION!" << hex::endl;
        
        hex::log << "this:" << hex::endl;
        hex::log << HashUtil::toString(this->hash) << hex::endl;
        hex::log << HexPointUtil::ToPointListString(this->black) << hex::endl;
        hex::log << HexPointUtil::ToPointListString(this->white) << hex::endl;
        
        hex::log << "other:" << hex::endl;
        hex::log << HashUtil::toString(hash) << hex::endl;
        hex::log << HexPointUtil::ToPointListString(black) << hex::endl;
        hex::log << HexPointUtil::ToPointListString(white) << hex::endl;
        
        // want to abort even if on a release build.
        HexAssert(false);
    } 
}

//----------------------------------------------------------------------------

int SolvedState::PackedSize() const
{
    return (sizeof(win) + 
            sizeof(flags) +
            sizeof(numstates) +
            sizeof(nummoves));
}

byte* SolvedState::Pack() const
{
    static byte data[256];
    
    int index = 0;
    MiscUtil::WordToBytes(win, &data[index]);
    index += 4;

    MiscUtil::WordToBytes(flags, &data[index]);
    index += 4;
    
    MiscUtil::WordToBytes(numstates, &data[index]);
    index += 4;

    MiscUtil::WordToBytes(nummoves, &data[index]);
    index += 4;

    return data;
}

void SolvedState::Unpack(const byte* data)
{
    int index = 0;
    
    win = MiscUtil::BytesToWord(&data[index]);
    index += 4;

    flags = MiscUtil::BytesToWord(&data[index]);
    index += 4;
    
    numstates = MiscUtil::BytesToWord(&data[index]);
    index += 4;

    nummoves = MiscUtil::BytesToWord(&data[index]);
    index += 4;
}

//----------------------------------------------------------------------------
