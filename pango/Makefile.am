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
	-DG_LOG_DOMAIN=\"Pango\"			\
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

BUILT_SOURCES = pango-enum-types.h pango-enum-types.c

pangoincludedir=$(includedir)/pango-1.0/pango

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

if OS_WIN32
else
libm = -lm
endif

# ------------------- libpango -------------------

lib_LTLIBRARIES = libpango-1.0.la

libpango_1_0_la_LDFLAGS = -version-info $(LT_VERSION_INFO) $(no_undefined)
libpango_1_0_la_LIBADD = $(GLIB_LIBS) mini-fribidi/libmini-fribidi.la $(libm)
libpango_1_0_la_DEPENDENCIES = mini-fribidi/libmini-fribidi.la

if OS_WIN32
libpango_1_0_la_LDFLAGS += -export-symbols $(srcdir)/pango.def
libpango_1_0_la_LIBADD += pango-win32res.lo
libpango_1_0_la_DEPENDENCIES += pango-win32res.lo
endif
pango-win32res.lo: pango.rc
	$(top_srcdir)/../glib/build/win32/lt-compile-resource $< $@

libpango_1_0_la_SOURCES =      \
	break.c 	       \
	ellipsize.c     	\
	fonts.c 	       \
	glyphstring.c 	       \
	mapping.c 	       \
	modules.c 	       \
	pango-attributes.c     \
	pango-color.c	       \
	pango-context.c        \
	pango-coverage.c       \
	pango-engine.c	       \
	pango-engine-private.h \
	pango-fontmap.c	       \
	pango-fontset.c	       \
	pango-glyph-item.c     \
	pango-glyph-item-private.h     \
	pango-impl-utils.h     \
	pango-item.c	       \
	pango-layout.c	       \
	pango-layout-private.h	       \
	pango-markup.c	       \
	pango-renderer.c       \
	pango-script.c	       \
	pango-script-lang-table.h \
	pango-script-table.h   \
	pango-tabs.c	       \
	pango-utils.c	       \
	reorder-items.c        \
	shape.c	               \
	pango-enum-types.c

pango_headers =		   \
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
	pango-item.h	   \
	pango-layout.h	   \
	pango-modules.h	   \
	pango-renderer.h   \
	pango-script.h	   \
	pango-tabs.h	   \
	pango-types.h	   \
	pango-utils.h	   \
	pangofc-font.h	   \
	pangofc-fontmap.h

pangoinclude_HEADERS =	   \
	$(pango_headers)   \
	pango-enum-types.h

pango-enum-types.h: s-enum-types-h
	@true
s-enum-types-h: @REBUILD@ $(pango_headers) Makefile
	( cd $(srcdir) && glib-mkenums \
			--fhead "#ifndef __PANGO_ENUM_TYPES_H__\n#define __PANGO_ENUM_TYPES_H__\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void);\n#define PANGO_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" 	\
			--ftail "G_END_DECLS\n\n#endif /* __PANGO_ENUM_TYPES_H__ */" \
		$(pango_headers) ) > tmp-pango-enum-types.h \
	&& (cmp -s tmp-pango-enum-types.h pango-enum-types.h || cp tmp-pango-enum-types.h pango-enum-types.h ) \
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
	&& (cmp -s tmp-pango-enum-types.c pango-enum-types.c || cp tmp-pango-enum-types.c pango-enum-types.c ) \
	&& rm -f tmp-pango-enum-types.c \
	&& echo timestamp > $(@F)


# ------------------- libpangox -------------------

if HAVE_X
pangoinclude_HEADERS += pangox.h
lib_LTLIBRARIES += libpangox-1.0.la
endif

libpangox_1_0_la_LDFLAGS = -version-info $(LT_VERSION_INFO) $(no_undefined)
libpangox_1_0_la_LIBADD = libpango-$(PANGO_API_VERSION).la $(INCLUDED_X_MODULES) $(X_LIBS) $(X_EXTRA_LIBS) $(GLIB_LIBS)
libpangox_1_0_la_DEPENDENCIES = libpango-$(PANGO_API_VERSION).la $(INCLUDED_X_MODULES)

