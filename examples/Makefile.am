## Process this file with automake to create Makefile.in.

EXTRA_DIST =			\
	pangowin32tobmp.c

INCLUDES =				\
	-I$(top_srcdir)			\
	$(PANGO_DEBUG_FLAGS)		\
	$(GLIB_CFLAGS)			\
	$(XFT_CFLAGS)			\
	$(CAIRO_CFLAGS)			\
	$(FREETYPE_CFLAGS)		\
	$(X_CFLAGS)

noinst_PROGRAMS =

if HAVE_CAIRO_PNG
noinst_PROGRAMS += cairosimple cairotwisted cairoshape

cairosimple_LDADD =					\
	../pango/libpango-$(PANGO_API_VERSION).la	\
	../pango/libpangocairo-$(PANGO_API_VERSION).la	\
	$(GLIB_LIBS)					\
	$(CAIRO_LIBS)
if HAVE_FREETYPE
cairosimple_LDADD +=					\
	../pango/libpangoft2-$(PANGO_API_VERSION).la
endif
cairotwisted_LDADD = $(cairosimple_LDADD)
cairoshape_LDADD   = $(cairosimple_LDADD)
endif
