## Process this file with automake to create Makefile.in.

sources =		 	\
	myanmar.c               \
	gurmukhi.c              \
	bengali.c               \
	gujarati.c              \
	devanagari.c		\
	pango-indic-script.h

if INCLUDE_INDIC
noinst_LTLIBRARIES = libpango-myanmar.la libpango-gurmukhi.la \
                     libpango-bengali.la \
		     libpango-devanagari.la \
		     libpango-gujarati.la
moddefine = -DMODULE_PREFIX
else
moduledir = $(libdir)/pango/modules
module_LTLIBRARIES = pango-myanmar.la pango-gurmukhi.la \
                     pango-bengali.la \
		     pango-devanagari.la \
		     pango-gujarati.la
endif

INCLUDES = -I$(top_srcdir) -I$(top_srcdir)/pango/ $(moddefine)


EXTRA_DIST = \
	pango-indic-script.h

pango_devanagari_la_LDFLAGS = -rpath $(libdir) -export-dynamic -avoid-version -module
pango_devanagari_la_SOURCES = devanagari.c

libpango_devanagari_la_SOURCES = devanagari.c


pango_myanmar_la_LDFLAGS = -rpath $(libdir) -export-dynamic -avoid-version -module
pango_myanmar_la_SOURCES = myanmar.c

libpango_myanmar_la_SOURCES = myanmar.c


pango_gurmukhi_la_LDFLAGS = -rpath $(libdir) -export-dynamic -avoid-version -module
pango_gurmukhi_la_SOURCES = gurmukhi.c

libpango_gurmukhi_la_SOURCES = gurmukhi.c


pango_bengali_la_LDFLAGS = -rpath $(libdir) -export-dynamic -avoid-version -module
pango_bengali_la_SOURCES = bengali.c

libpango_bengali_la_SOURCES = bengali.c


pango_gujarati_la_LDFLAGS = -rpath $(libdir) -export-dynamic -avoid-version -module
pango_gujarati_la_SOURCES = gujarati.c

libpango_gujarati_la_SOURCES = gujarati.c

