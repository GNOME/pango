## Process this file with automake to create Makefile.in.

basic_sources = basic.c

if HAVE_XFT
XFT_MODULES=pango-basic-xft.la
else
XFT_MODULES=
endif

if INCLUDE_BASIC
noinst_LTLIBRARIES = libpango-basic.la
moddefine = -DMODULE_PREFIX

else
moduledir = $(libdir)/pango/modules
module_LTLIBRARIES = $(XFT_MODULES) pango-basic.la
moduleflags=-rpath $(libdir)
endif

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/pango/ $(X_CFLAGS) $(moddefine)

pango_basic_la_LDFLAGS = $(moduleflags) -export-dynamic -avoid-version -module
pango_basic_la_LIBADD = 
pango_basic_la_SOURCES = $(basic_sources)

libpango_basic_la_SOURCES = $(basic_sources)

pango_basic_xft_la_LDFLAGS = -rpath $(libdir) -export-dynamic -avoid-version -module
pango_basic_xft_la_LIBADD = 
pango_basic_xft_la_SOURCES = basic-xft.c

EXTRA_DIST =		\
	tables-big.i	\
	tables-small.i	\
	basic-win32.c	\
	basic-ft2.c	\
	makefile.mingw	\
	makefile.mingw.in
