## Process this file with automake to create Makefile.in.

aliasdir = $(sysconfdir)/pango

INCLUDES =				\
	-DG_DISABLE_DEPRECATED		\
	-DPANGO_DISABLE_DEPRECATED	\
	$(PANGO_DEBUG_FLAGS)		\
	-I$(top_srcdir)			\
	$(CAIRO_CFLAGS)			\
	$(GLIB_CFLAGS)			\
	$(X_CFLAGS)			\
	$(XFT_CFLAGS)			\
	$(FREETYPE_CFLAGS)

###################################################

CLEANFILES = pango.modules
###################################################

noinst_PROGRAMS =

###################################################
if HAVE_FREETYPE
noinst_PROGRAMS += pangoft2topgm
endif

pangoft2topgm_SOURCES =				\
	pangoft2topgm.c				\
	renderdemo.c				\
	renderdemo.h				\
	argcontext.c				\
	argcontext.h
pangoft2topgm_LDADD =					\
	../pango/libpango-$(PANGO_API_VERSION).la	\
	../pango/libpangoft2-$(PANGO_API_VERSION).la	\
	$(FREETYPE_LIBS)
###################################################

###################################################
if HAVE_XFT
noinst_PROGRAMS += pango-xftview
endif

pango_xftview_SOURCES =				\
	xftview.c				\
	renderdemo.c				\
	argcontext.c				\
	argcontext.h
pango_xftview_LDADD =					\
	../pango/libpango-$(PANGO_API_VERSION).la	\
	../pango/libpangoft2-$(PANGO_API_VERSION).la	\
	../pango/libpangoxft-$(PANGO_API_VERSION).la	\
	$(XFT_LIBS)					\
	$(X_LIBS)
###################################################

###################################################
if HAVE_CAIRO
if HAVE_X
noinst_PROGRAMS += pango-cairoview
endif
endif

pango_cairoview_SOURCES =			\
	cairoview.c				\
	renderdemo.c				\
	argcontext.c				\
	argcontext.h
pango_cairoview_LDADD =					\
	../pango/libpango-$(PANGO_API_VERSION).la	\
	../pango/libpangoft2-$(PANGO_API_VERSION).la	\
	../pango/libpangocairo-$(PANGO_API_VERSION).la	\
	 $(CAIRO_LIBS)					\
	 $(X_LIBS)
###################################################

###################################################
if HAVE_CAIRO
noinst_PROGRAMS += cairosimple
endif

cairosimple_SOURCES =					\
	cairosimple.c
cairosimple_LDADD =					\
	../pango/libpango-$(PANGO_API_VERSION).la	\
	../pango/libpangocairo-$(PANGO_API_VERSION).la	\
	 $(CAIRO_LIBS)
###################################################

if CROSS_COMPILING
else
all-local: pango.modules
endif

pango.modules: $(top_builddir)/pango/pango-querymodules
	if find ../modules -name '*.so' | grep 'so' > /dev/null ; then \
          echo "Writing a pango.modules file to use when running examples before installing Pango."; \
	  (cd ../modules && \
           $(top_builddir)/pango/pango-querymodules `find . -name '*.la' | grep -v \.libs` > ../examples/pango.modules ) ;\
	else \
          echo "No dynamic modules found; will use only static modules for uninstalled example programs."; \
	  touch pango.modules; \
	fi

EXTRA_DIST=			\
	viewer-win32.c		\
	pangoft2topgm.c         \
	makefile.msc		\
	HELLO.utf8		\
	GLASS.utf8		\
	dev-example.utf		\
	muru.utf		\
	syriac.utf		\
	tibetan.utf		\
	pangorc			\
	pangox.aliases

if HAVE_X
alias_DATA = pangox.aliases
endif
