## Process this file with automake to create Makefile.in.

# We set GPATH here; this gives us semantics for GNU make
# which are more like other make's VPATH.
GPATH = $(srcdir)

include $(INTROSPECTION_MAKEFILE)

INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --warn-all --add-include-path=$(srcdir) --add-include-path=.
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir) --includedir=.

if HAVE_FREETYPE
OPENTYPE_SUBDIR=opentype
endif

FRIBIDI_SUBDIR=mini-fribidi

SUBDIRS = $(OPENTYPE_SUBDIR) $(FRIBIDI_SUBDIR)

DIST_SUBDIRS = mini-fribidi opentype

INCLUDES =						\
	-DG_LOG_DOMAIN=\"Pango\"			\
	-DPANGO_ENABLE_BACKEND				\
	-DPANGO_ENABLE_ENGINE				\
	-DSYSCONFDIR=\"$(sysconfdir)\"			\
	-DLIBDIR=\"$(libdir)\"				\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	$(PANGO_DEBUG_FLAGS)				\
	$(GLIB_CFLAGS)					\
	$(XFT_CFLAGS)					\
	$(CAIRO_CFLAGS)					\
	$(FREETYPE_CFLAGS)				\
	$(X_CFLAGS)

BUILT_SOURCES = pango-enum-types.h pango-enum-types.c pango-features.h

pangoincludedir=$(includedir)/pango-1.0/pango

if OS_WIN32
else
libm = -lm
endif

# ------------------- libpango -------------------

lib_LTLIBRARIES = libpango-1.0.la
INTROSPECTION_GIRS += Pango-1.0.gir

libpango_1_0_la_LDFLAGS = $(LIBRARY_LIBTOOL_OPTIONS)
libpango_1_0_la_LIBADD =		\
	$(GLIB_LIBS)			\
	$(libm)				\
	$(INCLUDED_LANG_MODULES)
libpango_1_0_la_DEPENDENCIES = 		\
	$(INCLUDED_LANG_MODULES)

libpango_1_0_la_LIBADD += mini-fribidi/libmini-fribidi.la
libpango_1_0_la_DEPENDENCIES += mini-fribidi/libmini-fribidi.la

if PLATFORM_WIN32
libpango_1_0_la_LDFLAGS += -export-symbols $(srcdir)/pango.def -Wl,pango-win32-res.o
libpango_1_0_la_DEPENDENCIES += pango-win32-res.o pango.def
endif
pango-win32-res.o: pango.rc
	$(AM_V_GEN) $(WINDRES) $< $@

libpango_1_0_la_SOURCES =			\
	break.c					\
	ellipsize.c				\
	fonts.c					\
	glyphstring.c				\
	modules.c				\
	pango-attributes.c			\
	pango-bidi-type.c			\
	pango-color.c				\
	pango-color-table.h			\
	pango-context.c				\
	pango-coverage.c			\
	pango-engine.c				\
	pango-engine-private.h			\
	pango-fontmap.c				\
	pango-fontset.c				\
	pango-glyph-item.c			\
	pango-gravity.c				\
	pango-impl-utils.h			\
	pango-item.c				\
	pango-language.c			\
	pango-language-sample-table.h		\
	pango-layout.c				\
	pango-layout-private.h			\
	pango-markup.c				\
	pango-matrix.c				\
	pango-renderer.c			\
	pango-script.c				\
	pango-script-lang-table.h		\
	pango-script-private.h				\
	pango-tabs.c				\
	pango-utils.c				\
	reorder-items.c				\
	shape.c					\
	pango-enum-types.c			\
	module-defs-lang.c

pango_headers =		   \
	pango.h		   \
	pango-attributes.h \
	pango-bidi-type.h  \
	pango-break.h	   \
	pango-context.h	   \
	pango-coverage.h   \
	pango-engine.h	   \
	pango-font.h       \
	pango-fontmap.h	   \
	pango-fontset.h    \
	pango-glyph.h	   \
	pango-glyph-item.h \
	pango-gravity.h    \
	pango-item.h	   \
	pango-language.h   \
	pango-layout.h	   \
	pango-matrix.h     \
	pango-modules.h	   \
	pango-renderer.h   \
	pango-script.h	   \
	pango-tabs.h	   \
	pango-types.h	   \
	pango-utils.h

pangoinclude_HEADERS =	   \
	$(pango_headers)   \
	pango-features.h   \
	pango-enum-types.h

