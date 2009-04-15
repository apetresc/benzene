//----------------------------------------------------------------------------
// $Id: InferiorCellsTest.cpp 1358 2008-04-30 21:13:57Z broderic $
//----------------------------------------------------------------------------

#include <boost/test/auto_unit_test.hpp>

#include "Hex.hpp"
#include "InferiorCells.hpp"

//---------------------------------------------------------------------------

namespace {

BOOST_AUTO_UNIT_TEST(InferiorCells_Basic)
{
    
}

BOOST_AUTO_UNIT_TEST(InferiorCells_Dominated) 
{
    HexPoint a1 = HexPointUtil::fromString("a1");
    HexPoint b1 = HexPointUtil::fromString("b1");
    HexPoint c1 = HexPointUtil::fromString("c1");   
    HexPoint a2 = HexPointUtil::fromString("a2");
    HexPoint b2 = HexPointUtil::fromString("b2");
    HexPoint c2 = HexPointUtil::fromString("c2");
    HexPoint a3 = HexPointUtil::fromString("a3");
    HexPoint b3 = HexPointUtil::fromString("b3");  
    HexPoint c3 = HexPointUtil::fromString("c3");
    InferiorCells inf;
    bitset_t dom;

    //   a1 -> b1 <- c1
    inf.Clear();
    inf.AddDominated(a1, b1);
    inf.AddDominated(c1, b1);
    dom = inf.Dominated();
    BOOST_CHECK_EQUAL(dom.count(), 2u);
    BOOST_CHECK(dom.test(a1));
    BOOST_CHECK(dom.test(c1));
    BOOST_CHECK(!dom.test(b1));
                
    //   a1 <- b1 -> c1
    inf.Clear();
    inf.AddDominated(b1, a1);
    inf.AddDominated(b1, c1);
    dom = inf.Dominated();
    BOOST_CHECK_EQUAL(dom.count(), 1u);
    BOOST_CHECK(!dom.test(a1));
    BOOST_CHECK(!dom.test(c1));
    BOOST_CHECK(dom.test(b1));
    
    //
    //   a1 -> b1 -> c1  (a1, b1 should be dominated).
    //  
    //   a2 -> b2 -> c2 -> c3  (a2 should be dominated).
    //              (vul)
    //
    inf.Clear();
    inf.AddDominated(a1, b1);
    inf.AddDominated(b1, c1);

    inf.AddDominated(a2, b2);
    inf.AddDominated(b2, c2);
    inf.AddVulnerable(c2, c3);

    dom = inf.Dominated();
    BOOST_CHECK(dom.test(a1));
    BOOST_CHECK(dom.test(b1));
    BOOST_CHECK(!dom.test(c1));
    
    BOOST_CHECK(dom.test(a2));
    BOOST_CHECK(!dom.test(b2));
    BOOST_CHECK(!dom.test(c2));    
    BOOST_CHECK(!dom.test(c3));

    // 
    //    +----+ 
    //    v    |
    //   a1 -> b1
    // 
    inf.Clear();
    inf.AddDominated(a1, b1);
    inf.AddDominated(b1, a1);
    dom = inf.Dominated();
    BOOST_CHECK_EQUAL(dom.count(), 1u);
    BOOST_CHECK(dom.test(a1) != dom.test(b1));

    //
    //    +----------+
    //    v          |
    //   a1 -> b1 -> c1 
    //         ^
    //         a2
    //         ^
    //         b2
    // 
    inf.Clear();
    inf.AddDominated(a1, b1);
    inf.AddDominated(b1, c1);
    inf.AddDominated(c1, a1);
    inf.AddDominated(b2, a2);
    inf.AddDominated(a2, b1);
    dom = inf.Dominated();

    BOOST_CHECK_EQUAL(dom.count(), 4u);
    BOOST_CHECK(dom.test(b2));
    BOOST_CHECK(dom.test(a2));
    BOOST_CHECK(!dom.test(a1) || !dom.test(b1) || !dom.test(c1));

    //
    //    +----------+      +----+
    //    v          |      v    |
    //   a1 -> b1 -> c1 -> a3 -> b3
    //         ^
    //         a2
    //         ^
    //         b2
    // 
    inf.AddDominated(c1, a3);
    inf.AddDominated(a3, b3);
    inf.AddDominated(b3, a3);
    dom = inf.Dominated();
    
    BOOST_CHECK_EQUAL(dom.count(), 6u);
    BOOST_CHECK(dom.test(b2));
    BOOST_CHECK(dom.test(a2));
    BOOST_CHECK(dom.test(a1));
    BOOST_CHECK(dom.test(b1));
    BOOST_CHECK(dom.test(c1));
    BOOST_CHECK(dom.test(a3) != dom.test(b3));


    // 
    //    +----+    
    //    v    |    
    //   a1 -> b1 -> c1
    //          ^    |
    //          +----+ 
    // 
    inf.Clear();
    inf.AddDominated(a1, b1);
    inf.AddDominated(b1, a1);
    inf.AddDominated(b1, c1);
    inf.AddDominated(c1, b1);
    dom = inf.Dominated();
    BOOST_CHECK_EQUAL(dom.count(), 2u);
    BOOST_CHECK(!dom.test(a1) || !dom.test(b1) || !dom.test(c1));
    
}

}

//---------------------------------------------------------------------------
