################################################################################
# You shouldn't need to edit this file. Edit makefile.conf instead.
################################################################################

include makefile.conf

BASE = insts/base
DIRS = include genlib src insts utils docs

all:
	@echo "making all ..."
	@for DIR in $(DIRS) $(INST_DIRS); \
	do \
	  ( cd $$DIR; $(MAKE) $(MFLAGS) all ); \
	done
ifneq ($(strip $(PACKAGE_DIRS)),)    # do only if PACKAGE_DIRS is nonempty
	@for DIR in $(PACKAGE_DIRS); \
	do \
		( cd $$DIR; $(MAKE) $(MFLAGS) all ); \
	done
endif
	@echo "done"

# Individual make targets.  Note that these are not necessarily equivalent
# to the subdir with the same name.

include::
	@echo "making and installing include..."
	@cd include; $(MAKE) $(MFLAGS) install
	@echo "done."; echo ""

src::
	@echo "making all in src..."
	@cd src; $(MAKE) $(MFLAGS) all
	@echo "done."; echo ""

genlib::
	@echo "making and installing genlib..."
	@cd genlib; $(MAKE) $(MFLAGS) install
	@echo "done."; echo ""

utils::
	@echo "making utils..."
	@cd utils; $(MAKE) $(MFLAGS) all
	@echo "done."; echo ""

docs::
	@echo "making docs ..."
	@cd docs; $(MAKE) $(MFLAGS) all
	@echo "done."; echo ""

insts::
	@echo "making insts ..."
	@cd insts; $(MAKE) $(MFLAGS) all
	@echo "done."; echo ""

base::
	@echo "making base insts ..."
	@cd insts; $(MAKE) $(MFLAGS) base
	@echo "done."; echo ""

packages::
ifneq ($(strip $(PACKAGE_DIRS)),)    # do only if PACKAGE_DIRS is nonempty
	@for DIR in $(PACKAGE_DIRS); \
	do \
		( cd $$DIR; echo "making $$DIR..."; \
		echo "include $(MAKEFILE_CONF)" > package.conf; \
		$(MAKE) $(MFLAGS) all; echo "done."; echo "" ); \
	done
endif

dsos:: insts

standalone::
	@echo "making standalone ..."
	@cd insts; $(MAKE) $(MFLAGS) standalone
	@echo "done."; echo ""

#############################################################  make install  ###

install::
	@echo "beginning install..."
	@if test ! -d $(LIBDIR); then mkdir $(LIBDIR); fi;
	@if test ! -d $(LIBDESTDIR); then mkdir $(LIBDESTDIR); fi;
	@for DIR in $(DIRS) $(INST_DIRS); \
	do \
	  ( cd $$DIR; $(MAKE) $(MFLAGS) install ); \
	done
ifneq ($(strip $(PACKAGE_DIRS)),)    # do only if PACKAGE_DIRS is nonempty
	@for DIR in $(PACKAGE_DIRS); \
	do \
		( cd $$DIR; $(MAKE) $(MFLAGS) install ); \
	done
endif
	@echo "install done."; echo ""

base_install:
	@echo "beginning base_install..."
	@cd $(BASE); $(MAKE) $(MFLAGS) install;
	@echo "base_install done."; echo ""

dso_install: 
	@echo "beginning dso_install..."
	@for DIR in $(INST_DIRS); \
	do \
	  ( cd $$DIR; $(MAKE) $(MFLAGS) dso_install ); \
	done
	@echo "dso_install done."; echo ""

standalone_install::
	@echo "beginning standalone_install..."
	@cd insts; $(MAKE) $(MFLAGS) standalone_install;
	@echo "standalone_install done."; echo ""

###########################################################  make uninstall  ###

uninstall::
	@echo "beginning uninstall..."
	@for DIR in $(DIRS); \
	do \
	  ( cd $$DIR; $(MAKE) $(MFLAGS) uninstall ); \
	done
ifneq ($(strip $(PACKAGE_DIRS)),)    # do only if PACKAGE_DIRS is nonempty
	@for DIR in $(PACKAGE_DIRS); \
	do \
		( cd $$DIR; $(MAKE) $(MFLAGS) uninstall ); \
	done
endif
	@echo "uninstall done."; echo ""

dso_uninstall: 
	@echo "beginning dso_uninstall..."
	@cd insts; $(MAKE) $(MFLAGS) dso_uninstall; 
	@echo "dso_uninstall done."; echo ""

standalone_uninstall::
	@echo "beginning standalone_uninstall..."
	@cd insts; $(MAKE) $(MFLAGS) standalone_uninstall; 
	@echo "standalone_uninstall done."; echo ""

###############################################################  make depend  ##
depend::
	@for DIR in $(DIRS); \
	do \
	  ( cd $$DIR; echo "making depend in $$DIR..."; \
	  $(RM) depend; \
	  $(MAKE) $(MFLAGS) depend ); \
	done
###############################################################  make clean  ###

clean::
	@for DIR in $(DIRS); \
	do \
	  ( cd $$DIR; echo "making clean in $$DIR..."; \
	  $(MAKE) $(MFLAGS) clean ); \
	done
ifneq ($(strip $(PACKAGE_DIRS)),)    # do only if PACKAGE_DIRS is nonempty
	@for DIR in $(PACKAGE_DIRS); \
	do \
		( cd $$DIR; echo "making clean in $$DIR..."; \
		$(MAKE) $(MFLAGS) clean ); \
	done
endif

cleanall::
	@for DIR in $(DIRS) $(INST_DIRS); \
	do \
	  ( cd $$DIR; echo "making cleanall in $$DIR..."; \
	  $(MAKE) $(MFLAGS) cleanall ); \
	done
ifneq ($(strip $(PACKAGE_DIRS)),)    # do only if PACKAGE_DIRS is nonempty
	@for DIR in $(PACKAGE_DIRS); \
	do \
		( cd $$DIR; echo "making clean in $$DIR..."; \
		$(MAKE) $(MFLAGS) cleanall ); \
	done
endif

# Make it clean for distribution or for moving to another system
distclean: cleanall
	$(RM) Minc/depend
	@for DIR in $(INST_DIRS); \
	do \
	  ( cd $$DIR; $(RM) package.conf );  \
	done
ifneq ($(strip $(PACKAGE_DIRS)),)    # do only if PACKAGE_DIRS is nonempty
	@for DIR in $(PACKAGE_DIRS); \
	do \
	  ( cd $$DIR; $(RM) package.conf );  \
	done
endif