pango-enum-types.h: s-enum-types-h
s-enum-types-h: $(pango_headers) Makefile.am
	$(AM_V_GEN) ( cd $(srcdir) && $(GLIB_MKENUMS) \
			--fhead "#ifndef __PANGO_ENUM_TYPES_H__\n#define __PANGO_ENUM_TYPES_H__\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void);\n#define @ENUMPREFIX@_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" 	\
			--ftail "G_END_DECLS\n\n#endif /* __PANGO_ENUM_TYPES_H__ */" \
		$(pango_headers) ) > tmp-pango-enum-types.h \
	&& (cmp -s tmp-pango-enum-types.h pango-enum-types.h || cp tmp-pango-enum-types.h pango-enum-types.h ) \
	&& rm -f tmp-pango-enum-types.h	\
	&& echo timestamp > $(@F)

pango-enum-types.c: s-enum-types-c
s-enum-types-c: $(pango_headers) Makefile.am
	$(AM_V_GEN) ( cd $(srcdir) && $(GLIB_MKENUMS) \
			--fhead "#include <pango.h>" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" 	\
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (g_intern_static_string (\"@EnumName@\"), values);\n  }\n  return etype;\n}\n" \
		$(pango_headers) ) > tmp-pango-enum-types.c \
	&& (cmp -s tmp-pango-enum-types.c pango-enum-types.c || cp tmp-pango-enum-types.c pango-enum-types.c ) \
	&& rm -f tmp-pango-enum-types.c \
	&& echo timestamp > $(@F)

pango-features.h:
	$(AM_V_GEN) cd $(top_builddir) && ./config.status pango/pango-features.h

pango_introspection_files = \
        $(libpango_1_0_la_SOURCES) \
	    $(pango_headers) \
	    pango-enum-types.h
Pango-1.0.gir: libpango-1.0.la

Pango_1_0_gir_PACKAGES = gobject-2.0 cairo freetype2
Pango_1_0_gir_EXPORT_PACKAGES = pango
Pango_1_0_gir_INCLUDES = GObject-2.0 cairo-1.0
Pango_1_0_gir_LIBS = libpango-1.0.la
Pango_1_0_gir_CFLAGS = -I$(top_builddir) -I$(top_srcdir) $(PANGO_CFLAGS)
Pango_1_0_gir_FILES = $(filter-out %-private.h pango-color-table.h, $(pango_introspection_files))
Pango_1_0_gir_SCANNERFLAGS = --c-include "pango/pango.h"

# ------------------- libpangox -------------------

if HAVE_X
pangoinclude_HEADERS += $(pangox_headers)
lib_LTLIBRARIES += libpangox-1.0.la
aliasdir = $(sysconfdir)/pango
alias_DATA = pangox.aliases
endif

pangox_headers = pangox.h
pangox_public_sources = pangox.c

libpangox_1_0_la_LDFLAGS = $(LIBRARY_LIBTOOL_OPTIONS)
libpangox_1_0_la_LIBADD = 			\
	libpango-$(PANGO_API_VERSION).la	\
	$(INCLUDED_X_MODULES)			\
	$(GLIB_LIBS)				\
	$(X_LIBS)				\
	$(X_EXTRA_LIBS)
libpangox_1_0_la_DEPENDENCIES =			\
	libpango-$(PANGO_API_VERSION).la	\
	$(INCLUDED_X_MODULES)

if PLATFORM_WIN32
libpangox_1_0_la_LDFLAGS += -export-symbols $(srcdir)/pangox.def -Wl,pangox-win32-res.o
libpangox_1_0_la_DEPENDENCIES += pangox-win32-res.o pangox.def
endif

pangox-win32-res.o: pangox.rc
	$(WINDRES) $< $@

libpangox_1_0_la_SOURCES = 			\
	$(pangox_public_sources) 		\
	modules.h 	   			\
	module-defs-x.c	   			\
	pangox-private.h   			\
	pangox-fontcache.c 			\
	pangox-fontmap.c

# ------------------- libpangoft2 -------------------

if HAVE_FREETYPE
pangoinclude_HEADERS += $(pangoft2_headers)
lib_LTLIBRARIES += libpangoft2-1.0.la
INTROSPECTION_GIRS += PangoFT2-1.0.gir
endif

pangoft2_headers = 		\
	pango-ot.h		\
	pangofc-font.h		\
	pangofc-fontmap.h	\
	pangofc-decoder.h	\
	pangoft2.h

pangoft2_public_sources = 	\
	pangofc-font.c	   	\
	pangofc-fontmap.c	\
	pangofc-decoder.c	\
	pangoft2.c

libpangoft2_1_0_la_LDFLAGS = $(LIBRARY_LIBTOOL_OPTIONS)
libpangoft2_1_0_la_LIBADD =			\
	opentype/libharfbuzz.la			\
	libpango-$(PANGO_API_VERSION).la	\
	$(INCLUDED_FC_MODULES)			\
	$(GLIB_LIBS)				\
	$(FREETYPE_LIBS)			\
	$(libm)
libpangoft2_1_0_la_DEPENDENCIES =		\
	opentype/libharfbuzz.la			\
	libpango-$(PANGO_API_VERSION).la	\
	$(INCLUDED_FC_MODULES)
