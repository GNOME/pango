## Process this file with automake to create Makefile.in.

moduledir = $(libdir)/pango/modules
module_LTLIBRARIES = pango-arabic.la

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/pango/ 

pango_arabic_la_SOURCES = \
	arabic.c \
	arconv.c \
	mulefont.c \
	mulefont.h \
	arconv.h 

pango_arabic_la_LDFLAGS = -rpath $(libdir) -export-dynamic -avoid-version -module

EXTRA_DIST= 
