#include <boost/test/auto_unit_test.hpp>

#include "SequenceHash.hpp"

//---------------------------------------------------------------------------

namespace {

BOOST_AUTO_UNIT_TEST(SequenceHash_Hash)
{
    MoveSequence a, b;

    BOOST_CHECK_EQUAL(SequenceHash::Hash(a), 0);
    
    a.push_back(HEX_CELL_A1);
    b.push_back(HEX_CELL_A1);
    BOOST_CHECK(SequenceHash::Hash(a) == SequenceHash::Hash(b));
    
    a.push_back(HEX_CELL_A2);
    b.push_back(HEX_CELL_A3);
    BOOST_CHECK(SequenceHash::Hash(a) != SequenceHash::Hash(b));

    // a = {1,2,3}, b = {1,3,2}
    a.push_back(HEX_CELL_A3);
    b.push_back(HEX_CELL_A2);
    BOOST_CHECK(SequenceHash::Hash(a) != SequenceHash::Hash(b));
    
    // a = {1,2,3}, b = {3,2,1}
    b.clear();
    b.push_back(HEX_CELL_A3);
    b.push_back(HEX_CELL_A2);
    b.push_back(HEX_CELL_A1);
    BOOST_CHECK(SequenceHash::Hash(a) != SequenceHash::Hash(b));

    // a = {1,2,3}, b = {}
    b.clear();
    BOOST_CHECK(SequenceHash::Hash(a) != SequenceHash::Hash(b));

    // a = {1,2,3}, b = {1}
    b.push_back(HEX_CELL_A1);
    BOOST_CHECK(SequenceHash::Hash(a) != SequenceHash::Hash(b));

    // a = {1,2,3}, b = {1,2}
    b.push_back(HEX_CELL_A2);
    BOOST_CHECK(SequenceHash::Hash(a) != SequenceHash::Hash(b));

    // a = {1,2,3}, b = {1,2,3}
    b.push_back(HEX_CELL_A3);
    BOOST_CHECK(SequenceHash::Hash(a) == SequenceHash::Hash(b));
}

}

//---------------------------------------------------------------------------
