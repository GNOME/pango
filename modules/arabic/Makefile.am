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


if INCLUDE_ARABIC_LANG
noinst_LTLIBRARIES += libpango-arabic-lang.la
else
if DYNAMIC_ARABIC_LANG
module_LTLIBRARIES += pango-arabic-lang.la
endif
endif

lang_sources = \
	arabic-lang.c

pango_arabic_lang_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS)
pango_arabic_lang_la_LIBADD = $(pangolibs)
pango_arabic_lang_la_SOURCES = $(lang_sources)
libpango_arabic_lang_la_SOURCES = $(lang_sources)
libpango_arabic_lang_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_arabic_lang
