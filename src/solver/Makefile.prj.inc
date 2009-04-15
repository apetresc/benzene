#-----------------------------------------------------------------------------
# GNU Makefile for static libraries: project dependent part
#
# $Id: Makefile.prj.inc 1470 2008-06-17 22:04:32Z broderic $
#-----------------------------------------------------------------------------

NAME = solver
TARG_NAME = libsolver.a
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
  SolvedState.cpp \
  SolverDB.cpp \
  Solver.cpp