libpangoft2_1_0_la_SOURCES =   	\
	$(pangoft2_public_sources) \
	modules.h		\
	module-defs-fc.c	\
	pangofc-private.h	\
	pangoft2.h		\
	pangoft2-private.h	\
	pangoft2-fontmap.c	\
	pangoft2-render.c	\
	pango-ot-private.h	\
	pango-ot-buffer.c	\
	pango-ot-info.c		\
	pango-ot-ruleset.c      \
	pango-ot-tag.c


if PLATFORM_WIN32
libpangoft2_1_0_la_LDFLAGS += -export-symbols $(srcdir)/pangoft2.def -Wl,pangoft2-win32-res.o
libpangoft2_1_0_la_DEPENDENCIES += pangoft2-win32-res.o pangoft2.def
endif
pangoft2-win32-res.o: pangoft2.rc
	$(AM_V_GEN) $(WINDRES) $< $@

pangoft2_introspection_files = \
        $(pangoft2_headers) \
        $(pangoft2_public_sources)

PangoFT2-1.0.gir: libpangoft2-1.0.la
PangoFT2-1.0.gir: Pango-1.0.gir

PangoFT2_1_0_gir_PACKAGES = freetype2
PangoFT2_1_0_gir_EXPORT_PACKAGES = pangoft2
PangoFT2_1_0_gir_INCLUDES = GObject-2.0 cairo-1.0 freetype2-2.0 fontconfig-2.0
PangoFT2_1_0_gir_LIBS = libpangoft2-1.0.la
PangoFT2_1_0_gir_CFLAGS = -I$(top_builddir) -I$(top_srcdir)
PangoFT2_1_0_gir_SCANNERFLAGS = --include-uninstalled=$(builddir)/Pango-1.0.gir --c-include "pango/pangoft2.h"
PangoFT2_1_0_gir_FILES = $(filter-out %-private.h, $(addprefix $(srcdir)/,$(pangoft2_introspection_files)))

# ------------------- libpangoxft -------------------

if HAVE_XFT
pangoinclude_HEADERS += $(pangoxft_headers)
lib_LTLIBRARIES += libpangoxft-1.0.la
INTROSPECTION_GIRS += PangoXft-1.0.gir
endif

pangoxft_headers = pangoxft.h pangoxft-render.h

libpangoxft_1_0_la_LDFLAGS = $(LIBRARY_LIBTOOL_OPTIONS)
libpangoxft_1_0_la_LIBADD =			\
	libpangoft2-$(PANGO_API_VERSION).la	\
	libpango-$(PANGO_API_VERSION).la	\
	$(GLIB_LIBS)				\
	$(X_LIBS)				\
	$(XFT_LIBS)				\
	$(FREETYPE_LIBS)			\
	$(libm)
libpangoxft_1_0_la_DEPENDENCIES =		\
	libpangoft2-$(PANGO_API_VERSION).la	\
	libpango-$(PANGO_API_VERSION).la

if PLATFORM_WIN32
libpangoxft_1_0_la_LDFLAGS += -export-symbols $(srcdir)/pangoxft.def -Wl,pangoxft-win32-res.o
libpangoxft_1_0_la_DEPENDENCIES += pangoxft-win32-res.o pangoxft.def
endif
pangoxft-win32-res.o: pangoxft.rc
	$(AM_V_GEN) $(WINDRES) $< $@

libpangoxft_1_0_la_SOURCES =   	\
	pangoxft-font.c		\
	pangoxft-fontmap.c      \
	pangoxft-private.h	\
	pangoxft-render.c

pangoxft_introspection_files = \
    $(pangoxft_headers) \
    $(libpangoxft_1_0_la_SOURCES)

PangoXft-1.0.gir: libpangoxft-1.0.la
PangoXft-1.0.gir: Pango-1.0.gir PangoFT2-1.0.gir

PangoXft_1_0_gir_PACKAGES = gobject-2.0 freetype2
PangoXft_1_0_gir_EXPORT_PACKAGES = pangoxft
PangoXft_1_0_gir_INCLUDES = GObject-2.0 xft-2.0 xlib-2.0
PangoXft_1_0_gir_LIBS = libpangoxft-1.0.la
PangoXft_1_0_gir_CFLAGS = -I$(top_builddir) -I$(top_srcdir) $(PANGO_CFLAGS)
PangoXft_1_0_gir_SCANNERFLAGS = --include-uninstalled=$(builddir)/Pango-1.0.gir --include-uninstalled=$(builddir)/PangoFT2-1.0.gir --c-include "pango/pangoxft.h"
PangoXft_1_0_gir_FILES = $(filter-out %-private.h, $(addprefix $(srcdir)/,$(pangoxft_introspection_files)))

# ------------------- libpangocairo -------------------

