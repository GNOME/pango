## Process this file with automake to create Makefile.in.

sources = hebrew-x.c

if HAVE_X
if INCLUDE_HEBREW_X
noinst_LTLIBRARIES = libpango-hebrew-x.la
moddefine = -DX_MODULE_PREFIX
else
moduledir = $(libdir)/pango/modules
module_LTLIBRARIES = pango-hebrew-x.la
moduleflags=-rpath $(libdir)
endif
endif

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/pango/ $(moddefine) $(X_CFLAGS)

pango_hebrew_x_la_LDFLAGS = $(moduleflags) -export-dynamic -avoid-version -module
pango_hebrew_x_la_LIBADD =
pango_hebrew_x_la_SOURCES = $(sources)

libpango_hebrew_x_la_SOURCES = $(sources)

