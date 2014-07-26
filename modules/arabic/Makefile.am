include $(top_srcdir)/modules/Module.mk


if INCLUDE_ARABIC_LANG
noinst_LTLIBRARIES += libpango-arabic-lang.la
endif

libpango_arabic_lang_la_SOURCES = arabic-lang.c
libpango_arabic_lang_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_arabic_lang

-include $(top_srcdir)/git.mk
