## Process this file with automake to create Makefile.in.

SUBDIRS = 		\
	arabic		\
	basic 		\
	hangul 		\
	indic           \
	thai		\
	hebrew

EXTRA_DIST =		\
	module.def

if CROSS_COMPILING
RUN_QUERY_IMMODULES_TEST=false
else
RUN_QUERY_IMMODULES_TEST=test -z "$(DESTDIR)"
endif

install-data-local:
	@if $(RUN_QUERY_IMMODULES_TEST) ; then 				\
	  echo $(mkinstalldirs) $(DESTDIR)$(sysconfdir)/pango \&\& 	\
	  $(top_builddir)/pango/pango-querymodules 			\
		\> $(DESTDIR)$(sysconfdir)/pango/pango.modules ;	\
	  $(mkinstalldirs) $(DESTDIR)$(sysconfdir)/pango && 		\
	  $(top_builddir)/pango/pango-querymodules 			\
		> $(DESTDIR)$(sysconfdir)/pango/pango.modules ;		\
	else								\
	  echo "***" ; 							\
	  echo "*** Warning: pango.modules not created" ; 		\
	  echo "***" ; 							\
	  echo "*** Generate this file on the target system" ; 		\
	  echo "*** system using pango-querymodules" ; 			\
	  echo "***" ; 							\
	fi

included-modules:
	@for d in $(SUBDIRS) ; do \
		( cd $$d && $(MAKE) $(AM_MAKEFLAGS) included-modules ) ; \
        done

.PHONY: included-modules

