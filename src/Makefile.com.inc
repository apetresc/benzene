#-----------------------------------------------------------------------------
# GNU Makefile: common part for all targets
#
# $Id: Makefile.com.inc 932 2007-11-01 22:29:59Z broderic $
#-----------------------------------------------------------------------------

# C++ compiler and flags.

CXX = g++
COMMON_CXXFLAGS += -DUNIX -Wall -O2 -g
# Use -Wextra only for GCC version > 3
ifneq ($(shell echo __GNUC__ | $(CXX) -E -xc - | tail -n 1), 3)
COMMON_CXXFLAGS += -Wextra
endif

CXXFLAGS += $(PROJ_CXXFLAGS) $(COMMON_CXXFLAGS)

ifeq ($(findstring $(BUILDTYPE),release),release)
CXXFLAGS += -DNDEBUG
endif

ifeq ($(findstring $(PROFILE),1),1)
CXXFLAGS += -pg 
endif

# C compiler and flags.

CC = g++
COMMON_CFLAGS += -DUNIX -Wall -O3 -g
CFLAGS += $(PROJ_CFLAGS) $(COMMON_CFLAGS)

ifeq ($(findstring $(BUILDTYPE),release),release)
CFLAGS += -DNDEBUG
endif

ifeq ($(findstring $(PROFILE),1),1)
CFLAGS += -pg 
endif

# Linker.

LN = $(CXX)
COMMON_LNFLAGS += 
LNFLAGS += $(PROJ_LNFLAGS) $(COMMON_LNFLAGS) -g

ifeq ($(findstring $(BUILDTYPE),release),release)
LNFLAGS +=
endif

ifeq ($(findstring $(PROFILE),1),1)
LNFLAGS += -pg 
endif


AR = ar

OBJS = \
  $(patsubst %.cpp, $(OBJDIR)/%.o, $(SRC_CPP)) \
  $(patsubst %.c, $(OBJDIR)/%.o, $(SRC_C))

#-----------------------------------------------------------------------------

$(OBJDIR)/%.o: %.cpp
	@echo "$(notdir $@)"
	mkdir -p $(dir $@)
	$(CXX) -c $(CXXFLAGS) -o $@ $<

$(OBJDIR)/%.o: %.c
	@echo "$(notdir $@)"
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) -o $@ $<


# Creation of dependency files using the compiler with -MM option and
# sed for postprocessing of its output (including the dependency file in
# the target section and adding the correct path to the object file). 
# Dependency files are separated for each project, even if the
# object files are shared, since the compiler generated dependency lines
# contain relative paths.

$(OBJDIR)/%.d: %.cpp
	@echo "$(notdir $@)"
	mkdir -p $(dir $@)
	$(CXX) -MM $(CXXFLAGS) $< | sed 's#\([-A-Za-z0-9_,]*\)\.o[ :]*#$(patsubst %.d,%.o,$@) $@ : #g' >$@


$(OBJDIR)/%.d: %.c
	@echo "$(notdir $@)"
	mkdir -p $(dir $@)
	$(CC) -MM $(CFLAGS) $< | sed 's#\([-A-Za-z0-9_,]*\)\.o[ :]*#$(patsubst %.d,%.o,$@) $@ : #g' >$@

doit: $(BINDIR)/$(TARG_NAME) post_build

all: doit

# Define post build steps in Makefile.prj.inc if needed.

post_build:

clean:
	@rm -f `find $(OBJDIR) -name "*.o" -o -name "*.d"`
	@rm -f $(BINDIR)/$(NAME)

#-----------------------------------------------------------------------------

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(SRC_CPP)),)
-include $(patsubst %.cpp, $(OBJDIR)/%.d, $(SRC_CPP))
endif
ifneq ($(strip $(SRC_C)),)
-include $(patsubst %.c, $(OBJDIR)/%.d, $(SRC_C))
endif
endif

#-----------------------------------------------------------------------------
