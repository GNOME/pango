## Process this file with automake to create Makefile.in.

moduledir = $(libdir)/pango/modules
module_LTLIBRARIES = pango-hangul.la

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/pango/ 

pango_hangul_la_SOURCES = hangul.c
pango_hangul_la_LDFLAGS = -rpath $(libdir) -export-dynamic -avoid-version -module

EXTRA_DIST =			\
	tables-ksc5601.i	\
	tables-johabfont.i
