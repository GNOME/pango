## Process this file with automake to create Makefile.in.

SUBDIRS = 		\
	arabic		\
	basic 		\
	indic           \
	thai

EXTRA_DIST =		\
	Module.mk	\
	module.def	\
	pangorc

included-modules:
	@for d in $(SUBDIRS) ; do \
		( cd $$d && $(MAKE) $(AM_MAKEFLAGS) included-modules ) ; \
        done

.PHONY: included-modules

-include $(top_srcdir)/git.mk
