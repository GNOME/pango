## Process this file with automake to create Makefile.in.

pangolibs = $(top_builddir)/pango/libpango-$(PANGO_API_VERSION).la $(GLIB_LIBS)
pangoft2libs = $(top_builddir)/pango/libpangoft2-$(PANGO_API_VERSION).la $(FREETYPE_LIBS) $(pangolibs)

INCLUDES =					\
	-DPANGO_ENABLE_ENGINE			\
	-DG_DISABLE_DEPRECATED			\
	$(PANGO_DEBUG_FLAGS)			\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/pango			\
	$(GLIB_CFLAGS)

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

moduledir = $(libdir)/pango/$(PANGO_MODULE_VERSION)/modules
module_LTLIBRARIES =
noinst_LTLIBRARIES =


if HAVE_FREETYPE
INCLUDES += $(FREETYPE_CFLAGS)
if INCLUDE_INDIC_FC
noinst_LTLIBRARIES += libpango-indic-fc.la
else
module_LTLIBRARIES += pango-indic-fc.la
endif
endif

fc_sources =		 	\
	indic-fc.c		\
	indic-ot-class-tables.c	\
	indic-ot.c		\
	indic-ot.h		\
	mprefixups.c		\
	mprefixups.h

pango_indic_fc_la_LDFLAGS = -export-dynamic -avoid-version -module $(no_undefined)
pango_indic_fc_la_LIBADD = $(pangoft2libs)
pango_indic_fc_la_SOURCES = $(fc_sources)
libpango_indic_fc_la_SOURCES = $(fc_sources)
libpango_indic_fc_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_indic_fc


included-modules: $(noinst_LTLIBRARIES)

.PHONY: included-modules

