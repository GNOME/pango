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
if INCLUDE_ARABIC_FC
noinst_LTLIBRARIES += libpango-arabic-fc.la
else
module_LTLIBRARIES += pango-arabic-fc.la
endif
endif

fc_sources = \
	arabic-fc.c \
	arabic-ot.c \
	arabic-ot.h

pango_arabic_fc_la_LDFLAGS = -export-dynamic -avoid-version -module $(no_undefined)
pango_arabic_fc_la_LIBADD = $(pangoft2libs)
pango_arabic_fc_la_SOURCES = $(fc_sources)
libpango_arabic_fc_la_SOURCES = $(fc_sources)
libpango_arabic_fc_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_arabic_fc


included-modules: $(noinst_LTLIBRARIES)

.PHONY: included-modules
