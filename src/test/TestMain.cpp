//----------------------------------------------------------------------------
// $Id: TestMain.cpp 1657 2008-09-15 23:32:09Z broderic $
//----------------------------------------------------------------------------

#include <cstdlib>
#include <boost/test/auto_unit_test.hpp>
#include <boost/version.hpp>

using namespace boost::unit_test;

//----------------------------------------------------------------------------

test_suite* init_unit_test_suite(int argc, char* argv[])
{
    return boost::unit_test::ut_detail::auto_unit_test_suite();
}

//----------------------------------------------------------------------------

