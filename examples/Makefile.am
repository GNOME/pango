## Process this file with automake to create Makefile.in.

noinst_PROGRAMS = pango-viewer

INCLUDES = -I$(top_srcdir)/libpango/

pango_viewer_SOURCES = \
	viewer.c
pango_viewer_LDADD = ../libpango/libpango.la ../libpango/libpangox.la $(GTK_LIBS) -lfribidi $(UNICODE_LIBS)

all-local: pango.modules

pango.modules:
	( cd ../modules && \
          ../libpango/pango-querymodules `find . -name '*.so'` > ../examples/pango.modules )
          
EXTRA_DIST=HELLO.utf8 muru.utf