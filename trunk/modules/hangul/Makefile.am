include $(top_srcdir)/modules/Module.mk

if HAVE_FREETYPE
INCLUDES += $(FREETYPE_CFLAGS)
if INCLUDE_HANGUL_FC
noinst_LTLIBRARIES += libpango-hangul-fc.la
else
if DYNAMIC_HANGUL_FC
module_LTLIBRARIES += pango-hangul-fc.la
endif
endif
endif

fc_sources = hangul-fc.c hangul-defs.h

pango_hangul_fc_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS)
pango_hangul_fc_la_LIBADD = $(pangoft2libs)
pango_hangul_fc_la_SOURCES = $(fc_sources)
libpango_hangul_fc_la_SOURCES = $(fc_sources)
libpango_hangul_fc_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_hangul_fc


EXTRA_DIST =			\
	tables-jamos.i
