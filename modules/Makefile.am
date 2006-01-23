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
	module.def	\
	pangorc

CLEANFILES =		\
	pango.modules

if CROSS_COMPILING
RUN_QUERY_MODULES_TEST=false
else
RUN_QUERY_MODULES_TEST=test -z "$(DESTDIR)"
all-local: pango.modules
endif

install-data-local: $(top_builddir)/pango/pango-querymodules
	@if $(RUN_QUERY_MODULES_TEST) ; then 				\
	  echo $(mkinstalldirs) $(DESTDIR)$(sysconfdir)/pango \&\& 	\
	  $(top_builddir)/pango/pango-querymodules 			\
		\> $(DESTDIR)$(sysconfdir)/pango/pango.modules ;	\
	  $(mkinstalldirs) $(DESTDIR)$(sysconfdir)/pango && 		\
	  $(top_builddir)/pango/pango-querymodules 			\
		> $(DESTDIR)$(sysconfdir)/pango/pango.modules ;		\
	else								\
	  echo "***" ; 							\
	  echo "*** Warning: $(sysconfdir)/pango/pango.modules" ; 	\
	  echo "*** not created" ; 					\
	  echo "*** Generate this file on the target system" ; 		\
	  echo "*** using pango-querymodules" ; 			\
	  echo "***" ; 							\
	fi

pango.modules: $(top_builddir)/pango/pango-querymodules $(SUBDIRS)
	@if find . -name '*.so' | grep 'so' > /dev/null ; then \
          echo "Writing a pango.modules file to use with tests/examples."; \
          $(top_builddir)/pango/pango-querymodules `find . -name '*.la' | grep -v '\.libs' | grep -v libpango` > $@ ;\
	else \
          echo "No dynamic modules found; will use only static modules for tests/examples."; \
	  touch $@; \
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
