## Process this file with automake to create Makefile.in.

pangolibs = $(top_builddir)/pango/libpango-$(PANGO_API_VERSION).la $(GLIB_LIBS)
pangoft2libs = $(top_builddir)/pango/libpangoft2-$(PANGO_API_VERSION).la $(FREETYPE_LIBS) $(pangolibs)

INCLUDES =			\
	-DPANGO_ENABLE_ENGINE	\
	-DG_DISABLE_DEPRECATED	\
	$(PANGO_DEBUG_FLAGS)	\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/pango/	\
	$(GLIB_CFLAGS)

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

moduledir = $(libdir)/pango/$(PANGO_MODULE_VERSION)/modules
module_LTLIBRARIES =
noinst_LTLIBRARIES =

common_sources = thai-shaper.c thai-shaper.h


if HAVE_FREETYPE
INCLUDES += $(FREETYPE_CFLAGS)
if INCLUDE_THAI_FC
noinst_LTLIBRARIES += libpango-thai-fc.la
else
module_LTLIBRARIES += pango-thai-fc.la
endif
endif

thai_fc_sources = $(common_sources) thai-fc.c thai-ot.c thai-ot.h

pango_thai_fc_la_LDFLAGS = -export-dynamic -avoid-version -module $(no_undefined)
pango_thai_fc_la_LIBADD = $(pangoft2libs)
pango_thai_fc_la_SOURCES = $(thai_fc_sources)
libpango_thai_fc_la_SOURCES = $(thai_fc_sources)
libpango_thai_fc_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_thai_fc


included-modules: $(noinst_LTLIBRARIES)

.PHONY: included-modules
