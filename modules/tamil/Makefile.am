## Process this file with automake to create Makefile.in.

pangolibs = $(top_builddir)/pango/libpango-$(PANGO_API_VERSION).la $(FRIBIDI_LIBS) $(GLIB_LIBS)
pangoxlibs = $(top_builddir)/pango/libpangox-$(PANGO_API_VERSION).la $(X_LIBS) $(pangolibs)
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

if HAVE_X
INCLUDES += $(X_CFLAGS)
if INCLUDE_TAMIL_X
noinst_LTLIBRARIES += libpango-tamil-x.la
else
module_LTLIBRARIES += pango-tamil-x.la
endif
endif

x_sources = \
	tamil-x.c \
	tadefs.h

pango_tamil_x_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_tamil_x_la_LIBADD = $(pangoxlibs)
pango_tamil_x_la_SOURCES = $(x_sources)
libpango_tamil_x_la_SOURCES = $(x_sources)
libpango_tamil_x_la_CFLAGS = -DPANGO_MODULE_PREFIX=_pango_tamil_x


included-modules: $(noinst_LTLIBRARIES)

.PHONY: included-modules
