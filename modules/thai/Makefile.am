## Process this file with automake to create Makefile.in.

sources = thai.c

if INCLUDE_THAI
noinst_LTLIBRARIES = libpango-thai.la
moddefine = -DMODULE_PREFIX
else
moduledir = $(libdir)/pango/modules
module_LTLIBRARIES = pango-thai.la
moduleflags=-rpath $(libdir)
endif

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/pango/ $(moddefine)

pango_thai_la_LDFLAGS = $(moduleflags) -export-dynamic -avoid-version -module
pango_thai_la_LIBADD =
pango_thai_la_SOURCES = $(sources)

libpango_thai_la_SOURCES = $(sources)

