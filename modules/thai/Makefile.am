## Process this file with automake to create Makefile.in.

sources = thai-x.c

if INCLUDE_THAI_X
noinst_LTLIBRARIES = libpango-thai-x.la
moddefine = -DX_MODULE_PREFIX
else
moduledir = $(libdir)/pango/modules
module_LTLIBRARIES = pango-thai-x.la
moduleflags=-rpath $(libdir)
endif

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/pango/ $(moddefine) $(X_CFLAGS)

pango_thai_x_la_LDFLAGS = $(moduleflags) -export-dynamic -avoid-version -module
pango_thai_x_la_LIBADD =
pango_thai_x_la_SOURCES = $(sources)

libpango_thai_x_la_SOURCES = $(sources)

