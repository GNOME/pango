## Process this file with automake to create Makefile.in.

sources = hangul-x.c

if INCLUDE_HANGUL_X
noinst_LTLIBRARIES = libpango-hangul-x.la
moddefine = -DX_MODULE_PREFIX
else
moduledir = $(libdir)/pango/modules
module_LTLIBRARIES = pango-hangul-x.la
moduleflags= -rpath $(libdir)
endif

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/pango/ $(moddefine)

pango_hangul_x_la_LDFLAGS = $(moduleflags) -export-dynamic -avoid-version -module
pango_hangul_x_la_SOURCES = $(sources)

libpango_hangul_x_la_SOURCES = $(sources)

EXTRA_DIST =			\
	tables-ksc5601.i	\
	tables-johabfont.i
