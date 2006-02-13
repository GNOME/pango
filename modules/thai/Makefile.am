include $(top_srcdir)/modules/Module.mk

common_sources = thai-charprop.c thai-charprop.h thai-shaper.c thai-shaper.h

if HAVE_FREETYPE
INCLUDES += $(FREETYPE_CFLAGS)
if INCLUDE_THAI_FC
noinst_LTLIBRARIES += libpango-thai-fc.la
else
if DYNAMIC_THAI_FC
module_LTLIBRARIES += pango-thai-fc.la
endif
endif
endif

thai_fc_sources = $(common_sources) thai-fc.c thai-ot.c thai-ot.h

pango_thai_fc_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS)
pango_thai_fc_la_LIBADD = $(pangoft2libs)
pango_thai_fc_la_SOURCES = $(thai_fc_sources)
libpango_thai_fc_la_SOURCES = $(thai_fc_sources)
libpango_thai_fc_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_thai_fc