if HAVE_CAIRO
pangoinclude_HEADERS += $(pangocairo_headers)
lib_LTLIBRARIES += libpangocairo-1.0.la
INTROSPECTION_GIRS += PangoCairo-1.0.gir
endif

pangocairo_headers = pangocairo.h

libpangocairo_1_0_la_LDFLAGS = $(LIBRARY_LIBTOOL_OPTIONS)
libpangocairo_1_0_la_LIBADD =			\
	libpango-$(PANGO_API_VERSION).la	\
	$(GLIB_LIBS)				\
	$(CAIRO_LIBS)				\
	$(libm)
libpangocairo_1_0_la_DEPENDENCIES =		\
	libpango-$(PANGO_API_VERSION).la
pangocairo_core_sources = \
	pangocairo-context.c    \
	pangocairo-font.c	\
	pangocairo-fontmap.c    \
	pangocairo-render.c	\
	pangocairo-private.h

libpangocairo_1_0_la_SOURCES = $(pangocairo_core_sources)

if HAVE_CAIRO_WIN32
if PLATFORM_WIN32
libpangocairo_1_0_la_LDFLAGS += -export-symbols $(srcdir)/pangocairo.def -Wl,pangocairo-win32-res.o
libpangocairo_1_0_la_DEPENDENCIES += pangocairo-win32-res.o pangocairo.def
endif
pangocairo-win32-res.o: pangocairo.rc
	$(AM_V_GEN) $(WINDRES) $< $@


libpangocairo_1_0_la_LIBADD += libpangowin32-$(PANGO_API_VERSION).la $(WIN32_LIBS)
libpangocairo_1_0_la_DEPENDENCIES += libpangowin32-$(PANGO_API_VERSION).la 

libpangocairo_1_0_la_SOURCES += pangocairo-win32font.c pangocairo-win32fontmap.c pangocairo-win32.h
endif

if HAVE_CAIRO_FREETYPE
libpangocairo_1_0_la_LIBADD +=  libpangoft2-$(PANGO_API_VERSION).la $(FREETYPE_LIBS)
libpangocairo_1_0_la_DEPENDENCIES += libpangoft2-$(PANGO_API_VERSION).la 
PANGOCAIRO_FONT_BACKEND_GI_MODULE = PangoFT2-1.0

libpangocairo_1_0_la_SOURCES += \
	pangocairo-fcfont.c	\
	pangocairo-fcfontmap.c  \
	pangocairo-fc.h
endif

if HAVE_CAIRO_ATSUI
if HAVE_CORE_TEXT
libpangocairo_1_0_la_SOURCES += \
	modules.h		\
	module-defs-coretext.c	\
	pangocoretext.h		\
	pangocoretext.c		\
	pangocoretext-private.h	\
	pangocoretext-fontmap.c	\
	pangocairo-coretext.h	\
	pangocairo-coretextfont.c	\
	pangocairo-coretextfont.h	\
	pangocairo-coretextfontmap.c
pangoinclude_HEADERS += pangocoretext.h

libpangocairo_1_0_la_LDFLAGS += -framework CoreFoundation -framework ApplicationServices
libpangocairo_1_0_la_LIBADD += $(INCLUDED_CORE_TEXT_MODULES)
# We don't have an CoreText GIR right now, so this is just hypothetical
PANGOCAIRO_FONT_BACKEND_GI_MODULE = PangoCoreText-1.0

else
libpangocairo_1_0_la_SOURCES += \
	modules.h		\
	module-defs-atsui.c	\
	pangoatsui.h		\
	pangoatsui.c		\
	pangoatsui-private.h	\
	pangoatsui-fontmap.c	\
	pangocairo-atsui.h	\
	pangocairo-atsuifont.c	\
	pangocairo-atsuifont.h	\
	pangocairo-atsuifontmap.c
pangoinclude_HEADERS += pangoatsui.h

libpangocairo_1_0_la_CFLAGS = -xobjective-c
libpangocairo_1_0_la_LDFLAGS += -framework CoreFoundation -framework Carbon -framework Cocoa
libpangocairo_1_0_la_LIBADD += $(INCLUDED_ATSUI_MODULES)
# We don't have an ATSUI GIR right now, so this is just hypothetical
PANGOCAIRO_FONT_BACKEND_GI_MODULE = PangoATSUI-1.0
endif
endif

pangocairo_introspection_files = \
    $(libpangocairo_1_0_la_SOURCES) \
    $(pangocairo_headers)

PangoCairo-1.0.gir: libpangocairo-1.0.la
PangoCairo-1.0.gir: Pango-1.0.gir $(PANGOCAIRO_FONT_BACKEND_GI_MODULE).gir

