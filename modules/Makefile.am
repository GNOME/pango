## Process this file with automake to create Makefile.in.

SUBDIRS = 		\
	arabic		\
	devanagari 	\
	basic 		\
	hangul 		\
	tamil 

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(localstatedir)/lib/pango
	$(top_builddir)/pango/pango-querymodules $(DESTDIR)$(libdir)/pango/modules/*.so > $(DESTDIR)$(localstatedir)/lib/pango/pango.modules

