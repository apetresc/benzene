//----------------------------------------------------------------------------
// $Id: FakeIntegerTest.cpp 1168 2008-02-22 00:09:30Z broderic $
//----------------------------------------------------------------------------

#include <boost/test/auto_unit_test.hpp>

#include "FakeInteger.hpp"

//---------------------------------------------------------------------------

namespace {

BOOST_AUTO_UNIT_TEST(FakeInteger_AllTests)
{
    int ia = 23;
    int arr[50];
    FakeInteger a = 7;
    FakeInteger b(-1);
    FakeInteger c = 13;

    BOOST_CHECK_EQUAL(a,  7);
    BOOST_CHECK_EQUAL(b, -1);
    BOOST_CHECK_EQUAL(c, 13);

    a = ia;
    BOOST_CHECK_EQUAL(a, ia);

    // check ++
    a++;
    BOOST_CHECK_EQUAL(a, ia+1);
    
    ++a;
    BOOST_CHECK_EQUAL(a, ia+2);

    a = b++;
    BOOST_CHECK_EQUAL(a, -1);
    BOOST_CHECK_EQUAL(b, 0);
    
    a = ++c;
    BOOST_CHECK_EQUAL(a, 14);
    BOOST_CHECK_EQUAL(c, 14);

    arr[a] = 5;
    BOOST_CHECK_EQUAL(arr[14], 5);

    // check --
    a = 10;
    b = 20;
    c = 30;
    
    a--;
    BOOST_CHECK_EQUAL(a, 9);

    --a;
    BOOST_CHECK_EQUAL(a, 8);

    a = b--;
    BOOST_CHECK_EQUAL(a, 20);
    BOOST_CHECK_EQUAL(b, 19);

    a = --c;
    BOOST_CHECK_EQUAL(a, 29);
    BOOST_CHECK_EQUAL(c, 29);

    // check +=, -=, *=, /=, %=
    a = 10;
    b = 2;
    c = 5;

    a += c;
    BOOST_CHECK_EQUAL(a, 15);

    a -= c;
    BOOST_CHECK_EQUAL(a, 10);

    a *= b;
    BOOST_CHECK_EQUAL(a, 20);

    a /= c;
    BOOST_CHECK_EQUAL(a, 4);

    a %= 3;
    BOOST_CHECK_EQUAL(a, 1);

    // check <<, >>, <<= and >>=
    b = 1;
    
    a = b << 2;
    BOOST_CHECK_EQUAL(a, 4);
    
    b = 16;
    a = b >> 3;
    BOOST_CHECK_EQUAL(a, 2);

    a <<= 2;
    BOOST_CHECK_EQUAL(a, 8);

    a >>= 3;
    BOOST_CHECK_EQUAL(a, 1);

    // check &=, |=, and ^=
    
    a = 0xffff;
    a &= 0xf0f0;
    BOOST_CHECK_EQUAL(a, 0xf0f0);
    
    a |= 0x0f0f;
    BOOST_CHECK_EQUAL(a, 0xffff);

    a ^= 1;
    BOOST_CHECK_EQUAL(a, 0xfffe);
}

}

//---------------------------------------------------------------------------
