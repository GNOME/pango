## Process this file with automake to create Makefile.in.

# We set GPATH here; this gives us semantics for GNU make
# which are more like other make's VPATH.
GPATH = $(srcdir)

if HAVE_FREETYPE
OPENTYPE_SUBDIR=opentype
XFT_SUBDIR = mini-xft
else
XFT_SUBDIR = 
endif

if HAVE_FRIBIDI
else
FRIBIDI_SUBDIR = mini-fribidi
endif

SUBDIRS = $(OPENTYPE_SUBDIR) $(FRIBIDI_SUBDIR) $(XFT_SUBDIR)

DIST_SUBDIRS = mini-fribidi mini-xft opentype

INCLUDES = 					\
	-DPANGO_ENABLE_BACKEND			\
	-DPANGO_ENABLE_ENGINE			\
	-DSYSCONFDIR=\"$(sysconfdir)\" 		\
	-DLIBDIR=\"$(libdir)\" 			\
	$(X_CFLAGS)				\
	$(FREETYPE_CFLAGS)			\
	-I$(top_srcdir)

if HAVE_X
X_HFILES=pangox.h
X_BUILD_LIBS=libpangox.la
else
X_HFILES=
X_BUILD_LIBS=
endif

if HAVE_WIN32
WIN32_HFILES=pangowin32.h
WIN32_BUILD_LIBS=libpangowin32.la
else
WIN32_HFILES=
WIN32_BUILD_LIBS=
endif

if HAVE_XFT
XFT_HFILES=pangoxft.h pango-ot.h
XFT_BUILD_LIBS=libpangoxft.la
else
XFT_HFILES=
XFT_BUILD_LIBS=
endif

if HAVE_FREETYPE
FT2_BUILD_LIBS=libpangoft2.la
FT2_HFILES=pangoft2.h
else
FT2_BUILD_LIBS=
FT2_HFILES=
endif

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if OS_WIN32
pango_export_symbols = -export-symbols pango.def
pangowin32_export_symbols = -export-symbols pangowin32.def
pangoft2_export_symbols = -export-symbols pangoft2.def

install-libtool-import-lib:
	$(INSTALL) .libs/libpango.dll.a $(DESTDIR)$(libdir)
if HAVE_FREETYPE
	$(INSTALL) .libs/libpangoft2.dll.a $(DESTDIR)$(libdir)
endif
	$(INSTALL) .libs/libpangowin32.dll.a $(DESTDIR)$(libdir)
uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libpango.dll.a
	-rm $(DESTDIR)$(libdir)/libpangoft2.dll.a
	-rm $(DESTDIR)$(libdir)/libpangowin32.dll.a
else
install-libtool-import-lib:
uninstall-libtool-import-lib:

endif

lib_LTLIBRARIES = libpango.la $(X_BUILD_LIBS) $(WIN32_BUILD_LIBS) $(XFT_BUILD_LIBS) $(FT2_BUILD_LIBS)

if MS_LIB_AVAILABLE
noinst_DATA = pango.lib pangoft2.lib pangowin32.lib

install-ms-lib:
	$(INSTALL) pango.lib $(DESTDIR)$(libdir)
if HAVE_FREETYPE
	$(INSTALL) pangoft2.lib $(DESTDIR)$(libdir)
endif
	$(INSTALL) pangowin32.lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/pango.lib
	-rm $(DESTDIR)$(libdir)/pangoft2.lib
	-rm $(DESTDIR)$(libdir)/pangowin32.lib
else
install-ms-lib:
uninstall-ms-lib:
endif

bin_PROGRAMS = pango-querymodules

pango_built_headers = pango-enum-types.h
pango_built_cfiles = pango-enum-types.c

$(OBJECTS) $(libpango_la_OBJECTS): $(pango_built_headers)

