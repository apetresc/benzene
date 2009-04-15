#-----------------------------------------------------------------------------
# GNU Makefile for static libraries: project dependent part
#
# $Id: Makefile.prj.inc 1470 2008-06-17 22:04:32Z broderic $
#-----------------------------------------------------------------------------

NAME = htp
TARG_NAME = libhtp.a
BUILD = ../../build

VPATH = ../..

EXPDIR = ../../fuego-0.1
OBJDIR = $(BUILD)/obj/$(NAME)
BINDIR = $(BUILD)

PROJ_CXXFLAGS = \
    -I../util \
    -I../hex/ -I../hex/eval/ -I../hex/patterns/ \
    -I../ab/ -I../uct/ -I../solver/ \
    -I../player/ -I../program/ \
    -I$(EXPDIR)/smartgame

PROJ_LNFLAGS =

PROJ_LIB =

PROP_DEP = \
    $(BUILD)/libutil.a \                                                  \
    $(BUILD)/libhex.a \
    $(BUILD)/libab.a \
    $(BUILD)/libuct.a \
    $(BUILD)/libsolver.a \
    $(BUILD)/libplayer.a \
    $(BUILD)/libprogram.a \
    $(EXPLIB)

default : all

SRC_CPP= \
  GtpEngine.cpp \
  HtpHexEngine.cpp \
  HtpUofAEngine.cpp


