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
