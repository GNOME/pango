## Process this file with automake to create Makefile.in.

if HAVE_XFT
XFT_MODULES=pango-arabic-xft.la
else
XFT_MODULES=
endif

sources = \
	arabic.c \
	arconv.c \
	mulefont.c \
	mulefont.h \
	langboxfont.c \
	langboxfont.h \
	naqshfont.c \
	naqshfont.h \
	arconv.h 

if INCLUDE_ARABIC
noinst_LTLIBRARIES = libpango-arabic.la
moddefine = -DMODULE_PREFIX
else
moduledir = $(libdir)/pango/modules
module_LTLIBRARIES = $(XFT_MODULES) pango-arabic.la
moduleflags = -rpath $(libdir)
endif

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/pango/ $(moddefine) $(X_CFLAGS) $(FREETYPE_CFLAGS)

pango_arabic_la_LDFLAGS = $(moduleflags) -export-dynamic -avoid-version -module
pango_arabic_la_SOURCES = $(sources)

libpango_arabic_la_SOURCES = $(sources)


pango_arabic_xft_la_LDFLAGS = -rpath $(libdir) -export-dynamic -avoid-version -module
pango_arabic_xft_la_LIBADD = 
pango_arabic_xft_la_SOURCES = arabic-xft.c arabic-ot.c arabic-ot.h

