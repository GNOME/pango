## Process this file with automake to create Makefile.in.

pangolibs = $(top_builddir)/pango/libpango-$(PANGO_API_VERSION).la $(FRIBIDI_LIBS) $(GLIB_LIBS)
pangoxlibs = $(top_builddir)/pango/libpangox-$(PANGO_API_VERSION).la $(X_LIBS) $(pangolibs)
pangoft2libs = $(top_builddir)/pango/libpangoft2-$(PANGO_API_VERSION).la $(FREETYPE_LIBS) $(pangolibs)

x_sources = \
	tamil-x.c \
	tadefs.h

if HAVE_X
if INCLUDE_TAMIL_X
X_MODULES=
X_INCLUDED=libpango-tamil-x.la
X_PREFIX=-DX_MODULE_PREFIX
else
X_MODULES=pango-tamil-x.la
X_INCLUDED=
X_PREFIX=
tamil_x_libadd=$(pangoxlibs)
endif
else
X_MODULES=
X_INCLUDED=
X_PREFIX=
endif

noinst_LTLIBRARIES = $(X_INCLUDED)
module_LTLIBRARIES = $(X_MODULES)
moddefine = $(X_PREFIX)
moduledir = $(libdir)/pango/$(PANGO_MODULE_VERSION)/modules

INCLUDES = 				\
	-DPANGO_ENABLE_ENGINE		\
	-DG_DISABLE_DEPRECATED		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/pango/		\
	$(moddefine)			\
	$(GLIB_CFLAGS)			\
	$(FREETYPE_CFLAGS)		\
	$(X_CFLAGS) 

pango_tamil_x_la_LDFLAGS = -export-dynamic -avoid-version -module
pango_tamil_x_la_LIBADD = $(tamil_x_libadd)
pango_tamil_x_la_SOURCES = $(x_sources)

libpango_tamil_x_la_SOURCES = $(x_sources)

included-modules: $(noinst_LTLIBRARIES)

.PHONY: included-modules
