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

#########################################################

bin_PROGRAMS = pango-view
pango_view_SOURCES =					\
	viewer-render.h					\
	viewer-render.c					\
	viewer.h					\
	viewer-main.c					\
	pango-view.c
pango_view_LDADD =					\
	../pango/libpango-$(PANGO_API_VERSION).la	\
	$(GLIB_LIBS)
if HAVE_X
pango_view_SOURCES +=					\
	viewer-x.h					\
	viewer-x.c					\
	viewer-pangox.c
pango_view_LDADD +=					\
	../pango/libpangox-$(PANGO_API_VERSION).la	\
	$(X_LIBS)
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
pango_view_SOURCES +=					\
	viewer-cairo.h					\
	viewer-cairo.c					\
	viewer-pangocairo.c
pango_view_LDADD +=					\
	../pango/libpangocairo-$(PANGO_API_VERSION).la	\
	$(CAIRO_LIBS)
endif

#########################################################

MAINTAINERCLEANFILES = pango-view.1.in
EXTRA_DIST += pango-view.1.in
CLEANFILES += pango-view.1

nodist_man_MANS = pango-view.1

# The indirection through pango-view.1.in is to make parallel build work.
# See bug 587768.
$(srcdir)/pango-view.1.in: ../configure.in $(pango_view_SOURCES)
	$(AM_V_GEN) $(top_srcdir)/missing --run \
	   help2man --no-info --section=1 \
		    --help-option="--help-all" --output="$@.tmp" \
		    --name 'Pango text viewer' ./pango-view \
	 && mv "$@.tmp" "$@" \
	  || (echo Failed to update pango-view.1, the man page may be outdated >&2; \
	      (test -f "$@" || echo help2man is required to generate this file. >> "$@"));
pango-view.1: pango-view$(EXEEXT)
	$(AM_V_GEN) $(MAKE) $(AM_MAKEFLAGS) pango-view.1.in && \
	cp $(srcdir)/pango-view.1.in $@

#########################################################

BUILT_SOURCES = pangorc

pangorc: $(srcdir)/../modules/pangorc
	cp $(srcdir)/../modules/pangorc $@

-include $(top_srcdir)/git.mk
