## Process this file with automake to create Makefile.in.

SUBDIRS = 		\
	arabic		\
	basic 		\
	hangul 		\
	indic           \
	tamil 		\
	thai



install-data-local:
	($(mkinstalldirs) $(DESTDIR)$(sysconfdir)/pango; \
	if ls $(DESTDIR)$(libdir)/pango/modules/*.so | grep -q 'so'; then \
          echo "Wrote $(DESTDIR)$(sysconfdir)/pango/pango.modules."; \
          $(top_builddir)/pango/pango-querymodules $(DESTDIR)$(libdir)/pango/modules/*.so \
		> $(DESTDIR)$(sysconfdir)/pango/pango.modules; \
        else \
          echo "No dynamic modules were installed; all modules were linked statically."; \
	  echo "# This is an empty file, because Pango was installed with no dynamic modules" > $(DESTDIR)$(sysconfdir)/pango/pango.modules; \
        fi)

EXTRA_DIST =		\
	module.def
