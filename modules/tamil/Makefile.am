## Process this file with automake to create Makefile.in.

sources = tamil-x.c taconv.c taconv.h tadefs.h

if HAVE_X
if INCLUDE_TAMIL_X
noinst_LTLIBRARIES = libpango-tamil-x.la
moddefine = -DX_MODULE_PREFIX
else
moduledir = $(libdir)/pango/modules
module_LTLIBRARIES = pango-tamil-x.la
moduleflags=-rpath $(libdir)
endif
endif

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/pango/ $(moddefine) $(X_CFLAGS)

pango_tamil_x_la_LDFLAGS = $(moduleflags) -export-dynamic -avoid-version -module
pango_tamil_x_la_SOURCES = $(sources)

libpango_tamil_x_la_SOURCES = $(sources)
