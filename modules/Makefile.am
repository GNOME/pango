## Process this file with automake to create Makefile.in.

SUBDIRS=basic hangul tamil arabic

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(localstatedir)/lib/pango
	$(top_builddir)/libpango/pango-querymodules $(DESTDIR)$(libdir)/pango/modules/*.so > $(DESTDIR)$(localstatedir)/lib/pango/pango.modules

