INCLUDES = 				\
	-DG_LOG_DOMAIN=\"Pango\"	\
	-DPANGO_ENABLE_ENGINE		\
	$(PANGO_DEBUG_FLAGS)		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/pango		\
	$(GLIB_CFLAGS)

if HAVE_LIBTHAI
noinst_LTLIBRARIES = libpango-thai-lang.la
endif

libpango_thai_lang_la_SOURCES = thai-lang.c
libpango_thai_lang_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_thai_lang $(LIBTHAI_CFLAGS)
libpango_thai_lang_la_LIBADD = $(LIBTHAI_LIBS)

-include $(top_srcdir)/git.mk
