## Process this file with automake to create Makefile.in.

noinst_LTLIBRARIES = pango-arabic.la

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/pango/ 

pango_arabic_la_SOURCES = \
	arabic.c \
	arconv.c \
	arconv.h 

pango_arabic_la_LDFLAGS = -rpath $(libdir) -export-dynamic -avoid-version -module

EXTRA_DIST= 
