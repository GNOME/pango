include $(top_srcdir)/modules/Module.mk

noinst_LTLIBRARIES =

if HAVE_FREETYPE
INCLUDES += $(FREETYPE_CFLAGS) $(HARFBUZZ_CFLAGS)
noinst_LTLIBRARIES += libpango-basic-fc.la
endif

libpango_basic_fc_la_SOURCES = basic-fc.c
libpango_basic_fc_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_basic_fc


if HAVE_WIN32
noinst_LTLIBRARIES += libpango-basic-win32.la
endif

libpango_basic_win32_la_LIBADD = -lgdi32 -lusp10
libpango_basic_win32_la_SOURCES = basic-win32.c
libpango_basic_win32_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_basic_win32

if HAVE_CORE_TEXT
INCLUDES += $(CAIRO_CFLAGS)
noinst_LTLIBRARIES += libpango-basic-coretext.la
endif

libpango_basic_coretext_la_SOURCES = basic-coretext.c
libpango_basic_coretext_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_basic_coretext

-include $(top_srcdir)/git.mk