libpango_la_SOURCES = 	   \
	break.c 	   \
	fonts.c 	   \
	glyphstring.c 	   \
	mapping.c 	   \
	modules.c 	   \
	pango-attributes.c \
	pango-color.c	   \
	pango-context.c    \
	pango-coverage.c   \
	pango-fontmap.c	   \
	pango-fontset.c	   \
	pango-intset.c	   \
	pango-intset.h	   \
	pango-item.c	   \
	pango-layout.c	   \
	pango-markup.c	   \
	pango-tabs.c	   \
	pango-utils.c	   \
	reorder-items.c    \
	shape.c	           \
	$(pango_built_cfiles)

libpangox_la_SOURCES = 	   \
	modules.h 	   \
	module-defs-x.c	   \
	pangox.c	   \
	pangox-private.h   \
	pangox-fontcache.c \
	pangox-fontmap.c   \
	pango-indic.c

# We use the -release flag for now until the API settles
# down at which point we'll either use the GTK+ versioning
# scheme or the standard libtool scheme.
#
libpango_la_LDFLAGS = -release $(VERSION) $(GLIB_LIBS) $(no_undefined) $(pango_export_symbols)
if HAVE_FRIBIDI
else
libpango_la_LIBADD = mini-fribidi/libmini-fribidi.la
endif

libpangox_la_LDFLAGS = -release $(VERSION)
libpangox_la_LIBADD =  $(INCLUDED_X_MODULES) $(X_LIBS) libpango.la
libpangox_la_DEPENDENCIES = $(INCLUDED_X_MODULES) libpango.la

libpangowin32_la_SOURCES =	\
	pangowin32.h		\
	pangowin32.c		\
	pangowin32-private.h	\
	pangowin32-fontcache.c	\
	pangowin32-fontmap.c	\
	module-defs-win32.c

libpangowin32_la_LDFLAGS = -release $(VERSION) $(GLIB_LIBS) $(no_undefined) $(pangowin32_export_symbols)
libpangowin32_la_LIBADD = $(INCLUDED_WIN32_MODULES) $(WIN32_LIBS) libpango.la
libpangowin32_la_DEPENDENCIES = libpango.la


libpangoxft_la_SOURCES =   	\
	pangoxft-font.c		\
	pangoxft-fontmap.c      \
	pangoxft-private.h	\
	module-defs-xft.c

libpangoxft_la_LIBADD = $(INCLUDED_XFT_MODULES) $(XFT_LIBS) opentype/libpango-ot.la libpango.la
libpangoxft_la_LDFLAGS = -release $(VERSION)
libpangoxft_la_DEPENDENCIES = $(INCLUDED_XFT_MODULES) libpango.la

libpangoft2_la_SOURCES =   	\
	pangoft2.h		\
	pangoft2.c		\
	pangoft2-private.h	\
	pangoft2-fontmap.c	\
	module-defs-ft2.c

libpangoft2_la_LIBADD = $(INCLUDED_FT2_MODULES) $(FREETYPE_LIBS) mini-xft/libmini-xft.la libpango.la
libpangoft2_la_LDFLAGS = -release $(VERSION) $(GLIB_LIBS) $(no_undefined) $(pangoft2_export_symbols)
libpangoft2_la_DEPENDENCIES = $(INCLUDED_FT2_MODULES) libpango.la

pangoincludedir=$(includedir)/pango-1.0/pango

pango_headers=		   \
	pango.h		   \
	pango-attributes.h \
	pango-break.h	   \
	pango-context.h	   \
	pango-coverage.h   \
	pango-engine.h	   \
	pango-font.h       \
	pango-fontmap.h	   \
	pango-fontset.h    \
	pango-glyph.h	   \
	pango-indic.h	   \
	pango-item.h	   \
	pango-layout.h	   \
	pango-modules.h	   \
	pango-tabs.h	   \
	pango-types.h	   \
	pango-utils.h

pangoinclude_HEADERS = 	   	\
	$(pango_headers)   	\
	$(pango_built_headers)	\
	$(X_HFILES)		\
	$(XFT_HFILES)           \
	$(FT2_HFILES)		\
	$(WIN32_HFILES)

pango-enum-types.h: s-enum-types-h
	@true
 
