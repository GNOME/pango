## Process this file with automake to create Makefile.in.

aliasdir = $(sysconfdir)/pango

INCLUDES = -I$(top_srcdir)

if BUILD_TESTS
bin_PROGRAMS = pango-viewer

pango_viewer_SOURCES = \
	viewer.c
pango_viewer_LDADD = ../pango/libpango.la ../pango/libpangox.la $(GTK_LIBS) -lfribidi $(UNICODE_LIBS)
endif

all-local: pango.modules

pango.modules:
	( cd ../modules && \
          ../pango/pango-querymodules `find . -name '*.so'` > ../examples/pango.modules )
          
EXTRA_DIST=HELLO.utf8 muru.utf pangox_aliases

alias_DATA = pangox_aliases

