## Process this file with automake to create Makefile.in.

sources =		 	\
	devanagari.c		\
	dev-ligatures.h

if INCLUDE_DEVANAGARI
noinst_LTLIBRARIES = libpango-devanagari.la
moddefine = -DMODULE_PREFIX
else
moduledir = $(libdir)/pango/modules
module_LTLIBRARIES = pango-devanagari.la
endif

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/pango/ $(moddefine)

pango_devanagari_la_LDFLAGS = -rpath $(libdir) -export-dynamic -avoid-version -module
pango_devanagari_la_SOURCES = $(sources)

libpango_devanagari_la_SOURCES = $(sources)

