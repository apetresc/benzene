#-----------------------------------------------------------------------------
# GNU Makefile for static libraries: project dependent part
#
# $Id: Makefile.prj.inc 1657 2008-09-15 23:32:09Z broderic $
#-----------------------------------------------------------------------------

NAME = benzene
TARG_NAME = benzene
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
EXPLIB = $(EXPDIR)/build/gmake/build/release/

PROJ_LNFLAGS = -L$(BUILD) -L$(EXPLIB)
PROJ_LIB = -lhtp -lprogram -lplayer -lsolver -luct -lhex -lutil \
    -lfuego_smartgame \
    -ldb -lboost_thread

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
  htpbenzene.cpp \
  Benzene.cpp


