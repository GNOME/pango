## Process this file with automake to create Makefile.in.

sources = hangul.c

if INCLUDE_HANGUL
noinst_LTLIBRARIES = libpango-hangul.la
moddefine = -DMODULE_PREFIX
else
moduledir = $(libdir)/pango/modules
module_LTLIBRARIES = pango-hangul.la
endif

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/pango/ $(moddefine)

pango_hangul_la_LDFLAGS = -rpath $(libdir) -export-dynamic -avoid-version -module
pango_hangul_la_SOURCES = $(sources)

libpango_hangul_la_SOURCES = $(sources)

EXTRA_DIST =			\
	tables-ksc5601.i	\
	tables-johabfont.i
