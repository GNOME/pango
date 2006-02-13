include $(top_srcdir)/modules/Module.mk

if HAVE_FREETYPE
INCLUDES += $(FREETYPE_CFLAGS)
if INCLUDE_ARABIC_FC
noinst_LTLIBRARIES += libpango-arabic-fc.la
else
if DYNAMIC_ARABIC_FC
module_LTLIBRARIES += pango-arabic-fc.la
endif
endif
endif

fc_sources = \
	arabic-fc.c \
	arabic-ot.c \
	arabic-ot.h

pango_arabic_fc_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS)
pango_arabic_fc_la_LIBADD = $(pangoft2libs)
pango_arabic_fc_la_SOURCES = $(fc_sources)
libpango_arabic_fc_la_SOURCES = $(fc_sources)
libpango_arabic_fc_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_arabic_fc
