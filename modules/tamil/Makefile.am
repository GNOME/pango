## Process this file with automake to create Makefile.in.

pangolibs = $(top_builddir)/pango/libpango-$(PANGO_API_VERSION).la $(FRIBIDI_LIBS) $(GLIB_LIBS)
pangoxlibs = $(top_builddir)/pango/libpangox-$(PANGO_API_VERSION).la $(X_LIBS) $(pangolibs)
pangoxftlibs = $(top_builddir)/pango/libpangoxft-$(PANGO_API_VERSION).la $(XFT_LIBS) $(pangolibs)
pangoft2libs = $(top_builddir)/pango/libpangoft2-$(PANGO_API_VERSION).la $(FREETYPE_LIBS) $(pangolibs)

if HAVE_XFT
if INCLUDE_TAMIL_XFT
XFT_MODULES=
XFT_INCLUDED=libpango-tamil-xft.la
XFT_PREFIX=-DXFT_MODULE_PREFIX
else
XFT_MODULES=pango-tamil-xft.la
XFT_INCLUDED=
XFT_PREFIX=
tamil_xft_libadd=$(pangoxftlibs)
endif
else
XFT_MODULES=
XFT_INCLUDED=
XFT_PREFIX=
endif

x_sources = \
	tamil-x.c \
	tadefs.h

xft_sources = \
	tamil-xft.c \
	tadefs.h

if HAVE_X
if INCLUDE_TAMIL_X
X_MODULES=
X_INCLUDED=libpango-tamil-x.la
X_PREFIX=-DX_MODULE_PREFIX
else
X_MODULES=pango-tamil-x.la
X_INCLUDED=
X_PREFIX=
tamil_x_libadd=$(pangoxlibs)
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

pango_tamil_x_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_tamil_x_la_LIBADD = $(tamil_x_libadd)
pango_tamil_x_la_SOURCES = $(x_sources)

libpango_tamil_x_la_SOURCES = $(x_sources)

pango_tamil_xft_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_tamil_xft_la_LIBADD = $(tamil_xft_libadd)
pango_tamil_xft_la_SOURCES = $(xft_sources)

libpango_tamil_xft_la_SOURCES = $(xft_sources)

included-modules: $(noinst_LTLIBRARIES)

.PHONY: included-modules
