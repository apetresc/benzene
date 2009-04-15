#-----------------------------------------------------------------------------
# GNU Makefile for static libraries: project dependent part
#
# $Id: Makefile.prj.inc 406 2007-06-04 19:13:47Z broderic $
#-----------------------------------------------------------------------------

NAME = six
TARG_NAME = six
BUILD = ../../build

VPATH = ../..

EXPDIR = ../../fuego-0.1
OBJDIR = $(BUILD)/obj/$(NAME)
BINDIR = ../../bin

PROJ_CXXFLAGS = \
    -I..\
    -I../util \
    -I../hex \
    -I../hex/eval -I../hex/patterns \
    -I../player/ \
    -I../program/ \
    -I../htp/ \
    -I$(EXPDIR)/smartgame 

# always link with release version of fuego-0.1
EXPLIB = $(EXPDIR)/build/gmake/build/release/

PROJ_LNFLAGS = -L$(BUILD) -L$(EXPLIB)
PROJ_LIB = -lhtp -lprogram -lplayer -lhex -lutil \
    -lfuego_smartgame\
    -lboost_thread\
    -lpthread

PROJ_DEP = \
    $(BUILD)/libutil.a \
    $(BUILD)/libhex.a \
    $(BUILD)/libplayer.a \
    $(BUILD)/libprogram.a \
    $(BUILD)/libhtp.a \
    $(EXPLIB)

default : all

SRC_CPP= \
  batch.cpp \
  carrier.cpp \
  circuit.cpp \
  connector.cpp \
  group.cpp \
  hexboard.cpp \
  hexgame.cpp \
  hexgamestate.cpp \
  hexmark.cpp \
  hexmove.cpp \
  lssolve.cpp \
  main.cpp \
  misc.cpp \
  sixplayer.cpp \
  Six.cpp \
  HtpSixEngine.cpp

