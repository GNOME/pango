## Process this file with automake to create Makefile.in.

basic_sources = basic-x.c

pangolibs = $(top_builddir)/pango/libpango-$(PANGO_API_VERSION).la $(FRIBIDI_LIBS) $(GLIB_LIBS)
pangoxlibs = $(top_builddir)/pango/libpangox-$(PANGO_API_VERSION).la $(X_LIBS) $(pangolibs)
pangoxftlibs = $(top_builddir)/pango/libpangoxft-$(PANGO_API_VERSION).la $(XFT_LIBS) $(pangolibs)
pangoft2libs = $(top_builddir)/pango/libpangoft2-$(PANGO_API_VERSION).la $(FREETYPE_LIBS) $(pangolibs)
pangowin32libs = $(top_builddir)/pango/libpangowin32-$(PANGO_API_VERSION).la $(pangolibs)

INCLUDES = 			\
	-DPANGO_ENABLE_ENGINE	\
	-DG_DISABLE_DEPRECATED	\
	$(PANGO_DEBUG_FLAGS)	\
	-I$(top_srcdir)		\
	-I$(top_srcdir)/pango/	\
	$(GLIB_CFLAGS)		\
	$(moddefine)

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

moduledir = $(libdir)/pango/$(PANGO_MODULE_VERSION)/modules
module_LTLIBRARIES =
noinst_LTLIBRARIES =

moddefine =

if HAVE_X
INCLUDES += $(X_CFLAGS)
if INCLUDE_BASIC_X
noinst_LTLIBRARIES += libpango-basic-x.la
INCLUDES += -DX_MODULE_PREFIX
else
module_LTLIBRARIES += pango-basic-x.la
endif
endif

pango_basic_x_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_basic_x_la_LIBADD = $(pangoxlibs)
pango_basic_x_la_SOURCES = basic-x.c
libpango_basic_x_la_SOURCES = basic-x.c


if HAVE_XFT
INCLUDES += $(XFT_CFLAGS)
if INCLUDE_BASIC_XFT
noinst_LTLIBRARIES += libpango-basic-xft.la
INCLUDES += -DXFT_MODULE_PREFIX
else
module_LTLIBRARIES += pango-basic-xft.la
endif
endif

pango_basic_xft_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_basic_xft_la_LIBADD = $(pangoxftlibs)
pango_basic_xft_la_SOURCES = basic-xft.c
libpango_basic_xft_la_SOURCES = basic-xft.c


if HAVE_FREETYPE
INCLUDES += $(FREETYPE_CFLAGS)
if INCLUDE_BASIC_FT2
noinst_LTLIBRARIES += libpango-basic-ft2.la
INCLUDES += -DFT2_MODULE_PREFIX
else
module_LTLIBRARIES += pango-basic-ft2.la
endif
endif

pango_basic_ft2_la_LDFLAGS = -export-dynamic -avoid-version -module $(no_undefined)
pango_basic_ft2_la_LIBADD = $(pangoft2libs)
pango_basic_ft2_la_SOURCES = basic-ft2.c
libpango_basic_ft2_la_SOURCES = basic-ft2.c


if HAVE_WIN32
if INCLUDE_BASIC_WIN32
noinst_LTLIBRARIES += libpango-basic-win32.la
moddefine += -DWIN32_MODULE_PREFIX
else
module_LTLIBRARIES += pango-basic-win32.la
endif
endif

pango_basic_win32_la_LDFLAGS = -export-dynamic -avoid-version -module $(no_undefined)
pango_basic_win32_la_LIBADD = $(pangowin32libs) -lgdi32
pango_basic_win32_la_SOURCES = basic-win32.c
libpango_basic_win32_la_SOURCES = basic-win32.c

if HAVE_USP10_H
basic-win32.lo: usp10.h

# Do it like this to avoid getting other headers from where usp10.h is
usp10.h: $(USP10_H)
	cp -p $(USP10_H) usp10.h
endif


EXTRA_DIST =		\
	basic-common.h	\
	tables-big.i	\
	tables-small.i

included-modules: $(noinst_LTLIBRARIES)

.PHONY: included-modules