s-enum-types-h: @REBUILD@ $(pango_headers) Makefile
	( cd $(srcdir) && glib-mkenums \
			--fhead "#ifndef __PANGO_ENUM_TYPES_H__\n#define __PANGO_ENUM_TYPES_H__\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void);\n#define PANGO_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" 	\
			--ftail "#endif /* __PANGO_ENUM_TYPES_H__ */" \
		$(pango_headers) ) > tmp-pango-enum-types.h \
	&& (cmp -s tmp-pango-enum-types.h $(srcdir)/pango-enum-types.h || cp tmp-pango-enum-types.h $(srcdir)/pango-enum-types.h ) \
	&& rm -f tmp-pango-enum-types.h	\
	&& echo timestamp > $(@F)

pango-enum-types.c: s-enum-types-c
	@true

s-enum-types-c: @REBUILD@ $(pango_headers) Makefile
	( cd $(srcdir) && glib-mkenums \
			--fhead "#include <pango.h>" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" 	\
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(pango_headers) ) > tmp-pango-enum-types.c \
	&& (cmp -s tmp-pango-enum-types.c $(srcdir)/pango-enum-types.c || cp tmp-pango-enum-types.c $(srcdir)/pango-enum-types.c ) \
	&& rm -f tmp-pango-enum-types.c \
	&& echo timestamp > $(@F)

#
# As a temporary hack, we pull in libpangox here so that we can link
#
pango_querymodules_SOURCES = \
	querymodules.c

if HAVE_XFT
pango_querymodules_LDADD = $(FT2_BUILD_LIBS) $(X_BUILD_LIBS) libpangoxft.la libpango.la $(GLIB_LIBS) $(FRIBIDI_LIBS) $(XFT_LIBS) $(FREETYPE_LIBS)
pango_querymodules_DEPENDENCIES = $(FT2_BUILD_LIBS) $(X_BUILD_LIBS) libpangoxft.la libpango.la
else
pango_querymodules_LDADD = $(FT2_BUILD_LIBS) $(X_BUILD_LIBS) libpango.la $(GLIB_LIBS) $(FRIBIDI_LIBS) $(X_LIBS) $(FREETYPE_LIBS)
pango_querymodules_DEPENDENCIES = $(FT2_BUILD_LIBS) $(X_BUILD_LIBS) libpango.la
endif

EXTRA_DIST =					\
	$(pango_built_headers)			\
	module-defs-ft2.c.win32			\
	module-defs-win32.c.win32		\
	module-defs.h				\
	pango.def				\
	pangowin32.def				\
	pangoft2.def				\
	makefile.mingw				\
	makefile.mingw.in			\
	pango.rc				\
	pango.rc.in				\
	pangoft2.rc				\
	pangoft2.rc.in				\
	pangowin32.rc				\
	pangowin32.rc.in


if HAVE_INCLUDED_MODULES
included-modules:
	@cd $(top_builddir)/modules && $(MAKE) $(AM_MAKEFLAGS) included-modules

$(INCLUDED_X_MODULES) $(INCLUDED_XFT_MODULES) $(INCLUDED_FT2_MODULES): included-modules
	@true

.PHONY: included-modules
endif

CLEANFILES =					\
	pango-enum-types.h			\
	s-enum-types-h				\
	pango-enum-types.c			\
	s-enum-types-c

pango.lib: libpango.la pango.def
	lib -name:libpango-@PANGO_MAJOR_VERSION@-@PANGO_MINOR_VERSION@.dll -def:pango.def -out:$@

pangoft2.lib: libpangoft2.la pangoft2.def
	lib -name:libpangoft2-@PANGO_MAJOR_VERSION@-@PANGO_MINOR_VERSION@.dll -def:pangoft2.def -out:$@

pangowin32.lib: libpangowin32.la pangowin32.def
	lib -name:libpangowin32-@PANGO_MAJOR_VERSION@-@PANGO_MINOR_VERSION@.dll -def:pangowin32.def -out:$@

install-data-local: install-ms-lib install-libtool-import-lib

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib
