## Process this file with automake to create Makefile.in.

pangolibs = $(top_builddir)/pango/libpango-$(PANGO_API_VERSION).la $(GLIB_LIBS)
pangoft2libs = $(top_builddir)/pango/libpangoft2-$(PANGO_API_VERSION).la $(FREETYPE_LIBS) $(pangolibs)

INCLUDES =				\
	-DG_LOG_DOMAIN=\"Pango\"	\
	-DPANGO_ENABLE_ENGINE		\
	-DG_DISABLE_DEPRECATED		\
	$(PANGO_DEBUG_FLAGS)		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/pango/		\
	$(GLIB_CFLAGS)

moduledir = $(libdir)/pango/$(PANGO_MODULE_VERSION)/modules
module_LTLIBRARIES =
noinst_LTLIBRARIES =

common_sources = hebrew-shaper.c hebrew-shaper.h


if HAVE_FREETYPE
INCLUDES += $(FREETYPE_CFLAGS)
if INCLUDE_HEBREW_FC
noinst_LTLIBRARIES += libpango-hebrew-fc.la
else
module_LTLIBRARIES += pango-hebrew-fc.la
endif
endif

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

hebrew_fc_sources = $(common_sources) hebrew-fc.c 

pango_hebrew_fc_la_LDFLAGS = -export-dynamic -avoid-version -module $(no_undefined)
pango_hebrew_fc_la_LIBADD = $(pangoft2libs)
pango_hebrew_fc_la_SOURCES = $(hebrew_fc_sources)
libpango_hebrew_fc_la_SOURCES = $(hebrew_fc_sources)
libpango_hebrew_fc_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_hebrew_fc


included-modules: $(noinst_LTLIBRARIES)

.PHONY: included-modules
