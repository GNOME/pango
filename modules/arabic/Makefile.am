## Process this file with automake to create Makefile.in.

pangolibs = $(top_builddir)/pango/libpango-$(PANGO_API_VERSION).la $(FRIBIDI_LIBS) $(GLIB_LIBS)
pangoxlibs = $(top_builddir)/pango/libpangox-$(PANGO_API_VERSION).la $(X_LIBS) $(pangolibs)
pangoxftlibs = $(top_builddir)/pango/libpangoxft-$(PANGO_API_VERSION).la $(XFT_LIBS) $(pangolibs)
pangoft2libs = $(top_builddir)/pango/libpangoft2-$(PANGO_API_VERSION).la $(FREETYPE_LIBS) $(pangolibs)

INCLUDES = 				\
	-DPANGO_ENABLE_ENGINE		\
	-DG_DISABLE_DEPRECATED		\
	$(PANGO_DEBUG_FLAGS)		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/pango/		\
	$(GLIB_CFLAGS)

moduledir = $(libdir)/pango/$(PANGO_MODULE_VERSION)/modules
module_LTLIBRARIES =
noinst_LTLIBRARIES =

if PLATFORM_WIN32
no_undefined = -no-undefined
endif


if HAVE_X
INCLUDES += $(X_CFLAGS)
if INCLUDE_ARABIC_X
noinst_LTLIBRARIES += libpango-arabic-x.la
INCLUDES += -DX_MODULE_PREFIX
else
module_LTLIBRARIES += pango-arabic-x.la
endif
endif

x_sources = \
	arabic-x.c \
	arconv.c \
	mulefont.c \
	mulefont.h \
	langboxfont.c \
	langboxfont.h \
	naqshfont.c \
	naqshfont.h \
	arconv.h 

pango_arabic_x_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_arabic_x_la_LIBADD = $(pangoxlibs)
pango_arabic_x_la_SOURCES = $(x_sources)
libpango_arabic_x_la_SOURCES = $(x_sources)


if HAVE_XFT
INCLUDES += $(XFT_CFLAGS)
if INCLUDE_ARABIC_XFT
noinst_LTLIBRARIES += libpango-arabic-xft.la
INCLUDES += -DXFT_MODULE_PREFIX
else
module_LTLIBRARIES += pango-arabic-xft.la
endif
endif

xft_sources = \
	arabic-xft.c \
	arabic-ot.c \
	arabic-ot.h

pango_arabic_xft_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_arabic_xft_la_LIBADD = $(pangoxftlibs)
pango_arabic_xft_la_SOURCES = $(xft_sources)
libpango_arabic_xft_la_SOURCES = $(xft_sources)


if HAVE_FREETYPE
INCLUDES += $(FREETYPE_CFLAGS)
if INCLUDE_ARABIC_FT2
noinst_LTLIBRARIES += libpango-arabic-ft2.la
INCLUDES += -DFT2_MODULE_PREFIX
else
module_LTLIBRARIES += pango-arabic-ft2.la
endif
endif

ft2_sources = \
	arabic-ft2.c \
	arabic-ot.c \
	arabic-ot.h

pango_arabic_ft2_la_LDFLAGS = -export-dynamic -avoid-version -module $(no_undefined)
pango_arabic_ft2_la_LIBADD = $(pangoft2libs)
pango_arabic_ft2_la_SOURCES = $(ft2_sources)
libpango_arabic_ft2_la_SOURCES = $(ft2_sources)


included-modules: $(noinst_LTLIBRARIES)

.PHONY: included-modules
