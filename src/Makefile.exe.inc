#------------------------------------------------------------------------------
# GNU Makefile: part for executables
#
# $Id: Makefile.exe.inc 240 2007-04-10 21:19:30Z broderic $
#------------------------------------------------------------------------------

$(BINDIR)/$(TARG_NAME): $(OBJS) $(PROJ_DEP)
	@echo "Linking $@"
	mkdir -p $(dir $@)
	$(LN) -o $@ $(LNFLAGS) $(OBJS) $(PROJ_LIB)

