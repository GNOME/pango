INCLUDES = 				\
	-DG_LOG_DOMAIN=\"Pango\"	\
	-DPANGO_ENABLE_ENGINE		\
	$(PANGO_DEBUG_FLAGS)		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/pango		\
	$(GLIB_CFLAGS)

noinst_LTLIBRARIES = libpango-arabic-lang.la

libpango_arabic_lang_la_SOURCES = arabic-lang.c
libpango_arabic_lang_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_arabic_lang

-include $(top_srcdir)/git.mk
