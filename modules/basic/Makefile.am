## Process this file with automake to create Makefile.in.

sources = basic.c

if INCLUDE_BASIC
noinst_LTLIBRARIES = libpango-basic.la
moddefine = -DMODULE_PREFIX

else
moduledir = $(libdir)/pango/modules
module_LTLIBRARIES = pango-basic.la
moduleflags=-rpath $(libdir)
endif

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/pango/ $(moddefine)

pango_basic_la_LDFLAGS = $(moduleflags) -export-dynamic -avoid-version -module
pango_basic_la_LIBADD = 
pango_basic_la_SOURCES = $(sources)

libpango_basic_la_SOURCES = $(sources)

EXTRA_DIST =		\
	tables-big.i	\
	tables-small.i	\
	basic-win32.c	\
	basic-ft2.c	\
	makefile.mingw	\
	makefile.mingw.in

makefile.mingw: $(top_builddir)/config.status $(top_srcdir)/modules/basic/makefile.mingw.in
	cd $(top_builddir) && CONFIG_FILES=modules/basic/$@ CONFIG_HEADERS= $(SHELL) ./config.status
