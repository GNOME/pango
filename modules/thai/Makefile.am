## Process this file with automake to create Makefile.in.

common_sources = thai-shaper.c thai-shaper.h
thai_x_sources = $(common_sources) thai-x.c 
thai_xft_sources = $(common_sources) thai-xft.c 

pangolibs = $(top_builddir)/pango/libpango-$(PANGO_API_VERSION).la $(FRIBIDI_LIBS) $(GLIB_LIBS)
pangoxlibs = $(top_builddir)/pango/libpangox-$(PANGO_API_VERSION).la $(X_LIBS) $(pangolibs)
pangoxftlibs = $(top_builddir)/pango/libpangoxft-$(PANGO_API_VERSION).la $(XFT_LIBS) $(pangolibs)
pangoft2libs = $(top_builddir)/pango/libpangoft2-$(PANGO_API_VERSION).la $(FREETYPE_LIBS) $(pangolibs)

if HAVE_XFT
if INCLUDE_THAI_XFT
XFT_MODULES=
XFT_INCLUDED=libpango-thai-xft.la
XFT_PREFIX=-DXFT_MODULE_PREFIX
else
XFT_MODULES=pango-thai-xft.la
XFT_INCLUDED=
XFT_PREFIX=
thai_xft_libadd=$(pangoxftlibs)
endif
else
XFT_MODULES=
XFT_INCLUDED=
XFT_PREFIX=
endif

if HAVE_X
if INCLUDE_THAI_X
X_MODULES=
X_INCLUDED=libpango-thai-x.la
X_PREFIX=-DX_MODULE_PREFIX
else
X_MODULES=pango-thai-x.la
X_INCLUDED=
X_PREFIX=
thai_x_libadd=$(pangoxlibs)
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

INCLUDES = -DPANGO_ENABLE_ENGINE -DG_DISABLE_DEPRECATED -I$(top_srcdir) -I$(top_srcdir)/pango/ $(moddefine) $(XFT_CFLAGS) $(FREETYPE_CFLAGS) $(X_CFLAGS)

pango_thai_x_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_thai_x_la_LIBADD = $(thai_x_libadd)
pango_thai_x_la_SOURCES = $(thai_x_sources)

libpango_thai_x_la_SOURCES = $(thai_x_sources)

pango_thai_xft_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_thai_xft_la_LIBADD = $(thai_xft_libadd)
pango_thai_xft_la_SOURCES = $(thai_xft_sources)

libpango_thai_xft_la_SOURCES = $(thai_xft_sources)

included-modules: $(noinst_LTLIBRARIES)

.PHONY: included-modules
