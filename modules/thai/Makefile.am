## Process this file with automake to create Makefile.in.

sources = thai-x.c

pangolibs = $(top_builddir)/pango/libpango-$(PANGO_API_VERSION).la $(FRIBIDI_LIBS) $(GLIB_LIBS)
pangoxlibs = $(top_builddir)/pango/libpangox-$(PANGO_API_VERSION).la $(X_LIBS) $(pangolibs)
pangoxftlibs = $(top_builddir)/pango/libpangoxft-$(PANGO_API_VERSION).la $(XFT_LIBS) $(pangolibs)
pangoft2libs = $(top_builddir)/pango/libpangoft2-$(PANGO_API_VERSION).la $(FREETYPE_LIBS) $(pangolibs)

if HAVE_X
if INCLUDE_THAI_X
noinst_LTLIBRARIES = libpango-thai-x.la
moddefine = -DX_MODULE_PREFIX
else
moduledir = $(libdir)/pango/$(PANGO_MODULE_VERSION)/modules
module_LTLIBRARIES = pango-thai-x.la
thai_x_libadd=$(pangoxlibs)
endif
endif

INCLUDES = -DPANGO_ENABLE_ENGINE -DG_DISABLE_DEPRECATED -I$(top_srcdir) -I$(top_srcdir)/pango/ $(moddefine) $(X_CFLAGS)

pango_thai_x_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_thai_x_la_LIBADD = $(thai_x_libadd)
pango_thai_x_la_SOURCES = $(sources)

libpango_thai_x_la_SOURCES = $(sources)

included-modules: $(noinst_LTLIBRARIES)

.PHONY: included-modules
