//----------------------------------------------------------------------------
// $Id: Bitset.hpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#ifndef BITSET_HPP
#define BITSET_HPP

#include <cassert>
#include <bitset>
#include <map>
#include <set>
#include <sstream>
#include <string>

#include "Types.hpp"

//----------------------------------------------------------------------------

/** Should always be a multiple of sizeof(int). 

    Very important. Only mess with this if you know what you are
    doing!
*/
static const int BITSETSIZE = 128;

//----------------------------------------------------------------------------

/** Makes IsLessThan() and IsSubsetOf() faster. */
#define OPTIMIZED_BITSET 1

//----------------------------------------------------------------------------

/** Standard-sized bitset. */
typedef std::bitset<BITSETSIZE> bitset_t;

/** Macro for empty bitset. */
#define EMPTY_BITSET  (bitset_t())

//----------------------------------------------------------------------------

/** Converts a bitset into a base 64 number. 
    @note Currently not used. */
std::string BitsetToBase64(const bitset_t& b);

/** Converts a string of base 64 symbols into a bitset. 
    @note Currently not used. */
bitset_t Base64ToBitset(const std::string& str);

//----------------------------------------------------------------------------

/** Utilities on bitsets. */
namespace BitsetUtil 
{
    /** Converts the bottom numbits of b into a byte stream. */
    void BitsetToBytes(const bitset_t& b, byte* out, int numbits);
    
    /** Converts a byte stream into a bitset. */
    bitset_t BytesToBitset(const byte* bytes, int numbits);

    /** Converts a bitset into a string of hex symbols. */
    std::string BitsetToHex(const bitset_t& b, int numbits);

    /** Converts a string of hex symbols into a bitset. */
    bitset_t HexToBitset(const std::string& str);

    //------------------------------------------------------------------------

    /** Subtracts b2 from b1. */
    bitset_t Subtract(const bitset_t& b1, const bitset_t& b2);

    /** If removeFrom - remove is not empty, stores that value in
        removeFrom and returns true.  Otherwise, removeFrom is not
        changed and returns false. */
    bool SubtractIfLeavesAny(bitset_t& removeFrom, const bitset_t& remove);


    /** Returns true if b1 is a subset of b2. */
    bool IsSubsetOf(const bitset_t& b1, const bitset_t& b2);
    
    /** Returns true if b1 comes before b2 in some consistent order
        (any well defined ordering, not necessarily lexicographic). */
    bool IsLessThan(const bitset_t& b1, const bitset_t& b2);

    //------------------------------------------------------------------------

    /** Stores indices of set bits in b in indices. 
        @note INT must be convertible to an int.
    */
    template<typename INT>
    void BitsetToVector(const bitset_t& b, std::vector<INT>& indices);

    /** Converts of set of indices into a bitset with those bits set. 
        @note INT must be convertible to an int. 
     */
    template<typename INT>
    bitset_t SetToBitset(const std::set<INT>& indices);
    
    //------------------------------------------------------------------------

    /** Returns the bit that is set in b. */
    int FindSetBit(const bitset_t& b);

    /** Returns least-significant set bit in b. */
    int FirstSetBit(const bitset_t& b);
}

//----------------------------------------------------------------------------

template<typename INT>
void BitsetUtil::BitsetToVector(const bitset_t& b, 
                                std::vector<INT>& indices)
{
    indices.clear();
    for (int i=0; i<BITSETSIZE; i++) {
        if (b.test(i))
            indices.push_back(static_cast<INT>(i));
    }
    assert(b.count() == indices.size());
}

template<typename INT>
bitset_t BitsetUtil::SetToBitset(const std::set<INT>& indices)
{
    bitset_t ret;
    typedef typename std::set<INT>::const_iterator const_iterator;
    for (const_iterator it=indices.begin(); 
         it != indices.end();
         ++it) 
    {
        ret.set(static_cast<int>(*it));
    }
    return ret;
}

//----------------------------------------------------------------------------

inline bool BitsetUtil::IsSubsetOf(const bitset_t& b1, const bitset_t& b2)
{
#if OPTIMIZED_BITSET
    const void* vb1 = &b1;
    const void* vb2 = &b2;
    const int* ib1 = (const int*)vb1;
    const int* ib2 = (const int*)vb2;
    static const int num = sizeof(bitset_t)/sizeof(int);
    for (int i=0; i<num; ++i, ++ib1, ++ib2) {
        if (*ib1 & ~(*ib2))
            return false;
    }
    return true;
#else
    return ((b1 & b2) == b1);
#endif
}

inline bool BitsetUtil::IsLessThan(const bitset_t& b1, const bitset_t& b2)
{
#if OPTIMIZED_BITSET
    const void* vb1 = &b1;
    const void* vb2 = &b2;
    const int* ib1 = (const int*)vb1;
    const int* ib2 = (const int*)vb2;
    const static int num = sizeof(bitset_t)/sizeof(int);
    for (int i=0; i<num; ++i, ++ib1, ++ib2) {
        if (*ib1 != *ib2) 
            return (*ib1 < *ib2);
    }
    return false;
#else 
    for (int i=0; i<BITSETSIZE; i++) {
        if (b1[i] != b2[i]) 
            return !b1[i];
    }
    return false;
#endif
}

//----------------------------------------------------------------------------

/** Extends the standard binary '-' operator for bitsets. */
bitset_t operator-(const bitset_t& b1, const bitset_t& b2);

//----------------------------------------------------------------------------

#endif // BITSET_HPP
