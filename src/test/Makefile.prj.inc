#-----------------------------------------------------------------------------
# GNU Makefile for unit tests: project dependent part
#
# $Id: Makefile.prj.inc 1657 2008-09-15 23:32:09Z broderic $
#-----------------------------------------------------------------------------

NAME = hex-test
TARG_NAME = hex-test
BUILD = ../../build

VPATH = ../..

EXPDIR = ../../fuego-0.1
OBJDIR = $(BUILD)/obj/$(NAME)
BINDIR = ../../bin

PROJ_CXXFLAGS = \
    -I../ \
    -I../util \
    -I../hex/ -I../hex/eval/ -I../hex/patterns/ \
    -I../uct/ -I../solver/ \
    -I../player/ -I../program/ -I../htp/ \
    -I$(EXPDIR)/smartgame

# always link with release version of fuego-0.1
EXPLIB = $(EXPDIR)/build/gmake/build/release

PROJ_LNFLAGS = -L$(BUILD) -L$(EXPLIB)
PROJ_LIB = -lhtp -lprogram -lplayer -lsolver -luct -lhex -lutil \
    -lfuego_smartgame \
    -ldb -lboost_thread -lboost_unit_test_framework

PROJ_DEP = \
    $(BUILD)/libutil.a \
    $(BUILD)/libhex.a \
    $(BUILD)/libuct.a \
    $(BUILD)/libsolver.a \
    $(BUILD)/libplayer.a \
    $(BUILD)/libprogram.a \
    $(BUILD)/libhtp.a \
    $(EXPLIB)

default : all

SRC_CPP= \
  TestMain.cpp\
  ../util/test/BitsetTest.cpp\
  ../util/test/ChangeLogTest.cpp\
  ../util/test/DigraphTest.cpp\
  ../util/test/FakeIntegerTest.cpp\
  ../util/test/HashTest.cpp\
  ../util/test/HashTableTest.cpp\
  ../util/test/LoggerTest.cpp\
  ../util/test/SortedSequenceTest.cpp\
  ../util/test/TimeTest.cpp\
  ../util/test/UnionFindTest.cpp\
  ../hex/patterns/test/RingGodelTest.cpp\
  ../hex/test/BitsetIteratorTest.cpp\
  ../hex/test/BoardIteratorTest.cpp\
  ../hex/test/BoardUtilsTest.cpp\
  ../hex/test/ConnectionsTest.cpp\
  ../hex/test/ConstBoardTest.cpp\
  ../hex/test/GroupBoardTest.cpp\
  ../hex/test/HexColorTest.cpp\
  ../hex/test/HexPointTest.cpp\
  ../hex/test/ICEngineTest.cpp\
  ../hex/test/InferiorCellsTest.cpp\
  ../hex/test/SequenceHashTest.cpp\
  ../hex/test/StoneBoardTest.cpp\
  ../hex/test/VCListTest.cpp\
  ../hex/test/VCTest.cpp\
  ../hex/test/VCUtilsTest.cpp\
  ../hex/test/ZobristHashTest.cpp
