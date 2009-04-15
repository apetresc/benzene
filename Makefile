#----------------------------------------------------------------------------
# $Id: Makefile 1699 2008-09-24 23:55:56Z broderic $
#----------------------------------------------------------------------------

.PHONY: default clean hex

SRCDIR = src/

HAVE_FUEGO = $(wildcard fuego-0.1)
ifeq ($(strip $(HAVE_FUEGO)),)
  $(error Symbolic link 'fuego-0.1' to Fuego-0.1 codebase does not exist)
endif

default: hex

all: test six 

hex:
	cd $(SRCDIR) && $(MAKE)

six:
	cd $(SRCDIR) && $(MAKE) six

test:   
	cd $(SRCDIR) && $(MAKE) test

clean:
	cd $(SRCDIR) && $(MAKE) clean

