## Process this file with automake to create Makefile.in.

aliasdir = $(sysconfdir)/pango

###################################################
if BUILD_QT_TEST

QT_INCLUDES=-I $(QTDIR)/include
QT_LIBS=-L $(QTDIR)/lib -lqt 
MOC=$(QTDIR)/bin/moc

INCLUDES = -I$(top_srcdir) $(QT_INCLUDES) $(GLIB_CFLAGS)

bin_PROGRAMS = pango-viewer

pango_viewer_SOURCES =		\
	viewer-qt.cc		\
	viewer-qt.h		\
	moc_viewer-qt.cc

moc_viewer-qt.cc: viewer-qt.h
	$(MOC) $< -o $@

BUILT_SOURCES=moc_viewer-qt.cc

pango_viewer_LDADD = ../pango/libpango.la ../pango/libpangox.la $(FRIBIDI_LIBS) $(UNICODE_LIBS) $(QT_LIBS)

endif
###################################################

all-local: pango.modules

pango.modules:
	( cd ../modules && \
          ../pango/pango-querymodules `find . -name '*.so'` > ../examples/pango.modules )
          
EXTRA_DIST=HELLO.utf8 muru.utf pangox_aliases

alias_DATA = pangox_aliases

