## Process this file with automake to create Makefile.in.

noinst_LTLIBRARIES = pango-basic.la

INCLUDES = -I$(top_srcdir)/libpango/

pango_basic_la_SOURCES = basic.c
pango_basic_la_LDFLAGS = -rpath $(libdir) -export-dynamic -avoid-version -module

EXTRA_DIST= \
	tables-small.i \
	tables-big.i
