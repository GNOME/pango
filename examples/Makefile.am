## Process this file with automake to create Makefile.in.

aliasdir = $(sysconfdir)/pango

###################################################
if BUILD_QT_TEST

QT_INCLUDES=-I $(QTDIR)/include
QT_LIBS=-L$(QTDIR)/lib -lqt 
MOC=$(QTDIR)/bin/moc

INCLUDES = -DG_DISABLE_DEPRECATED -I$(top_srcdir) $(QT_INCLUDES) $(GLIB_CFLAGS)

bin_PROGRAMS = pango-viewer

pango_viewer_SOURCES =		\
	viewer-qt.cc		\
	viewer-qt.h		\
	moc_viewer-qt.cc

moc_viewer-qt.cc: viewer-qt.h
	$(MOC) $< -o $@

BUILT_SOURCES=moc_viewer-qt.cc

pango_viewer_LDADD = ../pango/libpangox.la ../pango/libpango.la $(FRIBIDI_LIBS) $(QT_LIBS)

endif
###################################################

all-local: pango.modules

pango.modules:
	if find ../modules -name '*.so' | grep 'so' > /dev/null ; then \
          echo "Writing a pango.modules file to use when running examples before installing Pango."; \
	  (cd ../modules && \
           ../pango/pango-querymodules `find . -name '*.so'` > ../examples/pango.modules ) ;\
	else \
          echo "No dynamic modules found; will use only static modules for uninstalled example programs."; \
	  touch pango.modules; \
	fi

EXTRA_DIST=			\
	moc_viewer-qt.cc	\
	viewer-qt.cc		\
	viewer-qt.h		\
	viewer-win32.c		\
	viewer-ft2.c		\
	pangoft2topgm.c         \
	makefile.mingw		\
	makefile.mingw.in	\
	HELLO.utf8		\
	dev-example.utf		\
	muru.utf		\
	pangorc			\
	pangox.aliases		\
	pangoft2.aliases	\
	pangowin32.aliases

if HAVE_X
alias_DATA = pangox.aliases
endif
