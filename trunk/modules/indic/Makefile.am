include $(top_srcdir)/modules/Module.mk

if HAVE_FREETYPE
INCLUDES += $(FREETYPE_CFLAGS)
if INCLUDE_INDIC_FC
noinst_LTLIBRARIES += libpango-indic-fc.la
else
if DYNAMIC_INDIC_FC
module_LTLIBRARIES += pango-indic-fc.la
endif
endif
endif

fc_sources =		 	\
	indic-fc.c		\
	indic-ot-class-tables.c	\
	indic-ot.c		\
	indic-ot.h		\
	mprefixups.c		\
	mprefixups.h

pango_indic_fc_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS)
pango_indic_fc_la_LIBADD = $(pangoft2libs)
pango_indic_fc_la_SOURCES = $(fc_sources)
libpango_indic_fc_la_SOURCES = $(fc_sources)
libpango_indic_fc_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_indic_fc


if INCLUDE_INDIC_LANG
noinst_LTLIBRARIES += libpango-indic-lang.la
else
if DYNAMIC_INDIC_LANG
module_LTLIBRARIES += pango-indic-lang.la
endif
endif

lang_sources =		\
	indic-lang.c

pango_indic_lang_la_LDFLAGS = -module $(MODULE_LIBTOOL_OPTIONS)
pango_indic_lang_la_LIBADD = $(pangolibs)
pango_indic_lang_la_SOURCES = $(lang_sources)
libpango_indic_lang_la_SOURCES = $(lang_sources)
libpango_indic_lang_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_indic_lang
