## Process this file with automake to create Makefile.in.

sources = tamil.c taconv.c taconv.h tadefs.h

if INCLUDE_TAMIL
noinst_LTLIBRARIES = libpango-tamil.la
moddefine = -DMODULE_PREFIX=_pango_tamil_
else
moduledir = $(libdir)/pango/modules
module_LTLIBRARIES = pango-tamil.la
moduleflags=-rpath $(libdir)
endif

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/pango/ $(moddefine)

pango_tamil_la_LDFLAGS = $(moduleflags) -export-dynamic -avoid-version -module
pango_tamil_la_SOURCES = $(sources)

libpango_tamil_la_SOURCES = $(sources)
