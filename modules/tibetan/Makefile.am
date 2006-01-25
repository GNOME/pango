## Process this file with automake to create Makefile.in.

pangolibs = $(top_builddir)/pango/libpango-$(PANGO_API_VERSION).la $(GLIB_LIBS)
pangoft2libs = $(top_builddir)/pango/libpangoft2-$(PANGO_API_VERSION).la $(FREETYPE_LIBS) $(pangolibs)

INCLUDES =				\
	-DG_LOG_DOMAIN=\"Pango\"	\
	-DPANGO_ENABLE_ENGINE		\
	$(PANGO_DEBUG_FLAGS)		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/pango		\
	$(GLIB_CFLAGS)

moduledir = $(libdir)/pango/$(PANGO_MODULE_VERSION)/modules
module_LTLIBRARIES =
noinst_LTLIBRARIES =

if HAVE_FREETYPE
INCLUDES += $(FREETYPE_CFLAGS)
if INCLUDE_TIBETAN_FC
noinst_LTLIBRARIES += libpango-tibetan-fc.la
else
if DYNAMIC_TIBETAN_FC
module_LTLIBRARIES += pango-tibetan-fc.la
endif
endif
endif

tibetan_fc_sources = $(common_sources) tibetan-fc.c

pango_tibetan_fc_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS)
pango_tibetan_fc_la_LIBADD = $(pangoft2libs)
pango_tibetan_fc_la_SOURCES = $(tibetan_fc_sources)
libpango_tibetan_fc_la_SOURCES = $(tibetan_fc_sources)
libpango_tibetan_fc_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_tibetan_fc


included-modules: $(noinst_LTLIBRARIES)
dynamic-modules: $(module_LTLIBRARIES)

.PHONY: included-modules dynamic-modules
