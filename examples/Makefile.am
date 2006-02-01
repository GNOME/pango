## Process this file with automake to create Makefile.in.

TEST_TEXTS =			\
	test-arabic.txt	\
	test-devanagari.txt	\
	test-ipa.txt		\
	test-syriac.txt	\
	test-tamil.txt		\
	test-tibetan.txt	\
	HELLO.utf8		\
	GLASS.utf8

EXTRA_DIST =			\
	viewer-win32.c		\
	makefile.msc		\
	$(TEST_TEXTS)

CLEANFILES = pangorc

INCLUDES =				\
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
noinst_PROGRAMS += pangoft2-topgm
endif

pangoft2_topgm_SOURCES =			\
	pangoft2topgm.c				\
	renderdemo.c				\
	renderdemo.h				
pangoft2_topgm_LDADD =					\
	../pango/libpango-$(PANGO_API_VERSION).la	\
	../pango/libpangoft2-$(PANGO_API_VERSION).la	\
	$(GLIB_LIBS)					\
	$(FREETYPE_LIBS)
###################################################

###################################################
if HAVE_XFT
noinst_PROGRAMS += pangoxft-view
endif

pangoxft_view_SOURCES =				\
	xftview.c				\
	renderdemo.c				
pangoxft_view_LDADD =					\
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
noinst_PROGRAMS += pangocairo-view
endif
endif
endif

pangocairo_view_SOURCES =			\
	cairoview.c				\
	renderdemo.c				
pangocairo_view_LDADD =					\
	../pango/libpango-$(PANGO_API_VERSION).la	\
	../pango/libpangoft2-$(PANGO_API_VERSION).la	\
	../pango/libpangocairo-$(PANGO_API_VERSION).la	\
	$(GLIB_LIBS)					\
	$(CAIRO_LIBS)					\
	$(X_LIBS)
###################################################

###################################################
if HAVE_CAIRO_PNG
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