PangoCairo_1_0_gir_PACKAGES = gobject-2.0 cairo
PangoCairo_1_0_gir_EXPORT_PACKAGES = pangocairo
PangoCairo_1_0_gir_INCLUDES = GObject-2.0 cairo-1.0
PangoCairo_1_0_gir_LIBS = libpangocairo-1.0.la
PangoCairo_1_0_gir_CFLAGS = -I$(top_builddir) -I$(top_srcdir) $(PANGO_CFLAGS)
PangoCairo_1_0_gir_SCANNERFLAGS = --include-uninstalled=$(builddir)/Pango-1.0.gir --include-uninstalled=$(builddir)/$(PANGOCAIRO_FONT_BACKEND_GI_MODULE).gir --c-include "pango/pangocairo.h"
PangoCairo_1_0_gir_FILES = $(filter-out %-private.h, $(addprefix $(srcdir)/,$(pangocairo_introspection_files)))

# ------------------- libpangowin32 -------------------

if HAVE_WIN32
pangoinclude_HEADERS += pangowin32.h
lib_LTLIBRARIES += libpangowin32-1.0.la
endif

libpangowin32_1_0_la_LDFLAGS = $(LIBRARY_LIBTOOL_OPTIONS)
libpangowin32_1_0_la_LIBADD =			\
	libpango-$(PANGO_API_VERSION).la	\
	$(INCLUDED_WIN32_MODULES)		\
	$(GLIB_LIBS)				\
	$(WIN32_LIBS)
libpangowin32_1_0_la_DEPENDENCIES =		\
	libpango-$(PANGO_API_VERSION).la	\
	$(INCLUDED_WIN32_MODULES)
libpangowin32_1_0_la_SOURCES =	\
	modules.h		\
	module-defs-win32.c	\
	pangowin32.h		\
	pangowin32.c		\
	pangowin32-private.h	\
	pangowin32-fontcache.c	\
	pangowin32-fontmap.c

if PLATFORM_WIN32
libpangowin32_1_0_la_LDFLAGS += -export-symbols $(srcdir)/pangowin32.def -Wl,pangowin32-win32-res.o
libpangowin32_1_0_la_DEPENDENCIES += pangowin32-win32-res.o pangowin32.def
endif
pangowin32-win32-res.o: pangowin32.rc
	$(AM_V_GEN) $(WINDRES) $< $@


# ------------------- win32 build stuff -------------------

if OS_WIN32
install-def-files:
	$(AM_V_GEN) mkdir -p $(DESTDIR)$(libdir) && \
	$(INSTALL) $(srcdir)/pango.def $(DESTDIR)$(libdir)/pango-$(PANGO_API_VERSION).def
if HAVE_FREETYPE
	$(INSTALL) $(srcdir)/pangoft2.def $(DESTDIR)$(libdir)/pangoft2-$(PANGO_API_VERSION).def
endif
	$(INSTALL) $(srcdir)/pangowin32.def $(DESTDIR)$(libdir)/pangowin32-$(PANGO_API_VERSION).def
	$(INSTALL) $(srcdir)/pangocairo.def $(DESTDIR)$(libdir)/pangocairo-$(PANGO_API_VERSION).def
if HAVE_X
	$(INSTALL) $(srcdir)/pangox.def $(DESTDIR)$(libdir)/pangox-$(PANGO_API_VERSION).def
	$(INSTALL) $(srcdir)/pangoxft.def $(DESTDIR)$(libdir)/pangoxft-$(PANGO_API_VERSION).def
endif
uninstall-def-files:
	-rm $(DESTDIR)$(libdir)/pango$(PANGO_API_VERSION).def
if HAVE_FREETYPE
	-rm $(DESTDIR)$(libdir)/pangoft2-$(PANGO_API_VERSION).def
endif
	-rm $(DESTDIR)$(libdir)/pangowin32-$(PANGO_API_VERSION).def
	-rm $(DESTDIR)$(libdir)/pangocairo-$(PANGO_API_VERSION).def
if HAVE_X
	-rm $(DESTDIR)$(libdir)/pangox-$(PANGO_API_VERSION).def
	-rm $(DESTDIR)$(libdir)/pangoxft-$(PANGO_API_VERSION).def
endif
else
install-def-files:
uninstall-def-files:

endif

if MS_LIB_AVAILABLE
MS_LIB_FILES = pango-$(PANGO_API_VERSION).lib pangowin32-$(PANGO_API_VERSION).lib pangocairo-$(PANGO_API_VERSION).lib

if HAVE_FREETYPE
MS_LIB_FILES += pangoft2-$(PANGO_API_VERSION).lib 
endif

noinst_DATA = $(MS_LIB_FILES)

install-ms-lib: $(MS_LIB_FILES)
	$(INSTALL) pango-$(PANGO_API_VERSION).lib $(DESTDIR)$(libdir)
