## Process this file with automake to create Makefile.in.

pangolibs = $(top_builddir)/pango/libpango-$(PANGO_API_VERSION).la $(GLIB_LIBS)
pangoxlibs = $(top_builddir)/pango/libpangox-$(PANGO_API_VERSION).la $(X_LIBS) $(pangolibs)
pangoxftlibs = $(top_builddir)/pango/libpangoxft-$(PANGO_API_VERSION).la $(XFT_LIBS) $(pangolibs)
pangoft2libs = $(top_builddir)/pango/libpangoft2-$(PANGO_API_VERSION).la $(FREETYPE_LIBS) $(pangolibs)

INCLUDES =					\
	-DPANGO_ENABLE_ENGINE			\
	-DG_DISABLE_DEPRECATED			\
	$(PANGO_DEBUG_FLAGS)			\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/pango/			\
	$(GLIB_CFLAGS)

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

moduledir = $(libdir)/pango/$(PANGO_MODULE_VERSION)/modules
module_LTLIBRARIES =
noinst_LTLIBRARIES =


if HAVE_XFT
INCLUDES += $(XFT_CFLAGS)
if INCLUDE_INDIC_XFT
noinst_LTLIBRARIES += libpango-indic-xft.la
else
module_LTLIBRARIES += pango-indic-xft.la
endif
endif

xft_sources =		 	\
	indic-fc.c		\
	indic-ot-class-tables.c	\
	indic-ot.c		\
	indic-ot.h		\
	mprefixups.c		\
	mprefixups.h

pango_indic_xft_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_indic_xft_la_LIBADD = $(pangoxftlibs)
pango_indic_xft_la_SOURCES = $(xft_sources)
pango_indic_xft_la_CFLAGS = -DBUILD_XFT
libpango_indic_xft_la_SOURCES = $(xft_sources)
libpango_indic_xft_la_CFLAGS = -DBUILD_FT2 -DPANGO_MODULE_PREFIX=_pango_indic_xft


if HAVE_FREETYPE
INCLUDES += $(FREETYPE_CFLAGS)
if INCLUDE_INDIC_FT2
noinst_LTLIBRARIES += libpango-indic-ft2.la
else
module_LTLIBRARIES += pango-indic-ft2.la
endif
endif

ft2_sources =		 	\
	indic-fc.c		\
	indic-ot-class-tables.c	\
	indic-ot.c		\
	indic-ot.h		\
	mprefixups.c		\
	mprefixups.h

pango_indic_ft2_la_LDFLAGS = -export-dynamic -avoid-version -module $(no_undefined)
pango_indic_ft2_la_LIBADD = $(pangoft2libs)
pango_indic_ft2_la_SOURCES = $(ft2_sources)
pango_indic_ft2_la_CFLAGS = -DBUILD_FT2
libpango_indic_ft2_la_SOURCES = $(ft2_sources)
libpango_indic_ft2_la_CFLAGS = -DBUILD_FT2 -DPANGO_MODULE_PREFIX=_pango_indic_ft2


included-modules: $(noinst_LTLIBRARIES)

.PHONY: included-modules

