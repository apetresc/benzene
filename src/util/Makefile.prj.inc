#-----------------------------------------------------------------------------
# GNU Makefile for static libraries: project dependent part
#
# $Id: Makefile.prj.inc 1657 2008-09-15 23:32:09Z broderic $
#-----------------------------------------------------------------------------

NAME = util
TARG_NAME = libutil.a
BUILD = ../../build

VPATH = ../..

EXPDIR = ../../fuego-0.1
OBJDIR = $(BUILD)/obj/$(NAME)
BINDIR = $(BUILD)

PROJ_CXXFLAGS = -I$(EXPDIR)/smartgame
PROJ_LNFLAGS =
PROJ_LIB =
PROJ_DEP = 

default : all

SRC_CPP= \
  Bitset.cpp \
  HexException.cpp \
  Logger.cpp \
  lssolve.cpp \
  Misc.cpp \
  Time.cpp
