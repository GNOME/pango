## Process this file with automake to create Makefile.in.

if HAVE_FRIBIDI
else
SUBDIRS = mini-fribidi
endif

INCLUDES = 					\
	-DSYSCONFDIR=\"$(sysconfdir)\" 		\
	-DLIBDIR=\"$(libdir)\" 			\
	-I$(top_srcdir)

lib_LTLIBRARIES = libpango.la libpangox.la
bin_PROGRAMS = pango-querymodules

libpango_la_SOURCES = 	   \
	break.c 	   \
	fonts.c 	   \
	glyphstring.c 	   \
	mapping.c 	   \
	modules.c 	   \
	modules.h 	   \
	module-defs.c	   \
	pango-attributes.c \
	pango-context.c    \
	pango-coverage.c   \
	pango-fontmap.c	   \
	pango-intset.c	   \
	pango-intset.h	   \
	pango-item.c	   \
	pango-layout.c	   \
	pango-markup.c	   \
	pango-tabs.c	   \
	pango-utils.c	   \
	reorder-items.c    \
	shape.c

libpangox_la_SOURCES = 	   \
	pangox.c	   \
	pangox-private.h   \
	pangox-fontcache.c \
	pangox-fontmap.c   \
	pango-indic.c

# We use the -release flag for now until the API settles
# down at which point we'll either use the GTK+ versioning
# scheme or the standard libtool scheme.
#
libpango_la_LDFLAGS = -release $(VERSION) $(GLIB_LIBS)
if HAVE_FRIBIDI
else
libpango_la_LIBADD = mini-fribidi/libmini-fribidi.la
endif

libpangox_la_LDFLAGS = -release $(VERSION)
libpangox_la_LIBADD = $(INCLUDED_MODULES)

pangoincludedir=$(includedir)/pango-1.0/pango

pangoinclude_HEADERS = 	   \
	pango.h		   \
	pango-attributes.h \
	pango-context.h	   \
	pango-coverage.h   \
	pango-engine.h	   \
	pango-font.h       \
	pango-fontmap.h	   \
	pango-glyph.h	   \
	pango-indic.h	   \
	pango-item.h	   \
	pango-layout.h	   \
	pango-modules.h	   \
	pango-tabs.h	   \
	pango-types.h	   \
	pango-utils.h	   \
	pangox.h	

#
# As a temporary hack, we pull in libpangox here so that we can link
#
pango_querymodules_SOURCES = \
	querymodules.c
pango_querymodules_LDADD = libpangox.la libpango.la $(GLIB_LIBS) $(FRIBIDI_LIBS) $(X_LIBS)

EXTRA_DIST =			\
	module-defs.c.win32	\
	pango.def		\
	pangowin32.def		\
	pangowin32.h		\
	pangowin32.c		\
	pangowin32-private.h	\
	pangowin32-fontcache.c	\
	pangowin32-fontmap.c	\
	pangoft2.def		\
	pangoft2.h		\
	pangoft2.c		\
	pangoft2-private.h	\
	pangoft2-fontcache.c	\
	pangoft2-fontmap.c	\
	makefile.mingw		\
	makefile.mingw.in	\
	pango.rc		\
	pango.rc.in		\
	pangoft2.rc		\
	pangoft2.rc.in		\
	pangowin32.rc		\
	pangowin32.rc.in


makefile.mingw: $(top_builddir)/config.status $(top_srcdir)/pango/makefile.mingw.in
	cd $(top_builddir) && CONFIG_FILES=pango/$@ CONFIG_HEADERS= $(SHELL) ./config.status

pango2.rc: $(top_builddir)/config.status $(top_srcdir)/pango/pango.rc.in
	cd $(top_builddir) && CONFIG_FILES=pango/$@ CONFIG_HEADERS= $(SHELL) ./config.status

pangoft2.rc: $(top_builddir)/config.status $(top_srcdir)/pango/pangoft2.rc.in
	cd $(top_builddir) && CONFIG_FILES=pango/$@ CONFIG_HEADERS= $(SHELL) ./config.status

pangowin32.rc: $(top_builddir)/config.status $(top_srcdir)/pango/pangowin32.rc.in
	cd $(top_builddir) && CONFIG_FILES=pango/$@ CONFIG_HEADERS= $(SHELL) ./config.status
