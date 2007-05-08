## Process this file with automake to create Makefile.in.

SUBDIRS = 		\
	arabic		\
	basic 		\
	hangul 		\
	hebrew		\
	indic           \
	khmer		\
	syriac		\
	thai		\
	tibetan

EXTRA_DIST =		\
	Module.mk	\
	module.def	\
	pangorc

CLEANFILES =		\
	pango.modules

if CROSS_COMPILING
RUN_QUERY_MODULES_TEST=false
else
RUN_QUERY_MODULES_TEST=true
all-local: pango.modules
endif

install-data-local: $(top_builddir)/pango/pango-querymodules$(EXEEXT)
	@if $(RUN_QUERY_MODULES_TEST) && test -z "$(DESTDIR)" ; then 	\
	  echo $(mkinstalldirs) $(DESTDIR)$(sysconfdir)/pango \&\& 	\
	  $(top_builddir)/pango/pango-querymodules$(EXEEXT) 		\
		\> $(DESTDIR)$(sysconfdir)/pango/pango.modules ;	\
	  $(mkinstalldirs) $(DESTDIR)$(sysconfdir)/pango && 		\
	  $(top_builddir)/pango/pango-querymodules$(EXEEXT) 		\
		> $(DESTDIR)$(sysconfdir)/pango/pango.modules ;		\
	else								\
	  echo "***" ; 							\
	  echo "*** Warning: $(sysconfdir)/pango/pango.modules" ; 	\
	  echo "*** not created" ; 					\
	  echo "*** Generate this file on the target system" ; 		\
	  echo "*** using pango-querymodules" ; 			\
	  echo "***" ; 							\
	fi

uninstall-local:
	$(RM) $(DESTDIR)$(sysconfdir)/pango/pango.modules

pango.modules:						\
	$(SUBDIRS)					\
	$(top_builddir)/config.status			\
	$(top_builddir)/pango/pango-querymodules$(EXEEXT)
	@if $(RUN_QUERY_MODULES_TEST) && find . -name '*.la' | grep -v '\.libs' | grep -v libpango | grep '.' > /dev/null ; then \
          echo "Writing a pango.modules file to use with tests/examples."; \
          $(top_builddir)/pango/pango-querymodules$(EXEEXT) `find . -name '*.la' | grep -v '\.libs' | grep -v libpango` > $@ ;\
	else \
          echo "***" ; \
	  echo "*** No dynamic modules found, or cross-compiling" ; \
	  echo "*** Will use only static modules for tests and examples." ; \
	  echo "***" ; \
	  > $@; \
	fi

included-modules:
	@for d in $(SUBDIRS) ; do \
		( cd $$d && $(MAKE) $(AM_MAKEFLAGS) included-modules ) ; \
        done

dynamic-modules:
	@for d in $(SUBDIRS) ; do \
		( cd $$d && $(MAKE) $(AM_MAKEFLAGS) dynamic-modules ) ; \
        done

.PHONY: included-modules dynamic-modules
