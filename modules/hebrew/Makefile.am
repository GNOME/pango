## Process this file with automake to create Makefile.in.

common_sources = hebrew-shaper.c hebrew-shaper.h
hebrew_x_sources = $(common_sources) hebrew-x.c 
hebrew_xft_sources = $(common_sources) hebrew-xft.c 
hebrew_ft2_sources = $(common_sources) hebrew-ft2.c 

pangolibs = $(top_builddir)/pango/libpango-$(PANGO_API_VERSION).la $(FRIBIDI_LIBS) $(GLIB_LIBS)
pangoxlibs = $(top_builddir)/pango/libpangox-$(PANGO_API_VERSION).la $(X_LIBS) $(pangolibs)
pangoxftlibs = $(top_builddir)/pango/libpangoxft-$(PANGO_API_VERSION).la $(XFT_LIBS) $(pangolibs)
pangoft2libs = $(top_builddir)/pango/libpangoft2-$(PANGO_API_VERSION).la $(FREETYPE_LIBS) $(pangolibs)

if HAVE_XFT
if INCLUDE_BASIC_XFT
XFT_INCLUDED=libpango-hebrew-xft.la
XFT_MODULES=
XFT_PREFIX=-DXFT_MODULE_PREFIX
else
XFT_INCLUDED=
XFT_MODULES=pango-hebrew-xft.la
XFT_PREFIX=
hebrew_xft_libadd=$(pangoxlibs) $(pangoxftlibs)
endif
else
XFT_MODULES=
XFT_INCLUDED=
XFT_PREFIX=
endif

if HAVE_X
if INCLUDE_BASIC_X
X_INCLUDED=libpango-hebrew-x.la
X_MODULES=
X_PREFIX=-DX_MODULE_PREFIX
else
X_INCLUDED=
X_MODULES=pango-hebrew-x.la
X_PREFIX=
hebrew_x_libadd=$(pangoxlibs)
endif
else
X_INCLUDED=
X_MODULES=
X_PREFIX=
endif

if HAVE_FREETYPE
if INCLUDE_BASIC_FT2
FT2_INCLUDED=libpango-hebrew-ft2.la
FT2_MODULES=
FT2_PREFIX=-DFT2_MODULE_PREFIX
else
FT2_INCLUDED=
FT2_MODULES=pango-hebrew-ft2.la
FT2_PREFIX=
hebrew_ft2_libadd=$(pangoft2libs)
endif
else
FT2_MODULES=
FT2_INCLUDED=
FT2_PREFIX=
endif

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

noinst_LTLIBRARIES = $(X_INCLUDED) $(FT2_INCLUDED) $(XFT_INCLUDED)
module_LTLIBRARIES = $(X_MODULES) $(FT2_MODULES) $(XFT_MODULES)
moddefine = $(X_PREFIX) $(FT2_PREFIX) $(XFT_PREFIX)
moduledir = $(libdir)/pango/$(PANGO_MODULE_VERSION)/modules

INCLUDES = 			\
	-DPANGO_ENABLE_ENGINE	\
	-DG_DISABLE_DEPRECATED	\
	-I$(top_srcdir) 	\
	-I$(top_srcdir)/pango/ 	\
	$(X_CFLAGS) 		\
	$(FREETYPE_CFLAGS)	\
	$(moddefine)

pango_hebrew_x_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_hebrew_x_la_LIBADD = $(hebrew_x_libadd)
pango_hebrew_x_la_SOURCES = $(hebrew_x_sources)
libpango_hebrew_x_la_SOURCES = $(hebrew_x_sources)

pango_hebrew_xft_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_hebrew_xft_la_LIBADD = $(hebrew_xft_libadd)
pango_hebrew_xft_la_SOURCES = $(hebrew_xft_sources)
libpango_hebrew_xft_la_SOURCES = $(hebrew_xft_sources)

pango_hebrew_ft2_la_LDFLAGS = -export-dynamic -avoid-version -module $(no_undefined)
pango_hebrew_ft2_la_LIBADD = $(hebrew_ft2_libadd)
pango_hebrew_ft2_la_SOURCES = $(hebrew_ft2_sources)
libpango_hebrew_ft2_la_SOURCES = $(hebrew_ft2_sources)


included-modules: $(noinst_LTLIBRARIES)

.PHONY: included-modules
