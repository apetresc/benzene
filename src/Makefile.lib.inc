#------------------------------------------------------------------------------
# GNU Makefile: part for static libraries
#
# $Id: Makefile.lib.inc 240 2007-04-10 21:19:30Z broderic $
#------------------------------------------------------------------------------

$(BINDIR)/$(TARG_NAME): $(OBJS) $(PROJ_DEP)
	@echo "Creating $@"
	mkdir -p $(dir $@)
	$(AR) -csr $@ $(OBJS)
