## Process this file with automake to create Makefile.in.

sources =		 	\
	myanmar.c               \
	gurmukhi.c              \
	bengali.c               \
	gujarati.c              \
	devanagari.c		\
	pango-indic-script.h

if INCLUDE_MYANMAR
myanmar_noinst=libpango-myanmar.la
myanmar_cflags=-DMYANMAR_MODULE_PREFIX
else
myanmar_inst=pango-myanmar.la
myanmar_moduleflags=-rpath $(libdir)
endif

if INCLUDE_GURMUKHI
gurmukhi_noinst=libpango-gurmukhi.la
gurmukhi_cflags=-DGURMUKHI_MODULE_PREFIX
else
gurmukhi_inst=pango-gurmukhi.la
gurmukhi_moduleflags=-rpath $(libdir)
endif

if INCLUDE_BENGALI
bengali_noinst=libpango-bengali.la
bengali_cflags=-DBENGALI_MODULE_PREFIX
else
bengali_inst=pango-bengali.la
bengali_moduleflags=-rpath $(libdir)
endif

if INCLUDE_DEVANAGARI
devanagari_noinst=libpango-devanagari.la
devanagari_cflags=-DDEVANAGARI_MODULE_PREFIX
else
devanagari_inst=pango-devanagari.la
devanagari_moduleflags=-rpath $(libdir)
endif

if INCLUDE_GUJARATI
gujarati_noinst=libpango-gujarati.la
gujarati_cflags=-DGUJARATI_MODULE_PREFIX
else
gujarati_inst=pango-gujarati.la
gujarati_moduleflags=-rpath $(libdir)
endif

if INCLUDE_ANY_INDIC
noinst_LTLIBRARIES = $(myanmar_noinst) $(gurmukhi_noinst) $(bengali_noinst) $(devanagari_noinst) $(gujarati_noinst)
else
moduledir = $(libdir)/pango/modules
module_LTLIBRARIES = $(myanmar_inst) $(gurmukhi_inst) $(bengali_inst) $(devanagari_inst) $(gujarati_inst)
endif

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/pango/ $(myanmar_cflags) $(gurmukhi_cflags) $(bengali_cflags) $(devanagari_cflags) $(gujarati_cflags)

EXTRA_DIST = \
	pango-indic-script.h

pango_devanagari_la_LDFLAGS = $(devanagari_moduleflags) -export-dynamic -avoid-version -module
if INCLUDE_DEVANAGARI
libpango_devanagari_la_SOURCES = devanagari.c
else
pango_devanagari_la_SOURCES = devanagari.c
endif

pango_myanmar_la_LDFLAGS = $(myanmar_moduleflags) -export-dynamic -avoid-version -module
if INCLUDE_MYANMAR
libpango_myanmar_la_SOURCES = myanmar.c
else
pango_myanmar_la_SOURCES = myanmar.c
endif

pango_gurmukhi_la_LDFLAGS = $(gurmukhi_moduleflags) -export-dynamic -avoid-version -module
if INCLUDE_GURMUKHI
libpango_gurmukhi_la_SOURCES = gurmukhi.c
else
pango_gurmukhi_la_SOURCES = gurmukhi.c
endif

pango_bengali_la_LDFLAGS = $(bengali_moduleflags) -export-dynamic -avoid-version -module
if INCLUDE_BENGALI
libpango_bengali_la_SOURCES = bengali.c
else
pango_bengali_la_SOURCES = bengali.c
endif

pango_gujarati_la_LDFLAGS = $(gujarati_moduleflags) -export-dynamic -avoid-version -module
if INCLUDE_GUJARATI
libpango_gujarati_la_SOURCES = gujarati.c
else
pango_gujarati_la_SOURCES = gujarati.c
endif


