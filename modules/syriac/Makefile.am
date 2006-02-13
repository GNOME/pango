include $(top_srcdir)/modules/Module.mk

if HAVE_FREETYPE
INCLUDES += $(FREETYPE_CFLAGS)
if INCLUDE_SYRIAC_FC
noinst_LTLIBRARIES += libpango-syriac-fc.la
else
if DYNAMIC_SYRIAC_FC
module_LTLIBRARIES += pango-syriac-fc.la
endif
endif
endif

fc_sources = \
	syriac-fc.c \
	syriac-ot.c \
	syriac-ot.h

pango_syriac_fc_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS)
pango_syriac_fc_la_LIBADD = $(pangoft2libs)
pango_syriac_fc_la_SOURCES = $(fc_sources)
libpango_syriac_fc_la_SOURCES = $(fc_sources)
libpango_syriac_fc_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_syriac_fc
