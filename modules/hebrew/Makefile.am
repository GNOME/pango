## Process this file with automake to create Makefile.in.

sources = hebrew-x.c

pangolibs = $(top_builddir)/pango/libpango.la $(FRIBIDI_LIBS) $(GLIB_LIBS)
pangoxlibs = $(top_builddir)/pango/libpangox.la $(X_LIBS) $(pangolibs)
pangoxftlibs = $(top_builddir)/pango/libpangoxft.la $(XFT_LIBS) $(pangolibs)
pangoft2libs = $(top_builddir)/pango/libpangoft2.la $(FREETYPE_LIBS) $(pangolibs)

if HAVE_X
if INCLUDE_HEBREW_X
noinst_LTLIBRARIES = libpango-hebrew-x.la
moddefine = -DX_MODULE_PREFIX
else
moduledir = $(libdir)/pango/modules
module_LTLIBRARIES = pango-hebrew-x.la
hebrew_x_libadd=$(pangoxlibs)
endif
endif

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/pango/ $(moddefine) $(X_CFLAGS)

pango_hebrew_x_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_hebrew_x_la_LIBADD = $(hebrew_x_libadd)
pango_hebrew_x_la_SOURCES = $(sources)

libpango_hebrew_x_la_SOURCES = $(sources)

included-modules: $(noinst_LTLIBRARIES)

.PHONY: included-modules
