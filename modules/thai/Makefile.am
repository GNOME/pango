## Process this file with automake to create Makefile.in.

pangolibs = $(top_builddir)/pango/libpango-$(PANGO_API_VERSION).la $(FRIBIDI_LIBS) $(GLIB_LIBS)
pangoxlibs = $(top_builddir)/pango/libpangox-$(PANGO_API_VERSION).la $(X_LIBS) $(pangolibs)
pangoxftlibs = $(top_builddir)/pango/libpangoxft-$(PANGO_API_VERSION).la $(XFT_LIBS) $(pangolibs)
pangoft2libs = $(top_builddir)/pango/libpangoft2-$(PANGO_API_VERSION).la $(FREETYPE_LIBS) $(pangolibs)

INCLUDES =			\
	-DPANGO_ENABLE_ENGINE	\
	-DG_DISABLE_DEPRECATED	\
	$(PANGO_DEBUG_FLAGS)	\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/pango/	\
	$(GLIB_CFLAGS)

moduledir = $(libdir)/pango/$(PANGO_MODULE_VERSION)/modules
module_LTLIBRARIES =
noinst_LTLIBRARIES =

common_sources = thai-shaper.c thai-shaper.h


if HAVE_X
INCLUDES += $(X_CFLAGS)
if INCLUDE_THAI_X
noinst_LTLIBRARIES += libpango-thai-x.la
INCLUDES += -DX_MODULE_PREFIX
else
module_LTLIBRARIES += pango-thai-x.la
endif
endif

thai_x_sources = $(common_sources) thai-x.c 

pango_thai_x_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_thai_x_la_LIBADD = $(pangoxlibs)
pango_thai_x_la_SOURCES = $(thai_x_sources)
libpango_thai_x_la_SOURCES = $(thai_x_sources)


if HAVE_XFT
INCLUDES += $(XFT_CFLAGS)
if INCLUDE_THAI_XFT
noinst_LTLIBRARIES += libpango-thai-xft.la
INCLUDES += -DXFT_MODULE_PREFIX
else
module_LTLIBRARIES += pango-thai-xft.la
endif
endif

thai_xft_sources = $(common_sources) thai-xft.c 

pango_thai_xft_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_thai_xft_la_LIBADD = $(pangoxftlibs)
pango_thai_xft_la_SOURCES = $(thai_xft_sources)
libpango_thai_xft_la_SOURCES = $(thai_xft_sources)


included-modules: $(noinst_LTLIBRARIES)

.PHONY: included-modules
