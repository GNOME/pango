## Process this file with automake to create Makefile.in.

moduledir = $(libdir)/pango/modules
module_LTLIBRARIES = pango-tamil.la

INCLUDES = -I$(top_srcdir)/libpango/

pango_tamil_la_SOURCES = tamil.c taconv.c taconv.h tadefs.h
pango_tamil_la_LDFLAGS = -rpath $(libdir) -export-dynamic -avoid-version -module

EXTRA_DIST= 
