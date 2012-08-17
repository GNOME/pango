include $(top_srcdir)/modules/Module.mk

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

-include $(top_srcdir)/git.mk
