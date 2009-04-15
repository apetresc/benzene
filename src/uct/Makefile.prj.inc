#-----------------------------------------------------------------------------
# GNU Makefile for static libraries: project dependent part
#
# $Id: Makefile.prj.inc 1657 2008-09-15 23:32:09Z broderic $
#-----------------------------------------------------------------------------

NAME = uct
TARG_NAME = libuct.a
BUILD = ../../build

VPATH = ../..

EXPDIR = ../../fuego-0.1
OBJDIR = $(BUILD)/obj/$(NAME)
BINDIR = $(BUILD)

PROJ_CXXFLAGS = -I../util -I../hex/ -I../hex/eval/ -I../hex/patterns/ -I$(EXPDIR)/smartgame
PROJ_LNFLAGS =
PROJ_LIB =
PROJ_DEP = $(BUILD)/libutil.a $(BUILD)/libhex.a

default : all

SRC_CPP= \
  HexUctInit.cpp \
  HexUctPolicy.cpp \
  HexUctSearch.cpp \
  HexUctState.cpp \
  HexUctUtil.cpp
