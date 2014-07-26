include $(top_srcdir)/modules/Module.mk

if INCLUDE_THAI_LANG
noinst_LTLIBRARIES += libpango-thai-lang.la
endif

libpango_thai_lang_la_SOURCES = thai-lang.c
libpango_thai_lang_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_thai_lang $(LIBTHAI_CFLAGS)
libpango_thai_lang_la_LIBADD = $(LIBTHAI_LIBS)

-include $(top_srcdir)/git.mk
