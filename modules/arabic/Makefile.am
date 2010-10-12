include $(top_srcdir)/modules/Module.mk


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

-include $(top_srcdir)/git.mk
