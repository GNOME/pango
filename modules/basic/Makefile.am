## Process this file with automake to create Makefile.in.

moduledir = $(libdir)/pango/modules
module_LTLIBRARIES = pango-basic.la

pango_basic_la_SOURCES = basic.c
pango_basic_la_LDFLAGS = -rpath $(libdir) -export-dynamic -avoid-version -module

INCLUDES = -I$(top_srcdir)/libpango/


EXTRA_DIST= \
	tables-small.i \
	tables-big.i
