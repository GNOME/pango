## Process this file with automake to create Makefile.in.

SUBDIRS=basic hangul tamil

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(sysconfdir)/pango
	$(top_builddir)/libpango/pango-querymodules $(DESTDIR)$(libdir)/pango/modules/*.so > $(DESTDIR)$(sysconfdir)/pango/pango.modules
