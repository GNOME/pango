include $(top_srcdir)/modules/Module.mk

shaper_sources = thai-charprop.c thai-charprop.h thai-shaper.c thai-shaper.h

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

thai_fc_sources = $(shaper_sources) thai-fc.c

pango_thai_fc_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS)
pango_thai_fc_la_LIBADD = $(pangoft2libs)
pango_thai_fc_la_SOURCES = $(thai_fc_sources)
libpango_thai_fc_la_SOURCES = $(thai_fc_sources)
libpango_thai_fc_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_thai_fc


if INCLUDE_THAI_LANG
noinst_LTLIBRARIES += libpango-thai-lang.la
else
if DYNAMIC_THAI_LANG
module_LTLIBRARIES += pango-thai-lang.la
endif
endif

lang_sources = \
	thai-lang.c

pango_thai_lang_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS)
pango_thai_lang_la_LIBADD = $(pangolibs) $(LIBTHAI_LIBS)
pango_thai_lang_la_SOURCES = $(lang_sources)
libpango_thai_lang_la_SOURCES = $(lang_sources)
libpango_thai_lang_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_thai_lang $(LIBTHAI_CFLAGS)
libpango_thai_lang_la_LIBADD = $(LIBTHAI_LIBS)
