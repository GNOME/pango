## Process this file with automake to create Makefile.in.

pangolibs = $(top_builddir)/pango/libpango-$(PANGO_API_VERSION).la $(FRIBIDI_LIBS) $(GLIB_LIBS)
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


moduledir = $(libdir)/pango/$(PANGO_MODULE_VERSION)/modules
module_LTLIBRARIES =
noinst_LTLIBRARIES =

if HAVE_X
INCLUDES += $(X_CFLAGS)
if INCLUDE_MYANMAR_X
noinst_LTLIBRARIES += libpango-myanmar-x.la
else
module_LTLIBRARIES += pango-myanmar-x.la
endif

if INCLUDE_GURMUKHI_X
noinst_LTLIBRARIES += libpango-gurmukhi-x.la
else
module_LTLIBRARIES += pango-gurmukhi-x.la
endif

if INCLUDE_BENGALI_X
noinst_LTLIBRARIES += libpango-bengali-x.la
else
module_LTLIBRARIES += pango-bengali-x.la
endif

if INCLUDE_DEVANAGARI_X
noinst_LTLIBRARIES += libpango-devanagari-x.la
else
module_LTLIBRARIES += pango-devanagari-x.la
endif

if INCLUDE_GUJARATI_X
noinst_LTLIBRARIES += libpango-gujarati-x.la
else
module_LTLIBRARIES += pango-gujarati-x.la
endif
endif

pango_devanagari_x_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_devanagari_x_la_LIBADD = $(pangoxlibs)
pango_devanagari_x_la_SOURCES = devanagari-x.c pango-indic-script.h
libpango_devanagari_x_la_SOURCES = devanagari-x.c pango-indic-script.h
libpango_devanagari_x_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_devanagari_x

pango_myanmar_x_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_myanmar_x_la_LIBADD = $(pangoxlibs)
pango_myanmar_x_la_SOURCES = myanmar-x.c pango-indic-script.h
libpango_myanmar_x_la_SOURCES = myanmar-x.c pango-indic-script.h
libpango_myanmar_x_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_myanmar_x

pango_gurmukhi_x_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_gurmukhi_x_la_LIBADD = $(pangoxlibs)
pango_gurmukhi_x_la_SOURCES = gurmukhi-x.c pango-indic-script.h
libpango_gurmukhi_x_la_SOURCES = gurmukhi-x.c pango-indic-script.h
libpango_gurmukhi_x_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_gurmukhi_x

pango_bengali_x_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_bengali_x_la_LIBADD = $(pangoxlibs)
pango_bengali_x_la_SOURCES = bengali-x.c pango-indic-script.h
libpango_bengali_x_la_SOURCES = bengali-x.c pango-indic-script.h
libpango_bengali_x_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_bengali_x

pango_gujarati_x_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_gujarati_x_la_LIBADD = $(pangoxlibs)
pango_gujarati_x_la_SOURCES = gujarati-x.c pango-indic-script.h
libpango_gujarati_x_la_SOURCES = gujarati-x.c pango-indic-script.h
libpango_gujarati_x_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_gujarati_x


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
	indic-ot.h

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
	indic-ot.h

pango_indic_ft2_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_indic_ft2_la_LIBADD = $(pangoft2libs)
pango_indic_ft2_la_SOURCES = $(ft2_sources)
pango_indic_ft2_la_CFLAGS = -DBUILD_FT2
libpango_indic_ft2_la_SOURCES = $(ft2_sources)
libpango_indic_ft2_la_CFLAGS = -DBUILD_FT2 -DPANGO_MODULE_PREFIX=_pango_indic_ft2


included-modules: $(noinst_LTLIBRARIES)

.PHONY: included-modules

