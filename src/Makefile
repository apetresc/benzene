#----------------------------------------------------------------------------
# $Id: Makefile 1657 2008-09-15 23:32:09Z broderic $
#----------------------------------------------------------------------------

UOFA_PROJECTS = \
  util \
  hex \
  uct \
  solver \
  player \
  program \
  htp \
  benzene \

ALL_PROJECTS = \
  util \
  hex \
  uct \
  solver \
  player \
  program \
  htp \
  benzene \
  six

.PHONY: default
default: uofa 

.PHONY: all
all: uofa six

.PHONY: buildinfo
buildinfo:
	rm -f buildinfo.h > /dev/null
	echo "#define BUILD_NUMBER \""`svnversion -n .`\" > buildinfo.h

.PHONY: uofa
uofa: buildinfo
	@for dir in $(UOFA_PROJECTS); do \
	  if test -f $$dir/Makefile; then \
	    echo -e "\n------------- $$dir -------------"; \
	    $(MAKE) -C $$dir || exit -1; \
	  fi; \
	done
	@echo ""

.PHONY: six
six: uofa
	@if test -f six/Makefile; then \
	  echo -e "\n------------- six -------------"; \
	  $(MAKE) -C six || exit -1; \
	fi;
	@echo ""

.PHONY: clean
clean:
	@for dir in $(ALL_PROJECTS); do \
	  if test -f $$dir/Makefile; then \
	    echo -e "\n------------- $$dir -------------"; \
	    $(MAKE) -C $$dir clean || exit -1; \
	  fi; \
	done
	@echo ""
	$(MAKE) -C test clean

.PHONY: test
test: test-basic

.PHONY: test-basic
test-basic: uofa
	@echo "------------- test-basic -------------"
	$(MAKE) -C test run-basic

