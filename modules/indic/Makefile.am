## Process this file with automake to create Makefile.in.

sources =		 	\
	myanmar-x.c             \
	gurmukhi-x.c            \
	bengali-x.c             \
	gujarati-x.c            \
	devanagari-x.c		\
	pango-indic-script.h

if INCLUDE_MYANMAR_X
myanmar_x_noinst=libpango-myanmar-x.la
myanmar_x_cflags=-DMYANMAR_X_MODULE_PREFIX
else
myanmar_x_inst=pango-myanmar-x.la
myanmar_x_moduleflags=-rpath $(libdir)
endif

if INCLUDE_GURMUKHI_X
gurmukhi_x_noinst=libpango-gurmukhi-x.la
gurmukhi_x_cflags=-DGURMUKHI_X_MODULE_PREFIX
else
gurmukhi_x_inst=pango-gurmukhi-x.la
gurmukhi_x_moduleflags=-rpath $(libdir)
endif

if INCLUDE_BENGALI_X
bengali_x_noinst=libpango-bengali-x.la
bengali_x_cflags=-DBENGALI_X_MODULE_PREFIX
else
bengali_x_inst=pango-bengali-x.la
bengali_x_moduleflags=-rpath $(libdir)
endif

if INCLUDE_DEVANAGARI_X
devanagari_x_noinst=libpango-devanagari-x.la
devanagari_x_cflags=-DDEVANAGARI_X_MODULE_PREFIX
else
devanagari_x_inst=pango-devanagari-x.la
devanagari_x_moduleflags=-rpath $(libdir)
endif

if INCLUDE_GUJARATI_X
gujarati_x_noinst=libpango-gujarati-x.la
gujarati_x_cflags=-DGUJARATI_X_MODULE_PREFIX
else
gujarati_x_inst=pango-gujarati-x.la
gujarati_x_moduleflags=-rpath $(libdir)
endif

if INCLUDE_ANY_INDIC
noinst_LTLIBRARIES = $(myanmar_x_noinst) $(gurmukhi_x_noinst) $(bengali_x_noinst) $(devanagari_x_noinst) $(gujarati_x_noinst)
else
moduledir = $(libdir)/pango/modules
module_LTLIBRARIES = $(myanmar_x_inst) $(gurmukhi_x_inst) $(bengali_x_inst) $(devanagari_x_inst) $(gujarati_x_inst)
endif

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/pango/ $(X_CFLAGS) $(myanmar_x_cflags) $(gurmukhi_x_cflags) $(bengali_x_cflags) $(devanagari_x_cflags) $(gujarati_x_cflags)

EXTRA_DIST = \
	pango-indic-script.h

pango_devanagari_x_la_LDFLAGS = $(devanagari_x_moduleflags) -export-dynamic -avoid-version -module
if INCLUDE_DEVANAGARI_X
libpango_devanagari_x_la_SOURCES = devanagari-x.c
else
pango_devanagari_x_la_SOURCES = devanagari-x.c
endif

pango_myanmar_x_la_LDFLAGS = $(myanmar_x_moduleflags) -export-dynamic -avoid-version -module
if INCLUDE_MYANMAR_X
libpango_myanmar_x_la_SOURCES = myanmar-x.c
else
pango_myanmar_x_la_SOURCES = myanmar-x.c
endif

pango_gurmukhi_x_la_LDFLAGS = $(gurmukhi_x_moduleflags) -export-dynamic -avoid-version -module
if INCLUDE_GURMUKHI_X
libpango_gurmukhi_x_la_SOURCES = gurmukhi-x.c
else
pango_gurmukhi_x_la_SOURCES = gurmukhi-x.c
endif

pango_bengali_x_la_LDFLAGS = $(bengali_x_moduleflags) -export-dynamic -avoid-version -module
if INCLUDE_BENGALI_X
libpango_bengali_x_la_SOURCES = bengali-x.c
else
pango_bengali_x_la_SOURCES = bengali-x.c
endif

pango_gujarati_x_la_LDFLAGS = $(gujarati_x_moduleflags) -export-dynamic -avoid-version -module
if INCLUDE_GUJARATI_X
libpango_gujarati_x_la_SOURCES = gujarati-x.c
else
pango_gujarati_x_la_SOURCES = gujarati-x.c
endif


