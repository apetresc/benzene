#-----------------------------------------------------------------------------
# GNU Makefile for static libraries: project dependent part
#
# $Id: Makefile.prj.inc 1715 2008-10-28 22:13:37Z broderic $
#-----------------------------------------------------------------------------

NAME = player
TARG_NAME = libplayer.a
BUILD = ../../build

VPATH = ../..

EXPDIR = ../../fuego-0.1
OBJDIR = $(BUILD)/obj/$(NAME)
BINDIR = $(BUILD)

PROJ_CXXFLAGS = -I../util -I../hex/ -I../hex/eval/ -I../hex/patterns/ -I../uct/ -I../solver/ -I$(EXPDIR)/smartgame
PROJ_LNFLAGS =
PROJ_LIB =
PROJ_DEP = $(BUILD)/libutil.a $(BUILD)/libhex.a $(BUILD)/libuct.a $(BUILD)/libsolver.a

default : all

SRC_CPP= \
  BookCheck.cpp \
  EndgameCheck.cpp \
  HandBookCheck.cpp \
  HandicapPlayer.cpp \
  LadderCheck.cpp \
  MoHexPlayer.cpp \
  PerfectPlayer.cpp \
  PlayerFactory.cpp \
  SolverCheck.cpp \
  SwapCheck.cpp \
  UofAPlayer.cpp \
  VulPreCheck.cpp \
  WolvePlayer.cpp
