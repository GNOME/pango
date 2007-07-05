include $(top_srcdir)/modules/Module.mk

if HAVE_X
INCLUDES += $(X_CFLAGS)
if INCLUDE_BASIC_X
noinst_LTLIBRARIES += libpango-basic-x.la
else
if DYNAMIC_BASIC_X
module_LTLIBRARIES += pango-basic-x.la
endif
endif
endif

pango_basic_x_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS)
pango_basic_x_la_LIBADD = $(pangoxlibs)
pango_basic_x_la_SOURCES = basic-x.c
libpango_basic_x_la_SOURCES = basic-x.c
libpango_basic_x_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_basic_x


if HAVE_FREETYPE
INCLUDES += $(FREETYPE_CFLAGS)
if INCLUDE_BASIC_FC
noinst_LTLIBRARIES += libpango-basic-fc.la
else
if DYNAMIC_BASIC_FC
module_LTLIBRARIES += pango-basic-fc.la
endif
endif
endif

pango_basic_fc_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS)
pango_basic_fc_la_LIBADD = $(pangoft2libs)
pango_basic_fc_la_SOURCES = basic-fc.c
libpango_basic_fc_la_SOURCES = basic-fc.c
libpango_basic_fc_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_basic_fc


if HAVE_WIN32
if INCLUDE_BASIC_WIN32
noinst_LTLIBRARIES += libpango-basic-win32.la
else
if DYNAMIC_BASIC_WIN32
module_LTLIBRARIES += pango-basic-win32.la
endif
endif
endif

pango_basic_win32_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS)
pango_basic_win32_la_LIBADD = $(pangowin32libs) -lgdi32
pango_basic_win32_la_SOURCES = basic-win32.c
libpango_basic_win32_la_SOURCES = basic-win32.c
libpango_basic_win32_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_basic_win32

if HAVE_CAIRO_ATSUI
INCLUDES += $(ATSUI_CFLAGS) $(CAIRO_CFLAGS)
if INCLUDE_BASIC_ATSUI
noinst_LTLIBRARIES += libpango-basic-atsui.la
else
if DYNAMIC_BASIC_ATSUI
module_LTLIBRARIES += pango-basic-atsui.la
endif
endif
endif

pango_basic_atsui_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS) -framework Carbon
pango_basic_atsui_la_LIBADD = $(pangoatsuilibs)
pango_basic_atsui_la_SOURCES = basic-atsui.c
libpango_basic_atsui_la_SOURCES = basic-atsui.c
libpango_basic_atsui_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_basic_atsui

EXTRA_DIST =		\
	tables-big.i	\
	tables-small.i
