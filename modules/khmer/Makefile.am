include $(top_srcdir)/modules/Module.mk

if HAVE_FREETYPE
INCLUDES += $(FREETYPE_CFLAGS)
if INCLUDE_KHMER_FC
noinst_LTLIBRARIES += libpango-khmer-fc.la
else
if DYNAMIC_KHMER_FC
module_LTLIBRARIES += pango-khmer-fc.la
endif
endif
endif

fc_sources =		 	\
	khmer-fc.c

pango_khmer_fc_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS)
pango_khmer_fc_la_LIBADD = $(pangoft2libs)
pango_khmer_fc_la_SOURCES = $(fc_sources)
libpango_khmer_fc_la_SOURCES = $(fc_sources)
libpango_khmer_fc_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_khmer_fc
