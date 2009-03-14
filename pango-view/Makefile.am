## Process this file with automake to create Makefile.in.

TEST_TEXTS =			\
	test-arabic.txt		\
	test-chinese.txt	\
	test-devanagari.txt	\
	test-gurmukhi.txt	\
	test-hebrew.txt		\
	test-ipa.txt		\
	test-justify.txt	\
	test-lao.txt		\
	test-latin.txt		\
	test-long-paragraph.txt	\
	test-mixed.markup	\
	test-mixed.txt		\
	test-nko.txt		\
	test-opentype-language.markup \
	test-syriac.txt		\
	test-tamil.txt		\
	test-thai.txt		\
	test-tibetan.txt	\
	HELLO.txt		\
	GLASS.txt

EXTRA_DIST =			\
	$(TEST_TEXTS)

CLEANFILES = pangorc

INCLUDES =				\
	-I$(top_srcdir)			\
	$(PANGO_DEBUG_FLAGS)		\
	$(GLIB_CFLAGS)			\
	$(XFT_CFLAGS)			\
	$(CAIRO_CFLAGS)			\
	$(FREETYPE_CFLAGS)		\
	$(X_CFLAGS)

bin_PROGRAMS =

#########################################################
if HAVE_X
bin_PROGRAMS += pango-view
pango_view_SOURCES =					\
	viewer-render.h					\
	viewer-render.c					\
	viewer.h					\
	viewer-x.h					\
	viewer-x.c					\
	viewer-main.c					\
	pango-view.c
pango_view_LDADD =					\
	../pango/libpango-$(PANGO_API_VERSION).la	\
	$(GLIB_LIBS)
if HAVE_X
pango_view_SOURCES +=					\
	viewer-pangox.c
pango_view_LDADD +=					\
	../pango/libpangox-$(PANGO_API_VERSION).la
endif
if HAVE_FREETYPE
pango_view_SOURCES +=					\
	viewer-pangoft2.c
pango_view_LDADD +=					\
	../pango/libpangoft2-$(PANGO_API_VERSION).la	\
	$(FREETYPE_LIBS)
endif
if HAVE_XFT
pango_view_SOURCES +=					\
	viewer-pangoxft.c
pango_view_LDADD +=					\
	../pango/libpangoft2-$(PANGO_API_VERSION).la	\
	../pango/libpangoxft-$(PANGO_API_VERSION).la	\
	$(XFT_LIBS)
endif
if HAVE_CAIRO
if HAVE_CAIRO_XLIB
pango_view_SOURCES +=					\
	viewer-cairo.h					\
	viewer-cairo.c					\
	viewer-pangocairo.c
pango_view_LDADD +=					\
	../pango/libpangoft2-$(PANGO_API_VERSION).la	\
	../pango/libpangocairo-$(PANGO_API_VERSION).la	\
	$(CAIRO_LIBS)
endif
endif
pango_view_LDADD +=					\
	$(X_LIBS)
endif
#########################################################

BUILT_SOURCES = pangorc

pangorc: $(srcdir)/../modules/pangorc
	cp $(srcdir)/../modules/pangorc $@
