## Process this file with automake to create Makefile.in.

pangolibs = $(top_builddir)/pango/libpango-$(PANGO_API_VERSION).la $(GLIB_LIBS)
pangoft2libs = $(top_builddir)/pango/libpangoft2-$(PANGO_API_VERSION).la $(FREETYPE_LIBS) $(pangolibs)

INCLUDES = 				\
	-DPANGO_ENABLE_ENGINE		\
	-DG_DISABLE_DEPRECATED		\
	$(PANGO_DEBUG_FLAGS)		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/pango		\
	$(GLIB_CFLAGS)

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

moduledir = $(libdir)/pango/$(PANGO_MODULE_VERSION)/modules
module_LTLIBRARIES =
noinst_LTLIBRARIES =


if HAVE_FREETYPE
INCLUDES += $(FREETYPE_CFLAGS)
if INCLUDE_HANGUL_FC
noinst_LTLIBRARIES += libpango-hangul-fc.la
else
module_LTLIBRARIES += pango-hangul-fc.la
endif
endif

fc_sources = hangul-fc.c hangul-defs.h

pango_hangul_fc_la_LDFLAGS = -export-dynamic -avoid-version -module $(no_undefined)
pango_hangul_fc_la_LIBADD = $(pangoft2libs)
pango_hangul_fc_la_SOURCES = $(fc_sources)
libpango_hangul_fc_la_SOURCES = $(fc_sources)
libpango_hangul_fc_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_hangul_fc


EXTRA_DIST =			\
	tables-jamos.i

included-modules: $(noinst_LTLIBRARIES)

.PHONY: included-modules
