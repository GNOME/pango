## Process this file with automake to create Makefile.in.

pangolibs = $(top_builddir)/pango/libpango-$(PANGO_API_VERSION).la $(FRIBIDI_LIBS) $(GLIB_LIBS)
pangoxlibs = $(top_builddir)/pango/libpangox-$(PANGO_API_VERSION).la $(X_LIBS) $(pangolibs)
pangoxftlibs = $(top_builddir)/pango/libpangoxft-$(PANGO_API_VERSION).la $(XFT_LIBS) $(pangolibs)
pangoft2libs = $(top_builddir)/pango/libpangoft2-$(PANGO_API_VERSION).la $(FREETYPE_LIBS) $(pangolibs)

if HAVE_XFT
if INCLUDE_ARABIC_XFT
XFT_MODULES=
XFT_INCLUDED=libpango-arabic-xft.la
XFT_PREFIX=-DXFT_MODULE_PREFIX
else
XFT_MODULES=pango-arabic-xft.la
XFT_INCLUDED=
XFT_PREFIX=
arabic_xft_libadd=$(pangoxftlibs)
endif
else
XFT_MODULES=
XFT_INCLUDED=
XFT_PREFIX=
endif

x_sources = \
	arabic-x.c \
	arconv.c \
	mulefont.c \
	mulefont.h \
	langboxfont.c \
	langboxfont.h \
	naqshfont.c \
	naqshfont.h \
	arconv.h 

xft_sources = \
	arabic-xft.c \
	arabic-ot.c \
	arabic-ot.h

ft2_sources = \
	arabic-ft2.c \
	arabic-ot.c \
	arabic-ot.h

if HAVE_X
if INCLUDE_ARABIC_X
X_MODULES=
X_INCLUDED=libpango-arabic-x.la
X_PREFIX=-DX_MODULE_PREFIX
else
X_MODULES=pango-arabic-x.la
X_INCLUDED=
X_PREFIX=
arabic_x_libadd=$(pangoxlibs)
endif
else
X_MODULES=
X_INCLUDED=
X_PREFIX=
endif

if HAVE_FREETYPE
if INCLUDE_ARABIC_FT2
FT2_MODULES=
FT2_INCLUDED=libpango-arabic-ft2.la
FT2_PREFIX=-DFT2_MODULE_PREFIX
else
FT2_MODULES=pango-arabic-ft2.la
FT2_INCLUDED=
FT2_PREFIX=
arabic_ft2_libadd=$(pangoft2libs)
endif
else
FT2_MODULES=
FT2_INCLUDED=
FT2_PREFIX=
endif

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

noinst_LTLIBRARIES = $(X_INCLUDED) $(XFT_INCLUDED) $(FT2_INCLUDED)
module_LTLIBRARIES = $(X_MODULES) $(XFT_MODULES) $(FT2_MODULES)
moddefine = $(X_PREFIX) $(XFT_PREFIX) $(FT2_PREFIX)
moduledir = $(libdir)/pango/$(PANGO_MODULE_VERSION)/modules

INCLUDES = -DPANGO_ENABLE_ENGINE -DG_DISABLE_DEPRECATED -I$(top_srcdir) -I$(top_srcdir)/pango/ $(moddefine) $(X_CFLAGS) $(FREETYPE_CFLAGS)

pango_arabic_x_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_arabic_x_la_LIBADD = $(arabic_x_libadd)
pango_arabic_x_la_SOURCES = $(x_sources)

libpango_arabic_x_la_SOURCES = $(x_sources)

pango_arabic_xft_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_arabic_xft_la_LIBADD = $(arabic_xft_libadd)
pango_arabic_xft_la_SOURCES = $(xft_sources)

libpango_arabic_xft_la_SOURCES = $(xft_sources)

pango_arabic_ft2_la_LDFLAGS = -export-dynamic -avoid-version -module $(no_undefined)
pango_arabic_ft2_la_LIBADD = $(arabic_ft2_libadd)
pango_arabic_ft2_la_SOURCES = $(ft2_sources)

libpango_arabic_ft2_la_SOURCES = $(ft2_sources)

included-modules: $(noinst_LTLIBRARIES)

.PHONY: included-modules
