include $(top_srcdir)/modules/Module.mk

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
