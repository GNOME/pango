## Process this file with automake to create Makefile.in.

if HAVE_XFT
XFT_MODULES=pango-tamil-xft.la
if INCLUDE_TAMIL_XFT
XFT_INCLUDED=libpango-tamil-xft.la
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
	tamil-x.c \
	tadefs.h

xft_sources = \
	tamil-xft.c \
	tadefs.h

if HAVE_X
if INCLUDE_TAMIL_X
X_MODULES=
X_INCLUDED=libpango-tamil-x.la
X_PREFIX=-DX_MODULE_PREFIX
else
X_MODULES=pango-tamil-x.la
X_INCLUDED=
X_PREFIX=
endif
else
X_MODULES=
X_INCLUDED=
X_PREFIX=
endif

noinst_LTLIBRARIES = $(X_INCLUDED) $(XFT_INCLUDED)
module_LTLIBRARIES = $(X_MODULES) $(XFT_MODULES)
moddefine = $(X_PREFIX) $(XFT_PREFIX)
moduledir = $(libdir)/pango/modules
moduleflags=-rpath $(libdir)

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/pango/ $(moddefine) $(X_CFLAGS) $(FREETYPE_CFLAGS)

pango_tamil_x_la_LDFLAGS = $(moduleflags) -export-dynamic -avoid-version -module
pango_tamil_x_la_SOURCES = $(x_sources)

libpango_tamil_x_la_SOURCES = $(x_sources)

pango_tamil_xft_la_LDFLAGS = -rpath $(libdir) -export-dynamic -avoid-version -module
pango_tamil_xft_la_LIBADD = 
pango_tamil_xft_la_SOURCES = $(xft_sources)

libpango_tamil_xft_la_SOURCES = $(xft_sources)
