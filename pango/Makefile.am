## Process this file with automake to create Makefile.in.

# We set GPATH here; this gives us semantics for GNU make
# which are more like other make's VPATH.
GPATH = $(srcdir)

if HAVE_FREETYPE
OPENTYPE_SUBDIR=opentype
endif

SUBDIRS = $(OPENTYPE_SUBDIR) mini-fribidi

DIST_SUBDIRS = mini-fribidi opentype

INCLUDES =						\
	-DPANGO_ENABLE_BACKEND				\
	-DPANGO_ENABLE_ENGINE				\
	-DSYSCONFDIR=\"$(sysconfdir)\"			\
	-DLIBDIR=\"$(libdir)\"				\
	-DMODULE_VERSION=\"$(PANGO_MODULE_VERSION)\"	\
	-DG_DISABLE_DEPRECATED				\
	$(PANGO_DEBUG_FLAGS)				\
	$(GLIB_CFLAGS)					\
	$(XFT_CFLAGS)					\
	$(FREETYPE_CFLAGS)				\
	$(X_CFLAGS)					\
	-I$(top_srcdir)

if HAVE_X
X_HFILES=pangox.h
X_BUILD_LIBS=libpangox-1.0.la
else
X_HFILES=
X_BUILD_LIBS=
endif

if HAVE_WIN32
WIN32_HFILES=pangowin32.h
WIN32_BUILD_LIBS=libpangowin32-1.0.la
else
WIN32_HFILES=
WIN32_BUILD_LIBS=
endif

if HAVE_XFT
XFT_HFILES=pangoxft.h pango-ot.h
XFT_BUILD_LIBS=libpangoxft-1.0.la
else
XFT_HFILES=
XFT_BUILD_LIBS=
endif

if HAVE_FREETYPE
FT2_HFILES=pangoft2.h pango-ot.h
FT2_BUILD_LIBS=libpangoft2-1.0.la
else
FT2_HFILES=
FT2_BUILD_LIBS=
endif

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if OS_WIN32
pango_export_symbols = -export-symbols $(srcdir)/pango.def
pangowin32_export_symbols = -export-symbols $(srcdir)/pangowin32.def
pangoft2_export_symbols = -export-symbols $(srcdir)/pangoft2.def

pango_win32res = pango-win32res.lo
pangowin32_win32res = pangowin32-win32res.lo
pangoft2_win32res = pangoft2-win32res.lo

install-libtool-import-lib:
	$(INSTALL) .libs/libpango-$(PANGO_API_VERSION).dll.a $(DESTDIR)$(libdir)
if HAVE_FREETYPE
	$(INSTALL) .libs/libpangoft2-$(PANGO_API_VERSION).dll.a $(DESTDIR)$(libdir)
endif
	$(INSTALL) .libs/libpangowin32-$(PANGO_API_VERSION).dll.a $(DESTDIR)$(libdir)
uninstall-libtool-import-lib:
	-rm $(DESTDIR)$(libdir)/libpango-$(PANGO_API_VERSION).dll.a
	-rm $(DESTDIR)$(libdir)/libpangoft2-$(PANGO_API_VERSION).dll.a
	-rm $(DESTDIR)$(libdir)/libpangowin32-$(PANGO_API_VERSION).dll.a
else
install-libtool-import-lib:
uninstall-libtool-import-lib:

endif

lib_LTLIBRARIES = libpango-1.0.la $(X_BUILD_LIBS) $(WIN32_BUILD_LIBS) $(XFT_BUILD_LIBS) $(FT2_BUILD_LIBS)

if MS_LIB_AVAILABLE
noinst_DATA = pango-$(PANGO_API_VERSION).lib pangoft2-$(PANGO_API_VERSION).lib pangowin32-$(PANGO_API_VERSION).lib

install-ms-lib:
	$(INSTALL) pango-$(PANGO_API_VERSION).lib $(DESTDIR)$(libdir)
if HAVE_FREETYPE
	$(INSTALL) pangoft2-$(PANGO_API_VERSION).lib $(DESTDIR)$(libdir)
