## Process this file with automake to create Makefile.in.

sources = basic.c

if INCLUDE_BASIC
noinst_LTLIBRARIES = libpango-basic.la
moddefine = -DMODULE_PREFIX
else
moduledir = $(libdir)/pango/modules
module_LTLIBRARIES = pango-basic.la
endif

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/pango/ $(moddefine)

pango_basic_la_LDFLAGS = -rpath $(libdir) -export-dynamic -avoid-version -module
pango_basic_la_LIBADD = $(UNICODE_LIBS)
pango_basic_la_SOURCES = $(sources)

libpango_basic_la_SOURCES = $(sources)

