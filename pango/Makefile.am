	## Process this file with automake to create Makefile.in.

INCLUDES=-DSYSCONFDIR=\"$(sysconfdir)\" -DLOCALSTATEDIR=\"$(localstatedir)\" -I$(top_srcdir)

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
	pango-item.c	   \
	pango-layout.c	   \
	reorder-items.c    \
	shape.c 	   \
	utils.c  	   \
	utils.h

libpangox_la_SOURCES = 	   \
	pangox.c	   \
	pangox-fontmap.c

# We use the -release flag for now until the API settles
# down at which point we'll either use the GTK+ versioning
# scheme or the standard libtool scheme.
#
libpango_la_LDFLAGS = -release $(VERSION) $(GLIB_LIBS)

libpangox_la_LDFLAGS = -release $(VERSION)
libpangox_la_LIBADD = $(INCLUDED_MODULES)

pangoincludedir=$(includedir)/pango

pangoinclude_HEADERS = 	   \
	pango.h		   \
	pango-attributes.h \
	pango-context.h	   \
	pango-coverage.h   \
	pango-engine.h	   \
	pango-font.h       \
	pango-glyph.h	   \
	pango-item.h	   \
	pango-layout.h	   \
	pango-types.h	   \
	pangox.h	

#
# As a temporary hack, we pull in libpangox here so that we can link
#
pango_querymodules_SOURCES = \
	querymodules.c
pango_querymodules_LDADD = libpangox.la libpango.la $(GLIB_LIBS) $(FRIBIDI_LIBS) $(UNICODE_LIBS) $(X_LIBS)

