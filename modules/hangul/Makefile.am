## Process this file with automake to create Makefile.in.

sources = hangul-x.c

pangolibs = $(top_builddir)/pango/libpango-$(PANGO_API_VERSION).la $(FRIBIDI_LIBS) $(GLIB_LIBS)
pangoxlibs = $(top_builddir)/pango/libpangox-$(PANGO_API_VERSION).la $(X_LIBS) $(pangolibs)
pangoxftlibs = $(top_builddir)/pango/libpangoxft-$(PANGO_API_VERSION).la $(XFT_LIBS) $(pangolibs)
pangoft2libs = $(top_builddir)/pango/libpangoft2-$(PANGO_API_VERSION).la $(FREETYPE_LIBS) $(pangolibs)

if HAVE_X
if INCLUDE_HANGUL_X
noinst_LTLIBRARIES = libpango-hangul-x.la
moddefine = -DX_MODULE_PREFIX
else
moduledir = $(libdir)/pango/$(PANGO_MODULE_VERSION)/modules
module_LTLIBRARIES = pango-hangul-x.la
hangul_x_libadd = $(pangoxlibs)
endif
endif

INCLUDES = -DPANGO_ENABLE_ENGINE -DG_DISABLE_DEPRECATED -I$(top_srcdir) -I$(top_srcdir)/pango/ $(moddefine) $(X_CFLAGS)

pango_hangul_x_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_hangul_x_la_SOURCES = $(sources)
pango_hangul_x_la_LIBADD = $(hangul_x_libadd)

libpango_hangul_x_la_SOURCES = $(sources)

EXTRA_DIST =			\
	tables-ksc5601.i	\
	tables-johabfont.i

included-modules: $(noinst_LTLIBRARIES)

.PHONY: included-modules
