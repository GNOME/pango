## Process this file with automake to create Makefile.in.

sources = \
	arabic.c \
	arconv.c \
	mulefont.c \
	mulefont.h \
	langboxfont.c \
	langboxfont.h \
	naqshfont.c \
	naqshfont.h \
	arconv.h 

if INCLUDE_ARABIC
noinst_LTLIBRARIES = libpango-arabic.la
moddefine = -DMODULE_PREFIX
else
moduledir = $(libdir)/pango/modules
module_LTLIBRARIES = pango-arabic.la
moduleflags = -rpath $(libdir)
endif

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/pango/ $(moddefine)

pango_arabic_la_LDFLAGS = $(moduleflags) -export-dynamic -avoid-version -module
pango_arabic_la_SOURCES = $(sources)

libpango_arabic_la_SOURCES = $(sources)

