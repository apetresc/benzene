//---------------------------------------------------------------------------
// $Id: ConnectionsTest.cpp 1657 2008-09-15 23:32:09Z broderic $
//---------------------------------------------------------------------------

#include <boost/test/auto_unit_test.hpp>

#include "BitsetIterator.hpp"
#include "Connections.hpp"
#include "ChangeLog.hpp"

//---------------------------------------------------------------------------

namespace {

BOOST_AUTO_UNIT_TEST(Connections_CheckRevert)
{
    GroupBoard bd(11, 11);

    HexPoint a9 = HexPointUtil::fromString("a9");
    HexPoint f5 = HexPointUtil::fromString("f5");
    HexPoint i4 = HexPointUtil::fromString("i4");
    HexPoint h6 = HexPointUtil::fromString("h6");

    bd.startNewGame();
    bd.playMove(BLACK, a9);
    bd.playMove(WHITE, f5);
    bd.playMove(BLACK, i4);
    bd.playMove(WHITE, h6);

    hex::settings.put("vc-default-max-ors", "4");
    hex::settings.put("vc-full-soft-limit", "10");
    hex::settings.put("vc-semi-soft-limit", "25");
    hex::settings.put("vc-and-over-edge", "true");
    hex::settings.put("vc-use-greedy-union", "true");

    ChangeLog<VC> cl1, cl2;
    VCPatternSet emptyPatternSet;
    Connections con1(bd, BLACK, emptyPatternSet, cl1);
    Connections con2(bd, BLACK, emptyPatternSet, cl2);

    con1.build();
    con2.build();
    BOOST_CHECK(con1 == con2);

    for (BitsetIterator p(bd.getEmpty()); p; ++p) {
        bitset_t added[BLACK_AND_WHITE];
        added[BLACK].set(*p);

        bd.playMove(BLACK, *p);
        con2.build(added);

        con2.revert();
        bd.undoMove(*p);

        BOOST_CHECK(con1 == con2);
    }
}

}

//---------------------------------------------------------------------------