endif
	$(INSTALL) pangowin32-$(PANGO_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/pango-$(PANGO_API_VERSION).lib
	-rm $(DESTDIR)$(libdir)/pangoft2-$(PANGO_API_VERSION).lib
	-rm $(DESTDIR)$(libdir)/pangowin32-$(PANGO_API_VERSION).lib
else
install-ms-lib:
uninstall-ms-lib:
endif

bin_PROGRAMS = pango-querymodules

pango_built_headers = pango-enum-types.h
pango_built_cfiles = pango-enum-types.c

$(OBJECTS) $(libpango_1_0_la_OBJECTS): $(pango_built_headers)

libpango_1_0_la_SOURCES =  \
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
	pango-glyph-item.c \
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

libpangox_1_0_la_SOURCES = \
	modules.h 	   \
	module-defs-x.c	   \
	pangox.c	   \
	pangox-private.h   \
	pangox-fontcache.c \
	pangox-fontmap.c   \
	pango-indic.c

libpango_1_0_la_LDFLAGS =			\
	-version-info $(LT_VERSION_INFO)	\
	$(GLIB_LIBS)				\
	$(no_undefined) $(pango_export_symbols)

libpango_1_0_la_LIBADD = mini-fribidi/libmini-fribidi.la $(pango_win32res)
libpango_1_0_la_DEPENDENCIES = $(pango_win32res)

libpangox_1_0_la_LDFLAGS = -version-info $(LT_VERSION_INFO)
libpangox_1_0_la_LIBADD = libpango-$(PANGO_API_VERSION).la $(INCLUDED_X_MODULES) $(X_LIBS) $(X_EXTRA_LIBS)
libpangox_1_0_la_DEPENDENCIES = $(INCLUDED_X_MODULES) libpango-$(PANGO_API_VERSION).la

pango-win32res.lo : pango.rc
	$(top_srcdir)/../glib/build/win32/lt-compile-resource pango.rc pango-win32res.lo

libpangowin32_1_0_la_SOURCES =	\
	pangowin32.h		\
	pangowin32.c		\
	pangowin32-private.h	\
	pangowin32-fontcache.c	\
	pangowin32-fontmap.c	\
	module-defs-win32.c

libpangowin32_1_0_la_LDFLAGS = -version-info $(LT_VERSION_INFO)	\
	$(GLIB_LIBS)						\
	$(no_undefined) $(pangowin32_export_symbols)
libpangowin32_1_0_la_LIBADD = libpango-$(PANGO_API_VERSION).la $(INCLUDED_WIN32_MODULES) $(WIN32_LIBS) $(pangowin32_win32res)
libpangowin32_1_0_la_DEPENDENCIES = $(INCLUDED_WIN32_MODULES) libpango-$(PANGO_API_VERSION).la $(pangowin32_win32res)

pangowin32-win32res.lo : pangowin32.rc
	$(top_srcdir)/../glib/build/win32/lt-compile-resource pangowin32.rc pangowin32-win32res.lo

libpangoxft_1_0_la_SOURCES =   	\
	pangoxft-font.c		\
	pangoxft-fontmap.c      \
	pangoxft-private.h	\
	module-defs-xft.c

libpangoxft_1_0_la_LDFLAGS = -version-info $(LT_VERSION_INFO)
libpangoxft_1_0_la_LIBADD = opentype/libpango-ot.la libpango-$(PANGO_API_VERSION).la $(INCLUDED_XFT_MODULES) $(XFT_LIBS) 
libpangoxft_1_0_la_DEPENDENCIES = $(INCLUDED_XFT_MODULES) libpango-$(PANGO_API_VERSION).la

libpangoft2_1_0_la_SOURCES =   	\
	pangoft2.h		\
	pangoft2.c		\
	pangoft2-private.h	\
	pangoft2-fontmap.c	\
	module-defs-ft2.c

libpangoft2_1_0_la_LDFLAGS = -version-info $(LT_VERSION_INFO)	\
	$(GLIB_LIBS)						\
	$(no_undefined) $(pangoft2_export_symbols)
libpangoft2_1_0_la_LIBADD = opentype/libpango-ot.la libpango-$(PANGO_API_VERSION).la $(INCLUDED_FT2_MODULES) $(FREETYPE_LIBS) $(pangoft2_win32res)
libpangoft2_1_0_la_DEPENDENCIES = $(INCLUDED_FT2_MODULES) libpango-$(PANGO_API_VERSION).la

pangoft2-win32res.lo : pangoft2.rc
	$(top_srcdir)/../glib/build/win32/lt-compile-resource pangoft2.rc pangoft2-win32res.lo

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
	pango-glyph-item.h \
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
			--fhead "#ifndef __PANGO_ENUM_TYPES_H__\n#define __PANGO_ENUM_TYPES_H__\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void);\n#define PANGO_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" 	\
			--ftail "G_END_DECLS\n\n#endif /* __PANGO_ENUM_TYPES_H__ */" \
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
pango_querymodules_LDADD = $(FT2_BUILD_LIBS) $(X_BUILD_LIBS) libpangoxft-$(PANGO_API_VERSION).la $(GLIB_LIBS) $(XFT_LIBS) $(FREETYPE_LIBS)
pango_querymodules_DEPENDENCIES = $(FT2_BUILD_LIBS) $(X_BUILD_LIBS) libpangoxft-$(PANGO_API_VERSION).la libpango-$(PANGO_API_VERSION).la
else
pango_querymodules_LDADD = $(FT2_BUILD_LIBS) $(X_BUILD_LIBS) $(WIN32_BUILD_LIBS) $(GLIB_LIBS) $(X_LIBS) $(FREETYPE_LIBS) $(X_EXTRA_LIBS)
pango_querymodules_DEPENDENCIES = $(FT2_BUILD_LIBS) $(X_BUILD_LIBS) $(WIN32_BUILD_LIBS) libpango-$(PANGO_API_VERSION).la
endif

EXTRA_DIST =					\
	$(pango_built_headers)			\
	module-defs-ft2.c.win32			\
	module-defs-win32.c.win32		\
	module-defs.h				\
	pangofc-fontmap.cI			\
	pango.def				\
	pangowin32.def				\
	pangoft2.def				\
	makefile.msc				\
	pango.rc				\
	pango.rc.in				\
	pangoft2.rc				\
	pangoft2.rc.in				\
	pangowin32.rc				\
	pangowin32.rc.in			\
	testfonts.c


if HAVE_INCLUDED_MODULES
included-modules:
	@cd $(top_builddir)/modules && $(MAKE) $(AM_MAKEFLAGS) included-modules

$(INCLUDED_X_MODULES) $(INCLUDED_XFT_MODULES) $(INCLUDED_FT2_MODULES) $(INCLUDED_WIN32_MODULES): included-modules
	@true

.PHONY: included-modules
endif

CLEANFILES =					\
	pango-enum-types.h			\
	s-enum-types-h				\
	pango-enum-types.c			\
	s-enum-types-c

pango-$(PANGO_API_VERSION).lib: libpango-$(PANGO_API_VERSION).la $(srcdir)/pango.def
	lib -name:libpango-$(PANGO_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:$(srcdir)/pango.def -out:$@

pangoft2-$(PANGO_API_VERSION).lib: libpangoft2-$(PANGO_API_VERSION).la $(srcdir)/pangoft2.def
	lib -name:libpangoft2-$(PANGO_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:$(srcdir)/pangoft2.def -out:$@

pangowin32-$(PANGO_API_VERSION).lib: libpangowin32-$(PANGO_API_VERSION).la $(srcdir)/pangowin32.def
	lib -name:libpangowin32-$(PANGO_API_VERSION)-@LT_CURRENT_MINUS_AGE@.dll -def:$(srcdir)/pangowin32.def -out:$@

install-data-local: install-ms-lib install-libtool-import-lib

install-exec-hook: 
if DISABLE_EXPLICIT_DEPS
	-for i in libpangoft2-$(PANGO_API_VERSION).la libpangoxft-$(PANGO_API_VERSION).la libpangox-$(PANGO_API_VERSION).la libpangowin32-$(PANGO_API_VERSION).la; do \
	  test -f $(DESTDIR)$(libdir)/$$i && $(SHELL) $(top_srcdir)/sanitize-la.sh $(DESTDIR)$(libdir)/$$i ; \
	done
endif

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib
