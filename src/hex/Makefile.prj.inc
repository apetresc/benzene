#-----------------------------------------------------------------------------
# GNU Makefile for static libraries: project dependent part
#
# $Id: Makefile.prj.inc 1657 2008-09-15 23:32:09Z broderic $
#-----------------------------------------------------------------------------

NAME = hex
TARG_NAME = libhex.a
BUILD = ../../build

VPATH = ../..

EXPDIR = ../../fuego-0.1
OBJDIR = $(BUILD)/obj/$(NAME)
BINDIR = $(BUILD)

PROJ_CXXFLAGS = -I../util -I. -I./eval/ -I./patterns/ -I$(EXPDIR)/smartgame
PROJ_LNFLAGS =
PROJ_LIB =
PROJ_DEP = $(BUILD)/libutil.a

default : all

SRC_CPP= \
  eval/HexEval.cpp \
  eval/Resistance.cpp \
  eval/TwoDistance.cpp \
  patterns/HashedPatternSet.cpp \
  patterns/Pattern.cpp \
  patterns/PatternBoard.cpp \
  patterns/RingGodel.cpp \
  BoardUtils.cpp \
  Connections.cpp \
  ConstBoard.cpp \
  Game.cpp \
  GraphUtils.cpp \
  GroupBoard.cpp \
  HandCodedPattern.cpp \
  HexAbSearch.cpp \
  HexBoard.cpp \
  HexInit.cpp \
  HexPoint.cpp \
  HexProp.cpp \
  HexSettings.cpp \
  HexSgUtil.cpp \
  ICEngine.cpp \
  IcePatternSet.cpp \
  InferiorCells.cpp \
  OpeningBook.cpp \
  PlayerUtils.cpp \
  SequenceHash.cpp \
  StoneBoard.cpp \
  VC.cpp \
  VCList.cpp \
  VCPattern.cpp \
  VCUtils.cpp \
  ZobristHash.cpp