if HAVE_FREETYPE
	$(INSTALL) pangoft2-$(PANGO_API_VERSION).lib $(DESTDIR)$(libdir)
endif
	$(INSTALL) pangowin32-$(PANGO_API_VERSION).lib $(DESTDIR)$(libdir)
	$(INSTALL) pangocairo-$(PANGO_API_VERSION).lib $(DESTDIR)$(libdir)

uninstall-ms-lib:
	-rm $(DESTDIR)$(libdir)/pango-$(PANGO_API_VERSION).lib
	-rm $(DESTDIR)$(libdir)/pangoft2-$(PANGO_API_VERSION).lib
	-rm $(DESTDIR)$(libdir)/pangowin32-$(PANGO_API_VERSION).lib
	-rm $(DESTDIR)$(libdir)/pangocairo-$(PANGO_API_VERSION).lib
else
install-ms-lib:
uninstall-ms-lib:
endif

pango-$(PANGO_API_VERSION).lib: libpango-$(PANGO_API_VERSION).la $(srcdir)/pango.def
	$(AM_V_GEN) lib -machine:$(LIB_EXE_MACHINE_FLAG) -name:libpango-$(PANGO_API_VERSION)-$(PANGO_CURRENT_MINUS_AGE).dll -def:$(srcdir)/pango.def -out:$@

pangoft2-$(PANGO_API_VERSION).lib: libpangoft2-$(PANGO_API_VERSION).la $(srcdir)/pangoft2.def
	$(AM_V_GEN) lib -machine:$(LIB_EXE_MACHINE_FLAG) -name:libpangoft2-$(PANGO_API_VERSION)-$(PANGO_CURRENT_MINUS_AGE).dll -def:$(srcdir)/pangoft2.def -out:$@

pangowin32-$(PANGO_API_VERSION).lib: libpangowin32-$(PANGO_API_VERSION).la $(srcdir)/pangowin32.def
	$(AM_V_GEN) lib -machine:$(LIB_EXE_MACHINE_FLAG) -name:libpangowin32-$(PANGO_API_VERSION)-$(PANGO_CURRENT_MINUS_AGE).dll -def:$(srcdir)/pangowin32.def -out:$@

pangocairo-$(PANGO_API_VERSION).lib: libpangocairo-$(PANGO_API_VERSION).la $(srcdir)/pangocairo.def
	$(AM_V_GEN) lib -machine:$(LIB_EXE_MACHINE_FLAG) -name:libpangocairo-$(PANGO_API_VERSION)-$(PANGO_CURRENT_MINUS_AGE).dll -def:$(srcdir)/pangocairo.def -out:$@


# ------------------- introspection -------------------

if HAVE_INTROSPECTION

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibsdir = $(libdir)/girepository-1.0
typelibs_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

endif # HAVE_INTROSPECTION

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
	module-defs.h				\
	module-defs.h.win32			\
	module-defs-lang.c.win32		\
	module-defs-fc.c.win32			\
	module-defs-win32.c.win32		\
	pango.def				\
	pangocairo.def				\
	pangowin32.def				\
	pangoft2.def				\
	makefile.msc				\
	pango.rc				\
	pango.rc.in				\
	pangox.aliases				\
	pangoft2.rc				\
	pangoft2.rc.in				\
	pangowin32.rc				\
	pangowin32.rc.in			\
	pangocairo.rc				\
	pangocairo.rc.in			\
	pangox.rc				\
	pangox.rc.in				\
	pangoxft.rc				\
	pangoxft.rc.in				\
	check.defs


if HAVE_INCLUDED_MODULES
included-modules:
	$(AM_V_GEN) cd $(top_builddir)/modules && $(MAKE) $(AM_MAKEFLAGS) included-modules

$(INCLUDED_LANG_MODULES) $(INCLUDED_X_MODULES) $(INCLUDED_FC_MODULES) $(INCLUDED_WIN32_MODULES) $(INCLUDED_ATSUI_MODULES): included-modules
	$(AM_V_GEN) true

.PHONY: included-modules
endif

MODULE_DEF_FILES = \
	module-defs.h \
	module-defs-lang.c \
	module-defs-x.c \
	module-defs-fc.c \
	module-defs-win32.c \
	module-defs-atsui.c \
	module-defs-coretext.c

$(MODULE_DEF_FILES): $(top_builddir)/config.status
	$(AM_V_GEN) cd $(top_builddir) && $(SHELL) ./config.status pango/$@

CLEANFILES =			\
	pango-enum-types.h	\
	s-enum-types-h		\
	pango-enum-types.c	\
	s-enum-types-c      \
    $(INTROSPECTION_GIRS) \
    $(INTROSPECTION_GIRS:.gir=.typelib)

MOSTLYCLEANFILES = 		\
	*.list			\
	*.expected

DISTCLEANFILES = 		\
	pango-features.h	\
	$(MODULE_DEF_FILES)

