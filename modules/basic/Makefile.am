## Process this file with automake to create Makefile.in.

basic_sources = basic-x.c

pangolibs = $(top_builddir)/pango/libpango-$(PANGO_API_VERSION).la $(FRIBIDI_LIBS) $(GLIB_LIBS)
pangoxlibs = $(top_builddir)/pango/libpangox-$(PANGO_API_VERSION).la $(X_LIBS) $(pangolibs)
pangoxftlibs = $(top_builddir)/pango/libpangoxft-$(PANGO_API_VERSION).la $(XFT_LIBS) $(pangolibs)
pangoft2libs = $(top_builddir)/pango/libpangoft2-$(PANGO_API_VERSION).la $(FREETYPE_LIBS) $(pangolibs)
pangowin32libs = $(top_builddir)/pango/libpangowin32-$(PANGO_API_VERSION).la $(pangolibs)

if HAVE_XFT
if INCLUDE_BASIC_XFT
XFT_INCLUDED=libpango-basic-xft.la
XFT_MODULES=
XFT_PREFIX=-DXFT_MODULE_PREFIX
else
XFT_INCLUDED=
XFT_MODULES=pango-basic-xft.la
XFT_PREFIX=
basic_xft_libadd=$(pangoxlibs) $(pangoxftlibs)
endif
else
XFT_MODULES=
XFT_INCLUDED=
XFT_PREFIX=
endif

if HAVE_FREETYPE
if INCLUDE_BASIC_FT2
FT2_INCLUDED=libpango-basic-ft2.la
FT2_MODULES=
FT2_PREFIX=-DFT2_MODULE_PREFIX
else
FT2_INCLUDED=
FT2_MODULES=pango-basic-ft2.la
FT2_PREFIX=
basic_ft2_libadd=$(pangoft2libs)
endif
else
FT2_MODULES=
FT2_INCLUDED=
FT2_PREFIX=
endif

if HAVE_X
if INCLUDE_BASIC_X
X_INCLUDED=libpango-basic-x.la
X_MODULES=
X_PREFIX=-DX_MODULE_PREFIX
else
X_INCLUDED=
X_MODULES=pango-basic-x.la
X_PREFIX=
basic_x_libadd=$(pangoxlibs)
endif
else
X_INCLUDED=
X_MODULES=
X_PREFIX=
endif

if HAVE_WIN32
if INCLUDE_BASIC_WIN32
WIN32_INCLUDED=libpango-basic-win32.la
WIN32_MODULES=
WIN32_PREFIX=-DWIN32_MODULE_PREFIX
else
WIN32_INCLUDED=
WIN32_MODULES=pango-basic-win32.la
WIN32_PREFIX=
basic_win32_libadd=$(pangowin32libs)
endif
else
WIN32_INCLUDED=
WIN32_MODULES=
WIN32_PREFIX=
endif

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

noinst_LTLIBRARIES = $(X_INCLUDED) $(XFT_INCLUDED) $(FT2_INCLUDED) $(WIN32_INCLUDED)
module_LTLIBRARIES = $(X_MODULES) $(XFT_MODULES) $(FT2_MODULES) $(WIN32_MODULES)
moddefine = $(X_PREFIX) $(XFT_PREFIX) $(FT2_PREFIX) $(WIN32_PREFIX)
moduledir = $(libdir)/pango/$(PANGO_MODULE_VERSION)/modules

INCLUDES = 			\
	-DPANGO_ENABLE_ENGINE	\
	-DG_DISABLE_DEPRECATED	\
	-I$(top_srcdir) 	\
	-I$(top_srcdir)/pango/ 	\
	$(X_CFLAGS) 		\
	$(FREETYPE_CFLAGS)	\
	$(moddefine)

pango_basic_x_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_basic_x_la_LIBADD = $(basic_x_libadd)
pango_basic_x_la_SOURCES = $(basic_sources)
libpango_basic_x_la_SOURCES = $(basic_sources)

pango_basic_xft_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_basic_xft_la_LIBADD = $(basic_xft_libadd)
pango_basic_xft_la_SOURCES = basic-xft.c
libpango_basic_xft_la_SOURCES = basic-xft.c

pango_basic_ft2_la_LDFLAGS = -export-dynamic -avoid-version -module $(no_undefined)
pango_basic_ft2_la_LIBADD = $(basic_ft2_libadd)
pango_basic_ft2_la_SOURCES = basic-ft2.c
libpango_basic_ft2_la_SOURCES = basic-ft2.c

pango_basic_win32_la_LDFLAGS = -export-dynamic -avoid-version -module $(no_undefined)
pango_basic_win32_la_LIBADD = $(basic_win32_libadd)
pango_basic_win32_la_SOURCES = basic-win32.c
libpango_basic_win32_la_SOURCES = basic-win32.c

EXTRA_DIST =		\
	basic-common.h	\
	tables-big.i	\
	tables-small.i	\
	makefile.mingw	\
	makefile.mingw.in

included-modules: $(noinst_LTLIBRARIES)

.PHONY: included-modules
