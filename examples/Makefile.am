## Process this file with automake to create Makefile.in.

aliasdir = $(sysconfdir)/pango

INCLUDES = -DG_DISABLE_DEPRECATED -I$(top_srcdir) $(QT_INCLUDES) $(GLIB_CFLAGS) $(FONTCONFIG_CFLAGS) $(FREETYPE_CFLAGS)

###################################################

if BUILD_QT_TEST

QT_INCLUDES=-I$(QTDIR)/include
QT_LIBS=-L$(QTDIR)/lib -lqt 
MOC=$(QTDIR)/bin/moc

viewer_programs = pango-viewer

BUILT_SOURCES=moc_viewer-qt.cc

moc_viewer-qt.cc: viewer-qt.h
	$(MOC) $< -o $@

endif

pango_viewer_SOURCES =		\
	viewer-qt.cc		\
	viewer-qt.h

CLEANFILES = moc_viewer-qt.cc
pango_viewer_LDADD =					\
	../pango/libpangox-$(PANGO_API_VERSION).la	\
	../pango/libpango-$(PANGO_API_VERSION).la	\
	$(FRIBIDI_LIBS)					\
	$(QT_LIBS)					\
	moc_viewer-qt.$(OBJEXT)
###################################################

###################################################
if HAVE_FREETYPE
ft2_programs = pangoft2topgm
endif

pangoft2topgm_SOURCES = pangoft2topgm.c
pangoft2topgm_LDADD =					\
	../pango/libpango-$(PANGO_API_VERSION).la	\
	../pango/libpangoft2-$(PANGO_API_VERSION).la	\
	$(FREETYPE_LIBS)
###################################################

bin_PROGRAMS = $(viewer_programs)
noinst_PROGRAMS =  $(ft2_programs)

if CROSS_COMPILING
else
all-local: pango.modules
endif

pango.modules:
	if find ../modules -name '*.so' | grep 'so' > /dev/null ; then \
          echo "Writing a pango.modules file to use when running examples before installing Pango."; \
	  (cd ../modules && \
           ../pango/pango-querymodules `find . -name '*.la' | grep -v \.libs` > ../examples/pango.modules ) ;\
	else \
          echo "No dynamic modules found; will use only static modules for uninstalled example programs."; \
	  touch pango.modules; \
	fi

EXTRA_DIST=			\
	viewer-qt.cc		\
	viewer-qt.h		\
	viewer-win32.c		\
	pangoft2topgm.c         \
	makefile.msc		\
	HELLO.utf8		\
	dev-example.utf		\
	muru.utf		\
	pangorc			\
	pangox.aliases

if HAVE_X
alias_DATA = pangox.aliases
endif
