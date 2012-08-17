include $(top_srcdir)/modules/Module.mk

if INCLUDE_INDIC_LANG
noinst_LTLIBRARIES += libpango-indic-lang.la
else
if DYNAMIC_INDIC_LANG
module_LTLIBRARIES += pango-indic-lang.la
endif
endif

lang_sources =		\
	indic-lang.c

pango_indic_lang_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS)
pango_indic_lang_la_LIBADD = $(pangolibs)
pango_indic_lang_la_SOURCES = $(lang_sources)
libpango_indic_lang_la_SOURCES = $(lang_sources)
libpango_indic_lang_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_indic_lang

-include $(top_srcdir)/git.mk
