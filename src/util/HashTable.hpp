//----------------------------------------------------------------------------
// $Id: HashTable.hpp 1011 2007-11-23 19:55:18Z broderic $
//----------------------------------------------------------------------------

#ifndef HASHMAP_H
#define HASHMAP_H

#include <boost/concept_check.hpp>

#include <vector>
#include "Hash.hpp"

//----------------------------------------------------------------------------

namespace hex
{

/** Concept of a state in a hash table. */
template<class T>
struct HashTableStateConcept
{
    void constraints() 
    {
        boost::function_requires< boost::DefaultConstructibleConcept<T> >();
        boost::function_requires< boost::AssignableConcept<T> >();
    }
};

}


//----------------------------------------------------------------------------

/** 
    The HashTable has 2^n slots, each slot contains room for a single
    element of type T.  The table is initialized with elements using
    T's default constructor.
*/
template<typename T>
class HashTable
{
    BOOST_CLASS_REQUIRE(T, hex, HashTableStateConcept);

public:

    /** Constructs a hashmap with 2^bits entries. */
    HashTable(unsigned bits);

    /** Destructor. */
    virtual ~HashTable();

    /** Returns the number of entries in the hash map. */
    unsigned size() const;

    /** Returns access to element in slot for hash. */
    T& operator[] (hash_t hash);

    /** Clears the table. */
    void clear();

private:    
    unsigned m_size;
    unsigned m_mask;
    std::vector<T> m_data;
};

template<typename T>
HashTable<T>::HashTable(unsigned bits)
    : m_size(1 << bits),
      m_mask(m_size - 1),
      m_data(m_size)
{
}

template<typename T>
HashTable<T>::~HashTable()
{
}

template<typename T>
inline unsigned HashTable<T>::size() const
{
    return m_size;
}

template<typename T>
T& HashTable<T>::operator[](hash_t hash)
{
    return m_data[hash & m_mask];
}

template<typename T>
void HashTable<T>::clear()
{
    m_data = std::vector<T>(m_size);
}

#endif // HASHMAP_H

//----------------------------------------------------------------------------