install-data-local: install-ms-lib install-def-files

install-exec-hook: 
if DISABLE_EXPLICIT_DEPS
	-for i in libpangoft2-$(PANGO_API_VERSION).la libpangoxft-$(PANGO_API_VERSION).la libpangox-$(PANGO_API_VERSION).la libpangowin32-$(PANGO_API_VERSION).la; do \
	  test -f $(DESTDIR)$(libdir)/$$i && $(SHELL) $(top_srcdir)/sanitize-la.sh $(DESTDIR)$(libdir)/$$i ; \
	done
endif

uninstall-local: uninstall-ms-lib uninstall-def-files

TESTS = check.defs

dist-hook: \
	../build/win32/vs9/pango.vcproj \
	../build/win32/vs9/pangoft2.vcproj \
	../build/win32/vs9/pangocairo.vcproj \
	../build/win32/vs10/pango.vcxproj \
	../build/win32/vs10/pango.vcxproj.filters \
	../build/win32/vs10/pangocairo.vcxproj \
	../build/win32/vs10/pangocairo.vcxproj.filters \
	../build/win32/vs10/pangocairo_fc.vcxproj \
	../build/win32/vs10/pangocairo_fc.vcxproj.filters \
	../build/win32/vs10/pangoft2.vcxproj \
	../build/win32/vs10/pangoft2.vcxproj.filters

# pangowin32.vcproj, pangowin32.vcxproj and pangowin32.vcxproj.filters are not automatically built, it is not being changed

../build/win32/vs9/pango.vcproj: $(top_srcdir)/build/win32/vs9/pango.vcprojin mini-fribidi/libmini-fribidi.sourcefiles
	-rm $@
	for F in $(libpango_1_0_la_SOURCES); do \
		case $$F in \
		*.c) echo '   <File RelativePath="..\..\..\pango\'$$F'" />' \
		     ;; \
		esac; \
	done >libpango.sourcefiles
	$(CPP) -P - <$(top_srcdir)/build/win32/vs9/pango.vcprojin >$@

../build/win32/vs10/pango.vcxproj: $(top_srcdir)/build/win32/vs10/pango.vcxprojin mini-fribidi/libmini-fribidi.vs10.sourcefiles
	-rm $@
	for F in $(libpango_1_0_la_SOURCES); do \
		case $$F in \
		*.c) echo '   <ClCompile Include="..\..\..\pango\'$$F'" />' \
		     ;; \
		esac; \
	done >libpango.vs10.sourcefiles
	$(CPP) -P - <$(top_srcdir)/build/win32/vs10/pango.vcxprojin >$@

../build/win32/vs10/pango.vcxproj.filters: $(top_srcdir)/build/win32/vs10/pango.vcxproj.filtersin mini-fribidi/libmini-fribidi.vs10.sourcefiles.filters
	-rm $@
	for F in $(libpango_1_0_la_SOURCES); do \
		case $$F in \
		*.c) echo '   <ClCompile Include="..\..\..\pango\'$$F'"><Filter>Source Files</Filter></ClCompile>' \
		     ;; \
		esac; \
	done >libpango.vs10.sourcefiles.filters
	$(CPP) -P - <$(top_srcdir)/build/win32/vs10/pango.vcxproj.filtersin >$@

../build/win32/vs9/pangoft2.vcproj: $(top_srcdir)/build/win32/vs9/pangoft2.vcprojin opentype/libharfbuzz.sourcefiles
	-rm $@
	for F in $(libpangoft2_1_0_la_SOURCES); do \
		case $$F in \
		*.c) echo '   <File RelativePath="..\..\..\pango\'$$F'" />' \
		     ;; \
		esac; \
	done >libpangoft2.sourcefiles
	$(CPP) -P - <$(top_srcdir)/build/win32/vs9/pangoft2.vcprojin >$@

../build/win32/vs10/pangoft2.vcxproj: $(top_srcdir)/build/win32/vs10/pangoft2.vcxprojin opentype/libharfbuzz.vs10.sourcefiles
	-rm $@
	for F in $(libpangoft2_1_0_la_SOURCES); do \
		case $$F in \
		*.c) echo '   <ClCompile Include="..\..\..\pango\'$$F'" />' \
		     ;; \
		esac; \
	done >libpangoft2.vs10.sourcefiles
	$(CPP) -P - <$(top_srcdir)/build/win32/vs10/pangoft2.vcxprojin >$@

../build/win32/vs10/pangoft2.vcxproj.filters: $(top_srcdir)/build/win32/vs10/pangoft2.vcxproj.filtersin opentype/libharfbuzz.vs10.sourcefiles.filters
	-rm $@
	for F in $(libpangoft2_1_0_la_SOURCES); do \
		case $$F in \
		*.c) echo '   <ClCompile Include="..\..\..\pango\'$$F'"><Filter>Source Files</Filter></ClCompile>' \
		     ;; \
		esac; \
	done >libpangoft2.vs10.sourcefiles.filters
	$(CPP) -P - <$(top_srcdir)/build/win32/vs10/pangoft2.vcxproj.filtersin >$@

