## Process this file with automake to create Makefile.in.

pangolibs = $(top_builddir)/pango/libpango-$(PANGO_API_VERSION).la $(GLIB_LIBS)
pangoxlibs = $(top_builddir)/pango/libpangox-$(PANGO_API_VERSION).la $(X_LIBS) $(pangolibs)
pangoxftlibs = $(top_builddir)/pango/libpangoxft-$(PANGO_API_VERSION).la $(XFT_LIBS) $(pangolibs)
pangoft2libs = $(top_builddir)/pango/libpangoft2-$(PANGO_API_VERSION).la $(FREETYPE_LIBS) $(pangolibs)

INCLUDES = 				\
	-DPANGO_ENABLE_ENGINE		\
	-DG_DISABLE_DEPRECATED		\
	$(PANGO_DEBUG_FLAGS)		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/pango/		\
	$(GLIB_CFLAGS)

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

moduledir = $(libdir)/pango/$(PANGO_MODULE_VERSION)/modules
module_LTLIBRARIES =
noinst_LTLIBRARIES =

if HAVE_X
INCLUDES += $(X_CFLAGS)
if INCLUDE_HANGUL_X
noinst_LTLIBRARIES += libpango-hangul-x.la
else
module_LTLIBRARIES += pango-hangul-x.la
endif
endif

x_sources = hangul-x.c hangul-defs.h

pango_hangul_x_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_hangul_x_la_LIBADD = $(pangoxlibs)
pango_hangul_x_la_SOURCES = $(x_sources)
libpango_hangul_x_la_SOURCES = $(x_sources)
libpango_hangul_x_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_hangul_x


if HAVE_XFT
INCLUDES += $(XFT_CFLAGS)
if INCLUDE_HANGUL_XFT
noinst_LTLIBRARIES += libpango-hangul-xft.la
else
module_LTLIBRARIES += pango-hangul-xft.la
endif
endif

xft_sources = hangul-fc.c hangul-defs.h

pango_hangul_xft_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_hangul_xft_la_LIBADD = $(pangoxftlibs)
pango_hangul_xft_la_SOURCES = $(xft_sources)
pango_hangul_xft_la_CFLAGS = -DBUILD_XFT
libpango_hangul_xft_la_SOURCES = $(xft_sources)
libpango_hangul_xft_la_CFLAGS = -DBUILD_XFT -DPANGO_MODULE_PREFIX=_pango_hangul_xft


if HAVE_FREETYPE
INCLUDES += $(FREETYPE_CFLAGS)
if INCLUDE_HANGUL_FT2
noinst_LTLIBRARIES += libpango-hangul-ft2.la
else
module_LTLIBRARIES += pango-hangul-ft2.la
endif
endif

ft2_sources = hangul-fc.c hangul-defs.h

pango_hangul_ft2_la_LDFLAGS = -export-dynamic -avoid-version -module $(no_undefined)
pango_hangul_ft2_la_LIBADD = $(pangoft2libs)
pango_hangul_ft2_la_SOURCES = $(ft2_sources)
pango_hangul_ft2_la_CFLAGS = -DBUILD_FT2
libpango_hangul_ft2_la_SOURCES = $(ft2_sources)
libpango_hangul_ft2_la_CFLAGS = -DBUILD_FT2 -DPANGO_MODULE_PREFIX=_pango_hangul_ft2


EXTRA_DIST =			\
	tables-ksc5601.i	\
	tables-johabfont.i	\
	tables-jamos.i

included-modules: $(noinst_LTLIBRARIES)

.PHONY: included-modules
