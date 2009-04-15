//---------------------------------------------------------------------------
// $Id: BoardUtilsTest.cpp 1657 2008-09-15 23:32:09Z broderic $
//---------------------------------------------------------------------------
#include <boost/test/auto_unit_test.hpp>

#include "BoardUtils.hpp"

//---------------------------------------------------------------------------

namespace {

BOOST_AUTO_UNIT_TEST(BoardUtil_RandomEmptyCell)
{
    HexPoint p;
    BOOST_REQUIRE(MAX_WIDTH >= 2 && MAX_HEIGHT >= 2);
    
    // test under normal conditions
    StoneBoard sb = StoneBoard(2, 2);
    
    p = BoardUtils::RandomEmptyCell(sb);
    BOOST_CHECK(sb.isCell(p));
    sb.startNewGame();
    BOOST_CHECK(!sb.isLegal(SWAP_PIECES));
    p = BoardUtils::RandomEmptyCell(sb);
    BOOST_CHECK(sb.isCell(p));
    sb.playMove(BLACK, HEX_CELL_A1);
    BOOST_CHECK(sb.isLegal(SWAP_PIECES));
    sb.playMove(WHITE, HEX_CELL_A2);
    BOOST_CHECK(!sb.isLegal(SWAP_PIECES));
    BOOST_CHECK_EQUAL(sb.getPlayed().count(), 6);
    BOOST_CHECK(!sb.isEmpty(HEX_CELL_A1));
    BOOST_CHECK(!sb.isEmpty(HEX_CELL_A2));
    
    p = BoardUtils::RandomEmptyCell(sb);
    BOOST_CHECK(sb.isCell(p));
    BOOST_CHECK(sb.isEmpty(p));
    BOOST_CHECK(p != HEX_CELL_A1);
    BOOST_CHECK(p != HEX_CELL_A2);
    
    // test when one cell left
    sb = StoneBoard(1, 1);
    sb.startNewGame();
    p = BoardUtils::RandomEmptyCell(sb);
    BOOST_CHECK_EQUAL(p, HEX_CELL_A1);
    
    // test when no cells left
    sb = StoneBoard(1, 1);
    sb.playMove(BLACK, HEX_CELL_A1);
    p = BoardUtils::RandomEmptyCell(sb);
    BOOST_CHECK_EQUAL(p, INVALID_POINT);
    
    // test when game has been resigned
    sb = StoneBoard(1, 1);
    sb.startNewGame();
    sb.playMove(WHITE, RESIGN);
    BOOST_CHECK(!sb.isLegal(HEX_CELL_A1));
    p = BoardUtils::RandomEmptyCell(sb);
    BOOST_CHECK_EQUAL(p, HEX_CELL_A1);
}

} // namespace

//---------------------------------------------------------------------------
