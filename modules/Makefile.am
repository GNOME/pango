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
	($(mkinstalldirs) $(DESTDIR)$(sysconfdir)/pango; \
	if ls $(DESTDIR)$(libdir)/pango/modules/*.so | grep 'so' > /dev/null ; then \
          echo "Wrote $(DESTDIR)$(sysconfdir)/pango/pango.modules."; \
          $(top_builddir)/pango/pango-querymodules $(DESTDIR)$(libdir)/pango/modules/*.so \
		> $(DESTDIR)$(sysconfdir)/pango/pango.modules; \
        else \
          echo "No dynamic modules were installed; all modules were linked statically."; \
	  echo "# This is an empty file, because Pango was installed with no dynamic modules" > $(DESTDIR)$(sysconfdir)/pango/pango.modules; \
        fi)

included-modules:
	@for d in $(SUBDIRS) ; do \
		( cd $$d && $(MAKE) $(AM_MAKEFLAGS) included-modules ) ; \
        done

.PHONY: included-modules

