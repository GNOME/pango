## Process this file with automake to create Makefile.in.

SUBDIRS = 		\
	arabic		\
	basic 		\
	hangul 		\
	indic           \
	tamil 		\
	thai		\
	hebrew

EXTRA_DIST =		\
	module.def

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(sysconfdir)/pango && 	\
	$(top_builddir)/pango/pango-querymodules 		\
		> $(DESTDIR)$(sysconfdir)/pango/pango.modules

included-modules:
	@for d in $(SUBDIRS) ; do \
		( cd $$d && $(MAKE) $(AM_MAKEFLAGS) included-modules ) ; \
        done

.PHONY: included-modules

