## Process this file with automake to create Makefile.in.

if HAVE_XFT
XFT_MODULES=pango-arabic-xft.la
if INCLUDE_ARABIC_XFT
XFT_INCLUDED=libpango-arabic-xft.la
XFT_PREFIX=-DXFT_MODULE_PREFIX
else
XFT_INCLUDED=
XFT_PREFIX=
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

if INCLUDE_ARABIC_X
X_MODULES=
X_INCLUDED=libpango-arabic-x.la
X_PREFIX=-DX_MODULE_PREFIX
else
X_MODULES=pango-arabic-x.la
X_INCLUDED=
X_PREFIX=
endif


noinst_LTLIBRARIES = $(X_INCLUDED) $(XFT_INCLUDED)
module_LTLIBRARIES = $(X_MODULES) $(XFT_MODULES)
moddefine = $(X_PREFIX) $(XFT_PREFIX)
moduledir = $(libdir)/pango/modules
moduleflags=-rpath $(libdir)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/pango/ $(moddefine) $(X_CFLAGS) $(FREETYPE_CFLAGS)

pango_arabic_x_la_LDFLAGS = $(moduleflags) -export-dynamic -avoid-version -module
pango_arabic_x_la_SOURCES = $(x_sources)

libpango_arabic_x_la_SOURCES = $(x_sources)

pango_arabic_xft_la_LDFLAGS = -rpath $(libdir) -export-dynamic -avoid-version -module
pango_arabic_xft_la_LIBADD = 
pango_arabic_xft_la_SOURCES = $(xft_sources)

libpango_arabic_xft_la_SOURCES = $(xft_sources)


