## Process this file with automake to create Makefile.in.

pangolibs = $(top_builddir)/pango/libpango-$(PANGO_API_VERSION).la $(FRIBIDI_LIBS) $(GLIB_LIBS)
pangoxlibs = $(top_builddir)/pango/libpangox-$(PANGO_API_VERSION).la $(X_LIBS) $(pangolibs)
pangoxftlibs = $(top_builddir)/pango/libpangoxft-$(PANGO_API_VERSION).la $(XFT_LIBS) $(pangolibs)
pangoft2libs = $(top_builddir)/pango/libpangoft2-$(PANGO_API_VERSION).la $(FREETYPE_LIBS) $(pangolibs)

x_sources = hangul-x.c
xft_sources = hangul-xft.c

if HAVE_XFT
if INCLUDE_HANGUL_XFT
XFT_INCLUDED=libpango-hangul-xft.la
XFT_MODULES=
XFT_PREFIX=-DXFT_MODULE_PREFIX
else
XFT_MODULES=pango-hangul-xft.la
XFT_INCLUDED=
XFT_PREFIX=
hangul_xft_libadd=$(pangoxftlibs)
endif
else
XFT_MODULES=
XFT_INCLUDED=
XFT_PREFIX=
endif

if HAVE_X
if INCLUDE_HANGUL_X
X_MODULES=
X_INCLUDED=libpango-hangul-x.la
X_PREFIX=-DX_MODULE_PREFIX
else
X_MODULES=pango-hangul-x.la
X_INCLUDED=
X_PREFIX=
hangul_x_libadd=$(pangoxlibs)
endif
else
X_MODULES=
X_INCLUDED=
X_PREFIX=
endif

noinst_LTLIBRARIES = $(X_INCLUDED) $(XFT_INCLUDED)
module_LTLIBRARIES = $(X_MODULES) $(XFT_MODULES)
moddefine = $(X_PREFIX) $(XFT_PREFIX)
moduledir = $(libdir)/pango/$(PANGO_MODULE_VERSION)/modules

INCLUDES = -DPANGO_ENABLE_ENGINE -DG_DISABLE_DEPRECATED -I$(top_srcdir) -I$(top_srcdir)/pango/ $(moddefine) $(X_CFLAGS) $(FREETYPE_CFLAGS)

pango_hangul_x_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_hangul_x_la_SOURCES = $(x_sources)
pango_hangul_x_la_LIBADD = $(hangul_x_libadd)

libpango_hangul_x_la_SOURCES = $(xft_sources)

pango_hangul_xft_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_hangul_xft_la_LIBADD = $(hangul_xft_libadd)
pango_hangul_xft_la_SOURCES = $(xft_sources)

libpango_hangul_xft_la_SOURCES = $(xft_sources)

EXTRA_DIST =			\
	tables-ksc5601.i	\
	tables-johabfont.i

included-modules: $(noinst_LTLIBRARIES)

.PHONY: included-modules
