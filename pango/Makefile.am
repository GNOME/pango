	## Process this file with automake to create Makefile.in.

INCLUDES=-DSYSCONFDIR=\"$(sysconfdir)\"

lib_LTLIBRARIES = libpango.la
bin_PROGRAMS = pango-querymodules

libpango_la_SOURCES = \
	break.c \
	fonts.c \
	glyphstring.c \
	pangox.c \
	mapping.c \
	modules.c \
	modules.h \
	pango-context.c \
	pango-coverage.c \
	reorder-items.c \
	shape.c \
	utils.c \
	utils.h

# We use the -release flag for now until the API settles
# down at which point we'll either use the GTK+ versioning
# scheme or the standard libtool scheme.
#
libpango_la_LDFLAGS = -release $(VERSION)

include_HEADERS = pango.h pangox.h

pango_querymodules_SOURCES = \
	querymodules.c
pango_querymodules_LDADD = libpango.la $(GTK_LIBS) -lfribidi $(UNICODE_LIBS)

