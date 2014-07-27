INCLUDES = 				\
	-DG_LOG_DOMAIN=\"Pango\"	\
	-DPANGO_ENABLE_ENGINE		\
	$(PANGO_DEBUG_FLAGS)		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/pango		\
	$(GLIB_CFLAGS)

noinst_LTLIBRARIES = libpango-indic-lang.la

libpango_indic_lang_la_SOURCES = indic-lang.c
libpango_indic_lang_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_indic_lang

-include $(top_srcdir)/git.mk
