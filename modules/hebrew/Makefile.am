## Process this file with automake to create Makefile.in.

pangolibs = $(top_builddir)/pango/libpango-$(PANGO_API_VERSION).la $(GLIB_LIBS)
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

common_sources = hebrew-shaper.c hebrew-shaper.h


if HAVE_XFT
INCLUDES += $(XFT_CFLAGS)
if INCLUDE_HEBREW_XFT
noinst_LTLIBRARIES += libpango-hebrew-xft.la
else
module_LTLIBRARIES += pango-hebrew-xft.la
endif
endif

hebrew_xft_sources = $(common_sources) hebrew-fc.c 

pango_hebrew_xft_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_hebrew_xft_la_LIBADD = $(pangoxftlibs)
pango_hebrew_xft_la_SOURCES = $(hebrew_xft_sources)
pango_hebrew_xft_la_CFLAGS = -DBUILD_XFT
libpango_hebrew_xft_la_SOURCES = $(hebrew_xft_sources)
libpango_hebrew_xft_la_CFLAGS = -DBUILD_XFT -DPANGO_MODULE_PREFIX=_pango_hebrew_xft


if HAVE_FREETYPE
INCLUDES += $(FREETYPE_CFLAGS)
if INCLUDE_HEBREW_FT2
noinst_LTLIBRARIES += libpango-hebrew-ft2.la
else
module_LTLIBRARIES += pango-hebrew-ft2.la
endif
endif

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

hebrew_ft2_sources = $(common_sources) hebrew-fc.c 

pango_hebrew_ft2_la_LDFLAGS = -export-dynamic -avoid-version -module $(no_undefined)
pango_hebrew_ft2_la_LIBADD = $(pangoft2libs)
pango_hebrew_ft2_la_SOURCES = $(hebrew_ft2_sources)
pango_hebrew_ft2_la_CFLAGS = -DBUILD_FT2
libpango_hebrew_ft2_la_SOURCES = $(hebrew_ft2_sources)
libpango_hebrew_ft2_la_CFLAGS = -DBUILD_FT2 -DPANGO_MODULE_PREFIX=_pango_hebrew_ft2


included-modules: $(noinst_LTLIBRARIES)

.PHONY: included-modules