../build/win32/vs9/pangocairo.vcproj: $(top_srcdir)/build/win32/vs9/pangocairo.vcprojin
# Backend-specific sources for the libpangocairo DLL are explicitly
# listed in the vcprojin file
	-rm $@
	for F in $(pangocairo_core_sources); do \
		case $$F in \
		*.c) echo '   <File RelativePath="..\..\..\pango\'$$F'" />' \
		     ;; \
		esac; \
	done >libpangocairo.sourcefiles
	$(CPP) -P - <$(top_srcdir)/build/win32/vs9/pangocairo.vcprojin >$@

../build/win32/vs10/pangocairo.vcxproj: $(top_srcdir)/build/win32/vs10/pangocairo.vcxprojin
# Backend-specific sources for the libpangocairo DLL are explicitly
# listed in the vcxprojin file
# I know it does not sound nice to have 2 pangocairo projects/filters sets due to FontConfig usage (or not), but VS2010 forces me to do that...
	-rm $@
	for F in $(pangocairo_core_sources); do \
		case $$F in \
		*.c) echo '   <ClCompile Include="..\..\..\pango\'$$F'" />' \
		     ;; \
		esac; \
	done >libpangocairo.vs10.sourcefiles
	$(CPP) -P - <$(top_srcdir)/build/win32/vs10/pangocairo.vcxprojin >$@

../build/win32/vs10/pangocairo.vcxproj.filters: $(top_srcdir)/build/win32/vs10/pangocairo.vcxproj.filtersin
# Backend-specific sources for the libpangocairo DLL are explicitly
# listed in the vcxproj.filtersin file
	-rm $@
	for F in $(pangocairo_core_sources); do \
		case $$F in \
		*.c) echo '   <ClCompile Include="..\..\..\pango\'$$F'"><Filter>Source Files</Filter></ClCompile>' \
		     ;; \
		esac; \
	done >libpangocairo.vs10.sourcefiles.filters
	$(CPP) -P - <$(top_srcdir)/build/win32/vs10/pangocairo.vcxproj.filtersin >$@

../build/win32/vs10/pangocairo_fc.vcxproj: $(top_srcdir)/build/win32/vs10/pangocairo_fc.vcxprojin
# Backend-specific sources for the libpangocairo DLL are explicitly
# listed in the vcxprojin file
	-rm $@
	for F in $(pangocairo_core_sources); do \
		case $$F in \
		*.c) echo '   <ClCompile Include="..\..\..\pango\'$$F'" />' \
		     ;; \
		esac; \
	done >libpangocairo.vs10.sourcefiles
	$(CPP) -P - <$(top_srcdir)/build/win32/vs10/pangocairo_fc.vcxprojin >$@

../build/win32/vs10/pangocairo_fc.vcxproj.filters: $(top_srcdir)/build/win32/vs10/pangocairo_fc.vcxproj.filtersin
# Backend-specific sources for the libpangocairo DLL are explicitly
# listed in the vcxproj.filtersin file
	-rm $@
	for F in $(pangocairo_core_sources); do \
		case $$F in \
		*.c) echo '   <ClCompile Include="..\..\..\pango\'$$F'"><Filter>Source Files</Filter></ClCompile>' \
		     ;; \
		esac; \
	done >libpangocairo.vs10.sourcefiles.filters
	$(CPP) -P - <$(top_srcdir)/build/win32/vs10/pangocairo_fc.vcxproj.filtersin >$@

DISTCLEANFILES += \
	../build/win32/vs9/pango.vcproj \
	../build/win32/vs9/pangoft2.vcproj \
	../build/win32/vs9/pangocairo.vcproj \
	../build/win32/vs10/pango.vcxproj \
	../build/win32/vs10/pango.vcxproj.filters \
	../build/win32/vs10/pangocairo.vcxproj \
	../build/win32/vs10/pangocairo.vcxproj.filters \
	../build/win32/vs10/pangocairo_fc.vcxproj \
	../build/win32/vs10/pangocairo_fc.vcxproj.filters \
	../build/win32/vs10/pangoft2.vcxproj \
	../build/win32/vs10/pangoft2.vcxproj.filters \
	libpangocairo.vs10.sourcefiles.filters \
	libpangocairo.vs10.sourcefiles \
	libpangoft2.vs10.sourcefiles.filters \
	libpangoft2.vs10.sourcefiles \
	libpango.vs10.sourcefiles \
	libpango.vs10.sourcefiles
	

-include $(top_srcdir)/git.mk
