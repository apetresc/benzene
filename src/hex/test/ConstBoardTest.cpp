#include <boost/test/auto_unit_test.hpp>

#include "ConstBoard.hpp"

//---------------------------------------------------------------------------

namespace {

BOOST_AUTO_UNIT_TEST(ConstBoard_Dimensions)
{
    BOOST_REQUIRE(MAX_WIDTH >= 5 && MAX_HEIGHT >= 7);
    ConstBoard cb = ConstBoard(1, 1);
    BOOST_CHECK_EQUAL(cb.width(), 1);
    BOOST_CHECK_EQUAL(cb.height(), 1);
    cb = ConstBoard(5);
    BOOST_CHECK_EQUAL(cb.width(), 5);
    BOOST_CHECK_EQUAL(cb.height(), 5);
    cb = ConstBoard(4, 7);
    BOOST_CHECK_EQUAL(cb.width(), 4);
    BOOST_CHECK_EQUAL(cb.height(), 7);
    cb = ConstBoard(MAX_WIDTH, MAX_HEIGHT);
    BOOST_CHECK_EQUAL(cb.width(), MAX_WIDTH);
    BOOST_CHECK_EQUAL(cb.height(), MAX_HEIGHT);
}

BOOST_AUTO_UNIT_TEST(ConstBoard_CellsLocationsValid)
{
    BOOST_REQUIRE(MAX_WIDTH >= 5 && MAX_HEIGHT >= 3);
    ConstBoard cb = ConstBoard(5, 3);
    bitset_t b1 = cb.getCells();
    BOOST_CHECK_EQUAL(b1.count(), 15);
    BOOST_CHECK(b1.test(FIRST_CELL));
    BOOST_CHECK(!b1.test(FIRST_CELL-1));
    BOOST_CHECK(!b1.test(NORTH));
    BOOST_CHECK(!b1.test(SOUTH));
    BOOST_CHECK(!b1.test(WEST));
    BOOST_CHECK(!b1.test(EAST));
    bitset_t b2 = cb.getLocations(); // adds 4 edges
    BOOST_CHECK_EQUAL(b1.count() + 4, b2.count());
    BOOST_CHECK(BitsetUtil::IsSubsetOf(b1, b2));
    BOOST_CHECK(b2.test(FIRST_EDGE));
    BOOST_CHECK(!b2.test(FIRST_EDGE-1));
    BOOST_CHECK(!b2.test(SWAP_PIECES));
    bitset_t b3 = cb.getValid(); // adds swap and resign
    BOOST_CHECK_EQUAL(b2.count() + 2, b3.count());
    BOOST_CHECK(BitsetUtil::IsSubsetOf(b2, b3));
    BOOST_CHECK(b3.test(FIRST_SPECIAL));
    BOOST_CHECK(!b3.test(FIRST_SPECIAL-1));
    
    // checking individual HexPoints
    BOOST_CHECK(cb.isValid(SWAP_PIECES));
    BOOST_CHECK(!cb.isLocation(SWAP_PIECES));
    BOOST_CHECK(cb.isLocation(NORTH));
    BOOST_CHECK(cb.isLocation(SOUTH));
    BOOST_CHECK(cb.isValid(EAST));
    BOOST_CHECK(!cb.isCell(WEST));
    BOOST_CHECK(cb.isValid(HexPointUtil::fromString("a1")));
    BOOST_CHECK(cb.isCell(HexPointUtil::fromString("a3")));
    BOOST_CHECK(cb.isLocation(HexPointUtil::fromString("e3")));
    BOOST_CHECK(!cb.isValid(INVALID_POINT));
    BOOST_CHECK(cb.isValid(RESIGN));
    BOOST_CHECK(!cb.isLocation(RESIGN));
    BOOST_CHECK(FIRST_INVALID==BITSETSIZE || !cb.isValid(FIRST_INVALID));
    BOOST_CHECK(!cb.isValid(HexPointUtil::fromString("f1")));
    BOOST_CHECK(!cb.isValid(HexPointUtil::fromString("a4")));
    BOOST_CHECK(!cb.isValid(HexPointUtil::fromString("e4")));
    
    // checking validity of bitsets
    BOOST_CHECK(cb.isValid(b1));
    BOOST_CHECK(cb.isValid(b2));
    BOOST_CHECK(cb.isValid(b3));
    BOOST_CHECK(!cb.isValid(b3.flip()));
    b3.flip(0);
    BOOST_CHECK(!cb.isValid(b3.flip()));
    b1.reset();
    b1.set(0);
    BOOST_CHECK(!cb.isValid(b1));
    b1.flip(0);
    b1.set(6);
    b1.set(7);
    BOOST_CHECK(cb.isValid(b1));
    
    // testing that FIRST_INVALID is just beyond the last valid
    // HexPoint on the largest possible board
    cb = ConstBoard(MAX_WIDTH, MAX_HEIGHT);
    BOOST_CHECK(FIRST_INVALID==BITSETSIZE);
    BOOST_CHECK(FIRST_INVALID-1==HEX_CELL_K11);
    BOOST_CHECK(cb.isValid(HEX_CELL_K11));
}

BOOST_AUTO_UNIT_TEST(ConstBoard_BitsetPacking)
{
    BOOST_REQUIRE(MAX_WIDTH >= 7 && MAX_HEIGHT >= 9);
    ConstBoard cb = ConstBoard(7, 9);
    bitset_t b1, b2;
    b2 = cb.PackBitset(b1);
    BOOST_CHECK_EQUAL(cb.UnpackBitset(b2), b1);
    b1.flip();
    b2 = cb.PackBitset(b1);
    BOOST_CHECK_EQUAL(cb.UnpackBitset(b2), b1 & cb.getCells());
    BOOST_CHECK_EQUAL(b1.count(), BITSETSIZE);
    BOOST_CHECK_EQUAL(b2.count(), cb.getCells().count());
    b1.reset();
    b1.set(SWAP_PIECES);
    b1.set(NORTH);
    b1.set(FIRST_CELL);
    int adjustment = 1;
    if (FIRST_INVALID != BITSETSIZE) {
	b1.set(FIRST_INVALID);
	adjustment = 0;
    }
    b2 = cb.PackBitset(b1);
    BOOST_CHECK_EQUAL(b1.count(), 4 - adjustment);
    BOOST_CHECK_EQUAL(b2.count(), 1);
    BOOST_CHECK_EQUAL(cb.UnpackBitset(b2), b1 & cb.getCells());
}

BOOST_AUTO_UNIT_TEST(ConstBoard_DistanceAndAdjacency)
{
    BOOST_REQUIRE(MAX_WIDTH >= 11 && MAX_HEIGHT >= 11);
    
    // distance/adjacency from point on board to edges
    ConstBoard cb = ConstBoard(1, 11);
    BOOST_CHECK_EQUAL(cb.distance(HexPointUtil::fromString("a1"), NORTH), 1);
    BOOST_CHECK(cb.isAdjacent(HexPointUtil::fromString("a1"), NORTH));
    BOOST_CHECK_EQUAL(cb.distance(HexPointUtil::fromString("a1"), SOUTH), 11);
    BOOST_CHECK(!cb.isAdjacent(HexPointUtil::fromString("a1"), SOUTH));
    BOOST_CHECK_EQUAL(cb.distance(HexPointUtil::fromString("a1"), EAST), 1);
    BOOST_CHECK(cb.isAdjacent(HexPointUtil::fromString("a1"), EAST));
    BOOST_CHECK_EQUAL(cb.distance(HexPointUtil::fromString("a1"), WEST), 1);
    BOOST_CHECK(cb.isAdjacent(HexPointUtil::fromString("a1"), WEST));
    cb = ConstBoard(8, 1);
    BOOST_CHECK_EQUAL(cb.distance(HexPointUtil::fromString("b1"), NORTH), 1);
    BOOST_CHECK(cb.isAdjacent(HexPointUtil::fromString("a1"), NORTH));
    BOOST_CHECK_EQUAL(cb.distance(HexPointUtil::fromString("b1"), SOUTH), 1);
    BOOST_CHECK(cb.isAdjacent(HexPointUtil::fromString("a1"), SOUTH));
    BOOST_CHECK_EQUAL(cb.distance(HexPointUtil::fromString("b1"), EAST), 7);
    BOOST_CHECK_EQUAL(cb.distance(HexPointUtil::fromString("b1"), WEST), 2);
    
    // distance and adjacency between two edges
    cb = ConstBoard(6, 7);
    BOOST_CHECK_EQUAL(cb.distance(NORTH, NORTH), 0);
    BOOST_CHECK(!cb.isAdjacent(NORTH, NORTH));
    BOOST_CHECK_EQUAL(cb.distance(EAST, NORTH), 1);
    BOOST_CHECK_EQUAL(cb.distance(SOUTH, NORTH), 7);
    BOOST_CHECK_EQUAL(cb.distance(WEST, EAST), 6);
    BOOST_CHECK(!cb.isAdjacent(EAST, WEST));
    BOOST_CHECK(!cb.isAdjacent(NORTH, SOUTH));
    BOOST_CHECK(cb.isAdjacent(NORTH, EAST));
    BOOST_CHECK(cb.isAdjacent(NORTH, WEST));
    BOOST_CHECK(cb.isAdjacent(SOUTH, EAST));
    BOOST_CHECK(cb.isAdjacent(SOUTH, WEST));
    
    // adjacency of two points on board
    BOOST_CHECK(!cb.isAdjacent(HexPointUtil::fromString("c6"),
			       HexPointUtil::fromString("b5")));
    BOOST_CHECK(cb.isAdjacent(HexPointUtil::fromString("c6"),
			      HexPointUtil::fromString("b6")));
    BOOST_CHECK(cb.isAdjacent(HexPointUtil::fromString("c6"),
			      HexPointUtil::fromString("b7")));
    BOOST_CHECK(cb.isAdjacent(HexPointUtil::fromString("c6"),
			      HexPointUtil::fromString("c5")));
    BOOST_CHECK(!cb.isAdjacent(HexPointUtil::fromString("c6"),
			       HexPointUtil::fromString("c6")));
    BOOST_CHECK(cb.isAdjacent(HexPointUtil::fromString("c6"),
			      HexPointUtil::fromString("c7")));
    BOOST_CHECK(cb.isAdjacent(HexPointUtil::fromString("c6"),
			      HexPointUtil::fromString("d5")));
    BOOST_CHECK(cb.isAdjacent(HexPointUtil::fromString("c6"),
			      HexPointUtil::fromString("d6")));
    BOOST_CHECK(!cb.isAdjacent(HexPointUtil::fromString("c6"),
			       HexPointUtil::fromString("d7")));
    BOOST_CHECK(cb.isAdjacent(HexPointUtil::fromString("a7"), WEST));
    BOOST_CHECK(cb.isAdjacent(HexPointUtil::fromString("a7"), SOUTH));
    
    // distance between two points on board
    cb = ConstBoard(11, 11);
    BOOST_CHECK_EQUAL(cb.distance(HexPointUtil::fromString("f4"),
				  HexPointUtil::fromString("f4")),
		      0);
    BOOST_CHECK_EQUAL(cb.distance(HexPointUtil::fromString("f4"),
				  HexPointUtil::fromString("a1")),
		      8);
    BOOST_CHECK_EQUAL(cb.distance(HexPointUtil::fromString("f4"),
				  HexPointUtil::fromString("b7")),
		      4);
    BOOST_CHECK_EQUAL(cb.distance(HexPointUtil::fromString("f4"),
				  HexPointUtil::fromString("c4")),
		      3);
    BOOST_CHECK_EQUAL(cb.distance(HexPointUtil::fromString("f4"),
				  HexPointUtil::fromString("f1")),
		      3);
    BOOST_CHECK_EQUAL(cb.distance(HexPointUtil::fromString("f4"),
				  HexPointUtil::fromString("f10")),
		      6);
    BOOST_CHECK_EQUAL(cb.distance(HexPointUtil::fromString("f4"),
				  HexPointUtil::fromString("h4")),
		      2);
    BOOST_CHECK_EQUAL(cb.distance(HexPointUtil::fromString("f4"),
				  HexPointUtil::fromString("k11")),
		      12);
}

BOOST_AUTO_UNIT_TEST(ConstBoard_RotateAndMirror)
{
    BOOST_REQUIRE(MAX_WIDTH >= 11 && MAX_HEIGHT >= 11);
    
    // rotating edges
    ConstBoard cb = ConstBoard(11, 11);
    BOOST_CHECK_EQUAL(cb.rotate(NORTH), SOUTH);
    BOOST_CHECK_EQUAL(cb.rotate(EAST), WEST);
    BOOST_CHECK_EQUAL(cb.rotate(cb.rotate(EAST)), EAST);
    
    // mirroring edges
    BOOST_CHECK_EQUAL(cb.mirror(NORTH), WEST);
    BOOST_CHECK_EQUAL(cb.mirror(EAST), SOUTH);
    BOOST_CHECK_EQUAL(cb.mirror(cb.mirror(WEST)), WEST);
    
    // rotation of points on board
    BOOST_CHECK_EQUAL(cb.rotate(HexPointUtil::fromString("f6")),
		      HexPointUtil::fromString("f6"));
    BOOST_CHECK_EQUAL(cb.rotate(HexPointUtil::fromString("a1")),
		      HexPointUtil::fromString("k11"));
    BOOST_CHECK_EQUAL(cb.rotate(HexPointUtil::fromString("b1")),
		      HexPointUtil::fromString("j11"));
    BOOST_CHECK_EQUAL(cb.rotate(HexPointUtil::fromString("a2")),
		      HexPointUtil::fromString("k10"));
    BOOST_CHECK_EQUAL(cb.rotate(HexPointUtil::fromString("d9")),
		      HexPointUtil::fromString("h3"));
    BOOST_CHECK_EQUAL(cb.rotate(HexPointUtil::fromString("h3")),
		      HexPointUtil::fromString("d9"));
    
    // mirroring points on board
    BOOST_CHECK_EQUAL(cb.mirror(HexPointUtil::fromString("f6")),
		      HexPointUtil::fromString("f6"));
    BOOST_CHECK_EQUAL(cb.mirror(HexPointUtil::fromString("a1")),
		      HexPointUtil::fromString("a1"));
    BOOST_CHECK_EQUAL(cb.mirror(HexPointUtil::fromString("b1")),
		      HexPointUtil::fromString("a2"));
    BOOST_CHECK_EQUAL(cb.mirror(HexPointUtil::fromString("a2")),
		      HexPointUtil::fromString("b1"));
    BOOST_CHECK_EQUAL(cb.mirror(HexPointUtil::fromString("d9")),
		      HexPointUtil::fromString("i4"));
    BOOST_CHECK_EQUAL(cb.mirror(HexPointUtil::fromString("h3")),
		      HexPointUtil::fromString("c8"));
    
    // rotation of points on rectangular board
    cb = ConstBoard(9, 6);
    BOOST_CHECK_EQUAL(cb.rotate(HexPointUtil::fromString("a1")),
		      HexPointUtil::fromString("i6"));
    BOOST_CHECK_EQUAL(cb.rotate(HexPointUtil::fromString("a3")),
		      HexPointUtil::fromString("i4"));
    BOOST_CHECK_EQUAL(cb.rotate(HexPointUtil::fromString("e3")),
		      HexPointUtil::fromString("e4"));
    
    // rotation of points on board of even dimensions
    cb = ConstBoard(8, 8);
    BOOST_CHECK_EQUAL(cb.rotate(HexPointUtil::fromString("d4")),
		      HexPointUtil::fromString("e5"));
    BOOST_CHECK_EQUAL(cb.rotate(HexPointUtil::fromString("d5")),
		      HexPointUtil::fromString("e4"));
    
    // mirroring points on board of even dimensions
    BOOST_CHECK_EQUAL(cb.mirror(HexPointUtil::fromString("d4")),
		      HexPointUtil::fromString("d4"));
    BOOST_CHECK_EQUAL(cb.mirror(HexPointUtil::fromString("d5")),
		      HexPointUtil::fromString("e4"));
}

BOOST_AUTO_UNIT_TEST(ConstBoard_CentrePoints)
{
    BOOST_REQUIRE(MAX_WIDTH >= 10 && MAX_HEIGHT >= 10);
    
    // centre points on odd dimension boards
    ConstBoard cb = ConstBoard(9, 9);
    BOOST_CHECK_EQUAL(cb.centerPoint(), HexPointUtil::fromString("e5"));
    BOOST_CHECK_EQUAL(cb.centerPoint(), cb.centerPointRight());
    BOOST_CHECK_EQUAL(cb.centerPoint(), cb.centerPointLeft());
    
    // centre points on even dimension boards
    cb = ConstBoard(10, 10);
    BOOST_CHECK_EQUAL(cb.centerPointLeft(), HexPointUtil::fromString("e6"));
    BOOST_CHECK_EQUAL(cb.centerPointRight(), HexPointUtil::fromString("f5"));
    
    // centre points on rectangular boards
    cb = ConstBoard(7, 10);
    BOOST_CHECK_EQUAL(cb.centerPointLeft(), HexPointUtil::fromString("d5"));
    BOOST_CHECK_EQUAL(cb.centerPointRight(), HexPointUtil::fromString("d6"));
    
    cb = ConstBoard(10, 7);
    BOOST_CHECK_EQUAL(cb.centerPointLeft(), HexPointUtil::fromString("e4"));
    BOOST_CHECK_EQUAL(cb.centerPointRight(), HexPointUtil::fromString("f4"));
}

BOOST_AUTO_UNIT_TEST(ConstBoard_CellLocationValidIterators)
{
    BOOST_REQUIRE(MAX_WIDTH >= 9 && MAX_HEIGHT >= 6);
    ConstBoard cb = ConstBoard(9, 6);
    bitset_t originalBitset, remainingBitset;
    bool allInSet, noRepeats;
    
    // testing cells iterator
    allInSet = true;
    noRepeats = true;
    originalBitset = cb.getCells();
    remainingBitset = originalBitset;
    for (BoardIterator it(cb.const_cells()); it; ++it) {
	allInSet = allInSet && originalBitset.test(*it);
	noRepeats = noRepeats && remainingBitset.test(*it);
	remainingBitset.reset(*it);
    }
    BOOST_CHECK(allInSet);
    BOOST_CHECK(noRepeats);
    BOOST_CHECK(remainingBitset.none());
    
    // testing locations iterator
    allInSet = true;
    noRepeats = true;
    originalBitset = cb.getLocations();
    remainingBitset = originalBitset;
    for (BoardIterator it(cb.const_locations()); it; ++it) {
	allInSet = allInSet && originalBitset.test(*it);
	noRepeats = noRepeats && remainingBitset.test(*it);
	remainingBitset.reset(*it);
    }
    BOOST_CHECK(allInSet);
    BOOST_CHECK(noRepeats);
    BOOST_CHECK(remainingBitset.none());
    
    // testing all iterator
    allInSet = true;
    noRepeats = true;
    originalBitset = cb.getValid();
    remainingBitset = originalBitset;
    for (BoardIterator it(cb.const_valid()); it; ++it) {
	allInSet = allInSet && originalBitset.test(*it);
	noRepeats = noRepeats && remainingBitset.test(*it);
	remainingBitset.reset(*it);
    }
    BOOST_CHECK(allInSet);
    BOOST_CHECK(noRepeats);
    BOOST_CHECK(remainingBitset.none());
}

BOOST_AUTO_UNIT_TEST(ConstBoard_NeighbourIterators)
{
    BOOST_REQUIRE(MAX_WIDTH >= 11 && MAX_HEIGHT >= 11);
    BOOST_REQUIRE(Pattern::MAX_EXTENSION >= 3);
    ConstBoard cb = ConstBoard(8, 8);
    bitset_t b;
    bool allAdjacent, allUnique;
    
    // testing immediate neighbours iterator
    allAdjacent = true;
    allUnique = true;
    for (BoardIterator it(cb.const_nbs(FIRST_CELL)); it; ++it) {
	allAdjacent = allAdjacent && cb.isAdjacent(FIRST_CELL, *it);
	allUnique = allUnique && (!b.test(*it));
	b.set(*it);
    }
    BOOST_CHECK(allAdjacent);
    BOOST_CHECK(allUnique);
    BOOST_CHECK_EQUAL(b.count(), 4);
    
    b.reset();
    allAdjacent = true;
    allUnique = true;
    for (BoardIterator it(cb.const_nbs(WEST)); it; ++it) {
	allAdjacent = allAdjacent && cb.isAdjacent(WEST, *it);
	allUnique = allUnique && (!b.test(*it));
	b.set(*it);
    }
    BOOST_CHECK(allAdjacent);
    BOOST_CHECK(allUnique);
    BOOST_CHECK_EQUAL(b.count(), cb.height()+2); // interior cells + nbr edges
    
    b.reset();
    allAdjacent = true;
    allUnique = true;
    for (BoardIterator it(cb.const_nbs(HexPointUtil::fromString("b6"))); it; ++it) {
	allAdjacent = allAdjacent && cb.isAdjacent(HexPointUtil::fromString("b6"), *it);
	allUnique = allUnique && (!b.test(*it));
	b.set(*it);
    }
    BOOST_CHECK(allAdjacent);
    BOOST_CHECK(allUnique);
    BOOST_CHECK_EQUAL(b.count(), 6);
    
    // testing radius neighbours iterator
    cb = ConstBoard(11, 11);
    int radius;
    bool allWithinRadius;
    radius = 2;
    b.reset();
    allUnique = true;
    allWithinRadius = true;
    for (BoardIterator it(cb.const_nbs(HexPointUtil::fromString("f6"), radius)); it; ++it) {
	int curDistance = cb.distance(HexPointUtil::fromString("f6"), *it);
	allWithinRadius = allWithinRadius && (0 < curDistance) && (curDistance <= radius);
	allUnique = allUnique && (!b.test(*it));
	b.set(*it);
    }
    BOOST_CHECK(allUnique);
    BOOST_CHECK(allWithinRadius);
    BOOST_CHECK_EQUAL(b.count(), 18);
    
    radius = 3;
    b.reset();
    allUnique = true;
    allWithinRadius = true;
    for (BoardIterator it(cb.const_nbs(HexPointUtil::fromString("f6"), radius)); it; ++it) {
	int curDistance = cb.distance(HexPointUtil::fromString("f6"), *it);
	allWithinRadius = allWithinRadius && (0 < curDistance) && (curDistance <= radius);
	allUnique = allUnique && (!b.test(*it));
	b.set(*it);
    }
    BOOST_CHECK(allUnique);
    BOOST_CHECK(allWithinRadius);
    BOOST_CHECK_EQUAL(b.count(), 36);
    
    b.reset();
    allUnique = true;
    allWithinRadius = true;
    for (BoardIterator it(cb.const_nbs(HexPointUtil::fromString("d3"), radius)); it; ++it) {
	int curDistance = cb.distance(HexPointUtil::fromString("d3"), *it);
	allWithinRadius = allWithinRadius && (0 < curDistance) && (curDistance <= radius);
	allUnique = allUnique && (!b.test(*it));
	b.set(*it);
    }
    BOOST_CHECK(allUnique);
    BOOST_CHECK(allWithinRadius);
    BOOST_CHECK_EQUAL(b.count(), 33);
    
    b.reset();
    allUnique = true;
    allWithinRadius = true;
    for (BoardIterator it(cb.const_nbs(SOUTH, radius)); it; ++it) {
	int curDistance = cb.distance(SOUTH, *it);
	allWithinRadius = allWithinRadius && (0 < curDistance) && (curDistance <= radius);
	allUnique = allUnique && (!b.test(*it));
	b.set(*it);
    }
    BOOST_CHECK(allUnique);
    BOOST_CHECK(allWithinRadius);
    BOOST_CHECK_EQUAL(b.count(), radius*cb.width() + 2);
    
    cb = ConstBoard(1, 1);
    b.reset();
    allUnique = true;
    allWithinRadius = true;
    for (BoardIterator it(cb.const_nbs(EAST, radius)); it; ++it) {
	int curDistance = cb.distance(EAST, *it);
	allWithinRadius = allWithinRadius && (0 < curDistance) && (curDistance <= radius);
	allUnique = allUnique && (!b.test(*it));
	b.set(*it);
    }
    BOOST_CHECK(allUnique);
    BOOST_CHECK(allWithinRadius);
    BOOST_CHECK_EQUAL(b.count(), 3); // interior cell + 2 nbg edges
    
    cb = ConstBoard(3, 8);
    b.reset();
    allUnique = true;
    allWithinRadius = true;
    for (BoardIterator it(cb.const_nbs(WEST, radius)); it; ++it) {
	int curDistance = cb.distance(WEST, *it);
	allWithinRadius = allWithinRadius && (0 < curDistance) && (curDistance <= radius);
	allUnique = allUnique && (!b.test(*it));
	b.set(*it);
    }
    BOOST_CHECK(allUnique);
    BOOST_CHECK(allWithinRadius);
    BOOST_CHECK_EQUAL(b.count(), cb.getLocations().count() - 2);
    
    radius = 2;
    b.reset();
    allUnique = true;
    allWithinRadius = true;
    for (BoardIterator it(cb.const_nbs(WEST, radius)); it; ++it) {
	int curDistance = cb.distance(WEST, *it);
	allWithinRadius = allWithinRadius && (0 < curDistance) && (curDistance <= radius);
	allUnique = allUnique && (!b.test(*it));
	b.set(*it);
    }
    BOOST_CHECK(allUnique);
    BOOST_CHECK(allWithinRadius);
    BOOST_CHECK_EQUAL(b.count(), radius*cb.height() + 2);
   
}

BOOST_AUTO_UNIT_TEST(ConstBoard_CoordsToPoint)
{
    BOOST_REQUIRE(MAX_WIDTH >= 8 && MAX_HEIGHT >= 8);
    ConstBoard cb = ConstBoard(8, 8);
    BOOST_CHECK_EQUAL(cb.coordsToPoint(-2, 0), INVALID_POINT);
    BOOST_CHECK_EQUAL(cb.coordsToPoint(0, -2), INVALID_POINT);
    BOOST_CHECK_EQUAL(cb.coordsToPoint(-1, -1), INVALID_POINT);
    BOOST_CHECK_EQUAL(cb.coordsToPoint(cb.width(), cb.height()), INVALID_POINT);
    BOOST_CHECK_EQUAL(cb.coordsToPoint(-1, cb.height()), INVALID_POINT);
    BOOST_CHECK_EQUAL(cb.coordsToPoint(cb.width(), -1), INVALID_POINT);
    BOOST_CHECK_EQUAL(cb.coordsToPoint(0, -1), NORTH);
    BOOST_CHECK_EQUAL(cb.coordsToPoint(-1, 0), WEST);
    BOOST_CHECK_EQUAL(cb.coordsToPoint(-1, cb.height()-1), WEST);
    BOOST_CHECK_EQUAL(cb.coordsToPoint(cb.width()-1, cb.height()), SOUTH);
    BOOST_CHECK_EQUAL(cb.coordsToPoint(cb.width(), cb.height()-1), EAST);
    BOOST_CHECK_EQUAL(cb.coordsToPoint(0, 0), FIRST_CELL);
    BOOST_CHECK_EQUAL(cb.coordsToPoint(cb.width()-1, cb.height()-1),
		      HexPointUtil::fromString("h8"));
}

BOOST_AUTO_UNIT_TEST(ConstBoard_PointInDir)
{
    BOOST_REQUIRE(MAX_WIDTH >= 8 && MAX_HEIGHT >= 8);
    ConstBoard cb = ConstBoard(8, 8);

    BOOST_CHECK_EQUAL(cb.PointInDir(HEX_CELL_B2, DIR_EAST), HEX_CELL_C2);
    BOOST_CHECK_EQUAL(cb.PointInDir(HEX_CELL_B2, DIR_NORTH_EAST), HEX_CELL_C1);
    BOOST_CHECK_EQUAL(cb.PointInDir(HEX_CELL_B2, DIR_NORTH), HEX_CELL_B1);
    BOOST_CHECK_EQUAL(cb.PointInDir(HEX_CELL_B2, DIR_WEST), HEX_CELL_A2);
    BOOST_CHECK_EQUAL(cb.PointInDir(HEX_CELL_B2, DIR_SOUTH_WEST), HEX_CELL_A3);
    BOOST_CHECK_EQUAL(cb.PointInDir(HEX_CELL_B2, DIR_SOUTH), HEX_CELL_B3);

    BOOST_CHECK_EQUAL(cb.PointInDir(HEX_CELL_A1, DIR_NORTH_EAST), NORTH);
    BOOST_CHECK_EQUAL(cb.PointInDir(HEX_CELL_A1, DIR_NORTH), NORTH);
    BOOST_CHECK_EQUAL(cb.PointInDir(HEX_CELL_A1, DIR_WEST), WEST);
    BOOST_CHECK_EQUAL(cb.PointInDir(HEX_CELL_A1, DIR_SOUTH_WEST), WEST);

    BOOST_CHECK_EQUAL(cb.PointInDir(NORTH, DIR_SOUTH), NORTH);
    BOOST_CHECK_EQUAL(cb.PointInDir(NORTH, DIR_EAST), NORTH);
}

BOOST_AUTO_UNIT_TEST(ConstBoard_ShiftBitset)
{
    BOOST_REQUIRE(MAX_WIDTH >= 8 && MAX_HEIGHT >= 8);
    ConstBoard cb = ConstBoard(8, 8);
    bitset_t b1, b2;
    
    b1.set(HEX_CELL_A1);
    BOOST_CHECK(cb.ShiftBitset(b1, DIR_EAST, b2));
    BOOST_CHECK(b2.test(HEX_CELL_B1));
    
    BOOST_CHECK(!cb.ShiftBitset(b1, DIR_NORTH, b2));
    BOOST_CHECK(!cb.ShiftBitset(b1, DIR_WEST, b2));

    BOOST_CHECK(cb.ShiftBitset(b1, DIR_SOUTH, b2));
    BOOST_CHECK(b2.test(HEX_CELL_A2));

}

}

//---------------------------------------------------------------------------
