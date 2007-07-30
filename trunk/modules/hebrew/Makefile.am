include $(top_srcdir)/modules/Module.mk

common_sources = hebrew-shaper.c hebrew-shaper.h

if HAVE_FREETYPE
INCLUDES += $(FREETYPE_CFLAGS)
if INCLUDE_HEBREW_FC
noinst_LTLIBRARIES += libpango-hebrew-fc.la
else
if DYNAMIC_HEBREW_FC
module_LTLIBRARIES += pango-hebrew-fc.la
endif
endif
endif

hebrew_fc_sources = $(common_sources) hebrew-fc.c 

pango_hebrew_fc_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS)
pango_hebrew_fc_la_LIBADD = $(pangoft2libs)
pango_hebrew_fc_la_SOURCES = $(hebrew_fc_sources)
libpango_hebrew_fc_la_SOURCES = $(hebrew_fc_sources)
libpango_hebrew_fc_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_hebrew_fc
