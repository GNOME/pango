## Process this file with automake to create Makefile.in.

SUBDIRS = 		\
	arabic		\
	devanagari 	\
	basic 		\
	hangul 		\
	tamil 		\
	thai



install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(sysconfdir)/pango
	$(top_builddir)/pango/pango-querymodules $(DESTDIR)$(libdir)/pango/modules/*.so \
		> $(DESTDIR)$(sysconfdir)/pango/pango.modules

EXTRA_DIST =		\
	module.def