libpangox_1_0_la_SOURCES = \
	modules.h 	   \
	module-defs-x.c	   \
	pangox.c	   \
	pangox-private.h   \
	pangox-fontcache.c \
	pangox-fontmap.c


# ------------------- libpangoft2 -------------------

if HAVE_FREETYPE
pangoinclude_HEADERS += pangoft2.h pango-ot.h pangofc-decoder.h
lib_LTLIBRARIES += libpangoft2-1.0.la
endif

libpangoft2_1_0_la_LDFLAGS = -version-info $(LT_VERSION_INFO) $(no_undefined)
libpangoft2_1_0_la_LIBADD = opentype/libpango-ot.la libpango-$(PANGO_API_VERSION).la $(INCLUDED_FC_MODULES) $(FREETYPE_LIBS) $(GLIB_LIBS) $(libm)
libpangoft2_1_0_la_DEPENDENCIES = opentype/libpango-ot.la libpango-$(PANGO_API_VERSION).la $(INCLUDED_FC_MODULES)

libpangoft2_1_0_la_SOURCES =   	\
	pangofc-font.c	   	\
	pangofc-fontmap.c	\
	pangofc-decoder.c	\
	pangofc-private.h	\
	pangoft2.h		\
	pangoft2.c		\
	pangoft2-private.h	\
	pangoft2-fontmap.c	\
	pangoft2-render.c	\
	module-defs-fc.c

if OS_WIN32
libpangoft2_1_0_la_LDFLAGS += -export-symbols $(srcdir)/pangoft2.def
libpangoft2_1_0_la_LIBADD += pangoft2-win32res.lo
libpangoft2_1_0_la_DEPENDENCIES += pangoft2-win32res.lo
endif
pangoft2-win32res.lo: pangoft2.rc
	$(top_srcdir)/../glib/build/win32/lt-compile-resource $< $@


# ------------------- libpangoxft -------------------

if HAVE_XFT
pangoinclude_HEADERS += pangoxft.h pangoxft-render.h pango-ot.h
lib_LTLIBRARIES += libpangoxft-1.0.la
endif

libpangoxft_1_0_la_LDFLAGS = -version-info $(LT_VERSION_INFO) $(no_undefined)
libpangoxft_1_0_la_LIBADD = libpangoft2-$(PANGO_API_VERSION).la libpango-$(PANGO_API_VERSION).la $(XFT_LIBS) $(GLIB_LIBS)
libpangoxft_1_0_la_DEPENDENCIES = libpangoft2-$(PANGO_API_VERSION).la libpango-$(PANGO_API_VERSION).la

libpangoxft_1_0_la_SOURCES =   	\
	pangoxft-font.c		\
	pangoxft-fontmap.c      \
	pangoxft-private.h	\
	pangoxft-render.c


# ------------------- libpangowin32 -------------------

if HAVE_WIN32
pangoinclude_HEADERS += pangowin32.h
lib_LTLIBRARIES += libpangowin32-1.0.la
endif

libpangowin32_1_0_la_LDFLAGS = -version-info $(LT_VERSION_INFO)	$(no_undefined)
libpangowin32_1_0_la_LIBADD = libpango-$(PANGO_API_VERSION).la $(INCLUDED_WIN32_MODULES) $(WIN32_LIBS) $(GLIB_LIBS)
libpangowin32_1_0_la_DEPENDENCIES = libpango-$(PANGO_API_VERSION).la $(INCLUDED_WIN32_MODULES)

libpangowin32_1_0_la_SOURCES =	\
	pangowin32.h		\
	pangowin32.c		\
	pangowin32-private.h	\
	pangowin32-fontcache.c	\
	pangowin32-fontmap.c	\
	module-defs-win32.c

