## Process this file with automake to create Makefile.in.

moduledir = $(libdir)/pango/modules
module_LTLIBRARIES = pango-devanagari.la

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/pango/ 

pango_devanagari_la_SOURCES = 	\
	devanagari.c		\
	dev-ligatures.h

pango_devanagari_la_LDFLAGS = -rpath $(libdir) -export-dynamic -avoid-version -module

EXTRA_DIST= 
