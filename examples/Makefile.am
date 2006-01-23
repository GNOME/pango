## Process this file with automake to create Makefile.in.

EXTRA_DIST=			\
	viewer-win32.c		\
	makefile.msc		\
	HELLO.utf8		\
	GLASS.utf8		\
	dev-example.utf		\
	muru.utf		\
	syriac.utf		\
	tibetan.utf

CLEANFILES = pangorc

INCLUDES =				\
	-DG_DISABLE_DEPRECATED		\
	-DPANGO_DISABLE_DEPRECATED	\
	-I$(top_srcdir)			\
	$(PANGO_DEBUG_FLAGS)		\
	$(GLIB_CFLAGS)			\
	$(XFT_CFLAGS)			\
	$(CAIRO_CFLAGS)			\
	$(FREETYPE_CFLAGS)		\
	$(X_CFLAGS)

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
	$(GLIB_LIBS)					\
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
	$(GLIB_LIBS)					\
	$(XFT_LIBS)					\
	$(X_LIBS)
###################################################

###################################################
if HAVE_CAIRO
if HAVE_X
if HAVE_FREETYPE
noinst_PROGRAMS += pango-cairoview
endif
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
	$(GLIB_LIBS)					\
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
	$(GLIB_LIBS)					\
	$(CAIRO_LIBS)
###################################################

pangorc: $(srcdir)/../modules/pangorc
	cp $< $@

$(noinst_PROGRAMS): pangorc
