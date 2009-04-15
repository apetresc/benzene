//----------------------------------------------------------------------------
// $Id: GroupBoardTest.cpp 1122 2008-01-18 00:27:16Z broderic $
//----------------------------------------------------------------------------

#include <boost/test/auto_unit_test.hpp>

#include "GroupBoard.hpp"
#include "BoardIterator.hpp"

//---------------------------------------------------------------------------

namespace {

BOOST_AUTO_UNIT_TEST(GroupBoard_Captains)
{
    BOOST_REQUIRE(MAX_WIDTH >= 5 && MAX_HEIGHT >= 5);
    GroupBoard gb = GroupBoard(5, 5);
    gb.startNewGame();

    // on empty board, all edges and cells are captains
    // of themselves.
    gb.absorb();
    BOOST_CHECK(gb.getCaptain(NORTH) == NORTH);
    BOOST_CHECK(gb.getCaptain(SOUTH) == SOUTH);
    BOOST_CHECK(gb.getCaptain(EAST) == EAST);
    BOOST_CHECK(gb.getCaptain(WEST) == WEST);
    for (BoardIterator p(gb.const_cells()); p; ++p) {
        BOOST_CHECK(gb.getCaptain(*p) == *p);
    }

    // check that first cell is absorbed into the north group;
    // and that NORTH is always the captain of its group. 
    gb.playMove(BLACK, FIRST_CELL);
    gb.absorb();
    BOOST_CHECK(gb.getCaptain(NORTH) == NORTH);
    BOOST_CHECK(gb.getCaptain(FIRST_CELL) == NORTH);

}

BOOST_AUTO_UNIT_TEST(GroupBoard_Nbs)
{
    BOOST_REQUIRE(MAX_WIDTH >= 5 && MAX_HEIGHT >= 5);
    GroupBoard gb = GroupBoard(5, 5);
    gb.startNewGame();

    //  a  b  c  d  e  
    // 1\.  .  .  .  .\1
    //  2\W  B  .  .  .\2
    //   3\.  B  .  .  .\3
    //    4\.  .  .  .  .\4
    //     5\.  .  .  .  .\5
    //        a  b  c  d  e  
    HexPoint a1 = HexPointUtil::fromString("a1");
    HexPoint b1 = HexPointUtil::fromString("b1");
    HexPoint c1 = HexPointUtil::fromString("c1");   
    HexPoint a2 = HexPointUtil::fromString("a2");
    HexPoint b2 = HexPointUtil::fromString("b2");
    HexPoint c2 = HexPointUtil::fromString("c2");
    HexPoint a3 = HexPointUtil::fromString("a3");
    HexPoint b3 = HexPointUtil::fromString("b3");  
    HexPoint c3 = HexPointUtil::fromString("c3");
    HexPoint a4 = HexPointUtil::fromString("a4");
    HexPoint b4 = HexPointUtil::fromString("b4");
    HexPoint a5 = HexPointUtil::fromString("a5");    

    bitset_t nbs;    
    gb.playMove(BLACK, b2);
    gb.playMove(WHITE, a2);
    gb.playMove(BLACK, b3);
    
    gb.absorb();

    nbs = gb.Nbs(b2, EMPTY);
    BOOST_CHECK_EQUAL(nbs.count(), 7);
    BOOST_CHECK(nbs.test(b1));
    BOOST_CHECK(nbs.test(c1));
    BOOST_CHECK(nbs.test(c2));
    BOOST_CHECK(nbs.test(a3));
    BOOST_CHECK(nbs.test(c3));
    BOOST_CHECK(nbs.test(a4));
    BOOST_CHECK(nbs.test(b4));

    nbs = gb.Nbs(b2, WHITE);
    BOOST_CHECK_EQUAL(nbs.count(), 1);
    BOOST_CHECK(nbs.test(gb.getCaptain(a2)));
   
    nbs = gb.Nbs(c2, BLACK);
    BOOST_CHECK_EQUAL(nbs.count(), 1);
    BOOST_CHECK(nbs.test(gb.getCaptain(b2)));

    nbs = gb.Nbs(a2, EMPTY);
    BOOST_CHECK_EQUAL(nbs.count(), 5);
    BOOST_CHECK(nbs.test(a1));
    BOOST_CHECK(nbs.test(b1));
    BOOST_CHECK(nbs.test(a3));
    BOOST_CHECK(nbs.test(a4));
    BOOST_CHECK(nbs.test(a5));

    nbs = gb.Nbs(a3, EMPTY);
    BOOST_CHECK(nbs.count() == 1);
    BOOST_CHECK(nbs.test(a4));

    //
    // Test ComputeDigraph()
    //
    PointToBitset dg;
    gb.ComputeDigraph(BLACK, dg);

    BOOST_CHECK(dg[gb.getCaptain(b2)] == 
                gb.Nbs(gb.getCaptain(b2), EMPTY));

    BOOST_CHECK_EQUAL(dg[a3].count(), 6);
    BOOST_CHECK(dg[a3].test(b1));
    BOOST_CHECK(dg[a3].test(c1));
    BOOST_CHECK(dg[a3].test(c2));
    BOOST_CHECK(dg[a3].test(c3));
    BOOST_CHECK(dg[a3].test(a4));
    BOOST_CHECK(dg[a3].test(b4));

    gb.ComputeDigraph(WHITE, dg);

    BOOST_CHECK(dg[gb.getCaptain(a2)] == 
                gb.Nbs(gb.getCaptain(a2), EMPTY));

    BOOST_CHECK(dg[gb.getCaptain(c3)] == 
                gb.Nbs(gb.getCaptain(c3), EMPTY));

    BOOST_CHECK_EQUAL(dg[b1].count(), 5);
    BOOST_CHECK(dg[b1].test(a1));
    BOOST_CHECK(dg[b1].test(c1));
    BOOST_CHECK(dg[b1].test(a3));
    BOOST_CHECK(dg[b1].test(a4));
    BOOST_CHECK(dg[b1].test(a5));
}

}