if OS_WIN32
libpangowin32_1_0_la_LDFLAGS += -export-symbols $(srcdir)/pangowin32.def
libpangowin32_1_0_la_LIBADD += pangowin32-win32res.lo
libpangowin32_1_0_la_DEPENDENCIES += pangowin32-win32res.lo
endif
pangowin32-win32res.lo: pangowin32.rc
	$(top_srcdir)/../glib/build/win32/lt-compile-resource $< $@


# ------------------- win32 build stuff -------------------

if OS_WIN32
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

pango-$(PANGO_API_VERSION).lib: libpango-$(PANGO_API_VERSION).la $(srcdir)/pango.def
	lib -name:libpango-$(PANGO_API_VERSION)-$(LT_CURRENT_MINUS_AGE).dll -def:$(srcdir)/pango.def -out:$@

pangoft2-$(PANGO_API_VERSION).lib: libpangoft2-$(PANGO_API_VERSION).la $(srcdir)/pangoft2.def
	lib -name:libpangoft2-$(PANGO_API_VERSION)-$(LT_CURRENT_MINUS_AGE).dll -def:$(srcdir)/pangoft2.def -out:$@

pangowin32-$(PANGO_API_VERSION).lib: libpangowin32-$(PANGO_API_VERSION).la $(srcdir)/pangowin32.def
	lib -name:libpangowin32-$(PANGO_API_VERSION)-$(LT_CURRENT_MINUS_AGE).dll -def:$(srcdir)/pangowin32.def -out:$@


# ------------------- pango-querymodules -------------------

bin_PROGRAMS = pango-querymodules

#
# As a temporary hack, we pull in libpangox here so that we can link
#
pango_querymodules_SOURCES = \
	querymodules.c

pango_querymodules_LDADD =
if HAVE_X
pango_querymodules_LDADD += libpangox-$(PANGO_API_VERSION).la
endif
if HAVE_XFT
pango_querymodules_LDADD += libpangoxft-$(PANGO_API_VERSION).la
endif
if HAVE_FREETYPE
pango_querymodules_LDADD += libpangoft2-$(PANGO_API_VERSION).la
endif
if HAVE_WIN32
pango_querymodules_LDADD += libpangowin32-$(PANGO_API_VERSION).la
endif
pango_querymodules_LDADD += libpango-$(PANGO_API_VERSION).la $(GLIB_LIBS)

EXTRA_DIST =					\
	pango-enum-types.h			\
	module-defs-fc.c.win32			\
	module-defs-win32.c.win32		\
	module-defs.h				\
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

$(INCLUDED_X_MODULES) $(INCLUDED_FC_MODULES) $(INCLUDED_WIN32_MODULES): included-modules
	@true

.PHONY: included-modules
endif

module-defs.h: $(top_builddir)/config.status
	cd $(top_builddir) && $(SHELL) ./config.status pango/$@
module-defs-x.c: $(top_builddir)/config.status
	cd $(top_builddir) && $(SHELL) ./config.status pango/$@
module-defs-fc.c: $(top_builddir)/config.status
	cd $(top_builddir) && $(SHELL) ./config.status pango/$@
module-defs-win32.c: $(top_builddir)/config.status
	cd $(top_builddir) && $(SHELL) ./config.status pango/$@

CLEANFILES =			\
	pango-enum-types.h	\
	s-enum-types-h		\
	pango-enum-types.c	\
	s-enum-types-c		\
	module-defs.h		\
	module-defs-x.c		\
	module-defs-fc.c	\
	module-defs-win32.c


install-data-local: install-ms-lib install-libtool-import-lib

install-exec-hook: 
if DISABLE_EXPLICIT_DEPS
	-for i in libpangoft2-$(PANGO_API_VERSION).la libpangoxft-$(PANGO_API_VERSION).la libpangox-$(PANGO_API_VERSION).la libpangowin32-$(PANGO_API_VERSION).la; do \
	  test -f $(DESTDIR)$(libdir)/$$i && $(SHELL) $(top_srcdir)/sanitize-la.sh $(DESTDIR)$(libdir)/$$i ; \
	done
endif

uninstall-local: uninstall-ms-lib uninstall-libtool-import-lib
