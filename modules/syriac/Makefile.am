## Process this file with automake to create Makefile.in.

pangolibs = $(top_builddir)/pango/libpango-$(PANGO_API_VERSION).la $(GLIB_LIBS)
pangoft2libs = $(top_builddir)/pango/libpangoft2-$(PANGO_API_VERSION).la $(FREETYPE_LIBS) $(pangolibs)

INCLUDES = 				\
	-DG_LOG_DOMAIN=\"Pango\"	\
	-DPANGO_ENABLE_ENGINE		\
	-DG_DISABLE_DEPRECATED		\
	$(PANGO_DEBUG_FLAGS)		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/pango		\
	$(GLIB_CFLAGS)

moduledir = $(libdir)/pango/$(PANGO_MODULE_VERSION)/modules
module_LTLIBRARIES =
noinst_LTLIBRARIES =

if PLATFORM_WIN32
no_undefined = -no-undefined
endif


if HAVE_FREETYPE
INCLUDES += $(FREETYPE_CFLAGS)
if INCLUDE_SYRIAC_FC
noinst_LTLIBRARIES += libpango-syriac-fc.la
else
module_LTLIBRARIES += pango-syriac-fc.la
endif
endif

fc_sources = \
	syriac-fc.c \
	syriac-ot.c \
	syriac-ot.h

pango_syriac_fc_la_LDFLAGS = -export-dynamic -avoid-version -module $(no_undefined)
pango_syriac_fc_la_LIBADD = $(pangoft2libs)
pango_syriac_fc_la_SOURCES = $(fc_sources)
libpango_syriac_fc_la_SOURCES = $(fc_sources)
libpango_syriac_fc_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_syriac_fc


included-modules: $(noinst_LTLIBRARIES)

.PHONY: included-modules
