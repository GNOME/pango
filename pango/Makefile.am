## Process this file with automake to create Makefile.in.

# We set GPATH here; this gives us semantics for GNU make
# which are more like other make's VPATH.
GPATH = $(srcdir)

include $(INTROSPECTION_MAKEFILE)

INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --warn-all --add-include-path=$(srcdir) --add-include-path=.
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir) --includedir=.
INTROSPECTION_SCANNER_ENV = CC="$(CC)"

SUBDIRS = mini-fribidi

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
	$(HARFBUZZ_CFLAGS)				\
	$(FREETYPE_CFLAGS)				\
	$(FONTCONFIG_CFLAGS)				\
	$(LIBTHAI_CFLAGS)					\
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
	$(LIBTHAI_LIBS)			\
	$(libm)

libpango_1_0_la_LIBADD += mini-fribidi/libmini-fribidi.la
libpango_1_0_la_DEPENDENCIES = mini-fribidi/libmini-fribidi.la

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
	pango-script-private.h			\
	pango-tabs.c				\
	pango-utils.c				\
	reorder-items.c				\
	shape.c					\
	pango-enum-types.c

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
s-enum-types-h: $(pango_headers) Makefile.am pango-enum-types.h.template
	$(AM_V_GEN) ( cd $(srcdir) && $(GLIB_MKENUMS) \
			 --template pango-enum-types.h.template \
			$(pango_headers) ) > pango-enum-types.h.tmp \
			&& mv pango-enum-types.h.tmp pango-enum-types.h \
			&& echo timestamp > $@

pango-enum-types.c: s-enum-types-c
s-enum-types-c: $(pango_headers) Makefile.am pango-enum-types.c.template
	$(AM_V_GEN) ( cd $(srcdir) && $(GLIB_MKENUMS) \
			 --template pango-enum-types.c.template \
			$(pango_headers) ) > pango-enum-types.c.tmp \
			&& mv pango-enum-types.c.tmp pango-enum-types.c \
			&& echo timestamp > $@

pango-features.h:
	$(AM_V_GEN) cd $(top_builddir) && ./config.status pango/pango-features.h

pango_introspection_files = \
        $(libpango_1_0_la_SOURCES) \
	    $(pango_headers) \
	    pango-enum-types.h
Pango-1.0.gir: libpango-1.0.la

Pango_1_0_gir_PACKAGES = gobject-2.0 cairo glib-2.0
if HAVE_CAIRO_FREETYPE
Pango_1_0_gir_PACKAGES += freetype2
endif
Pango_1_0_gir_EXPORT_PACKAGES = pango
Pango_1_0_gir_INCLUDES = GObject-2.0 cairo-1.0
Pango_1_0_gir_LIBS = libpango-1.0.la
Pango_1_0_gir_CFLAGS = -I$(top_builddir) -I$(top_srcdir) $(INCLUDES)
Pango_1_0_gir_FILES = $(filter-out %-private.h pango-color-table.h, $(pango_introspection_files))
Pango_1_0_gir_SCANNERFLAGS = --c-include "pango/pango.h"

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
	pangofc-shape.c		\
	pangoft2.c

libpangoft2_1_0_la_LDFLAGS = $(LIBRARY_LIBTOOL_OPTIONS)
libpangoft2_1_0_la_LIBADD =			\
	libpango-$(PANGO_API_VERSION).la	\
	$(GLIB_LIBS)				\
	$(HARFBUZZ_LIBS)			\
	$(FREETYPE_LIBS)			\
	$(FONTCONFIG_LIBS)			\
	$(libm)
libpangoft2_1_0_la_DEPENDENCIES =		\
	libpango-$(PANGO_API_VERSION).la
libpangoft2_1_0_la_SOURCES =   	\
	$(pangoft2_public_sources) \
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
PangoFT2_1_0_gir_LIBS = $(Pango_1_0_gir_LIBS) libpangoft2-1.0.la
PangoFT2_1_0_gir_CFLAGS = -I$(top_builddir) -I$(top_srcdir) $(INCLUDES)
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
	$(XFT_LIBS)				\
	$(FREETYPE_LIBS)			\
	$(FONTCONFIG_LIBS)			\
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
PangoXft_1_0_gir_LIBS = $(PangoFT2_1_0_gir_LIBS) libpangoxft-1.0.la
PangoXft_1_0_gir_CFLAGS = -I$(top_builddir) -I$(top_srcdir) $(INCLUDES)
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
	$(FONTCONFIG_LIBS)			\
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


libpangocairo_1_0_la_LIBADD += libpangowin32-$(PANGO_API_VERSION).la -lgdi32
libpangocairo_1_0_la_DEPENDENCIES += libpangowin32-$(PANGO_API_VERSION).la

libpangocairo_1_0_la_SOURCES += pangocairo-win32font.c pangocairo-win32fontmap.c pangocairo-win32.h
endif

if HAVE_CAIRO_FREETYPE
libpangocairo_1_0_la_LIBADD +=  libpangoft2-$(PANGO_API_VERSION).la $(FREETYPE_LIBS)
libpangocairo_1_0_la_DEPENDENCIES += libpangoft2-$(PANGO_API_VERSION).la 
PANGOCAIRO_FONT_BACKEND_GI_MODULE = PangoFT2-1.0.gir

libpangocairo_1_0_la_SOURCES += \
	pangocairo-fcfont.c	\
	pangocairo-fcfontmap.c  \
	pangocairo-fc.h
endif

if HAVE_CAIRO_QUARTZ
if HAVE_CORE_TEXT
libpangocairo_1_0_la_SOURCES += \
	pangocoretext.h		\
	pangocoretext.c		\
	pangocoretext-private.h	\
	pangocoretext-fontmap.c	\
	pangocoretext-shape.c	\
	pangocairo-coretext.h	\
	pangocairo-coretextfont.c	\
	pangocairo-coretextfont.h	\
	pangocairo-coretextfontmap.c
pangoinclude_HEADERS += pangocoretext.h

libpangocairo_1_0_la_LDFLAGS += -framework CoreFoundation -framework ApplicationServices
# We don't have an CoreText GIR right now, so this is just hypothetical
# PANGOCAIRO_FONT_BACKEND_GI_MODULE = PangoCoreText-1.0.gir
endif
endif

pangocairo_introspection_files = \
    $(libpangocairo_1_0_la_SOURCES) \
    $(pangocairo_headers)

PangoCairo-1.0.gir: libpangocairo-1.0.la
PangoCairo-1.0.gir: Pango-1.0.gir $(PANGOCAIRO_FONT_BACKEND_GI_MODULE)

PangoCairo_1_0_gir_PACKAGES = gobject-2.0 cairo
PangoCairo_1_0_gir_EXPORT_PACKAGES = pangocairo
PangoCairo_1_0_gir_INCLUDES = GObject-2.0 cairo-1.0
PangoCairo_1_0_gir_LIBS = $(Pango_1_0_gir_LIBS) libpangocairo-1.0.la
PangoCairo_1_0_gir_CFLAGS = -I$(top_builddir) -I$(top_srcdir) $(INCLUDES)
PangoCairo_1_0_gir_SCANNERFLAGS = --include-uninstalled=$(builddir)/Pango-1.0.gir --c-include "pango/pangocairo.h"
if HAVE_CAIRO_FREETYPE
PangoCairo_1_0_gir_SCANNERFLAGS += --include-uninstalled=$(builddir)/$(PANGOCAIRO_FONT_BACKEND_GI_MODULE)
PangoCairo_1_0_gir_LIBS += $(PangoFT2_1_0_gir_LIBS)
endif
PangoCairo_1_0_gir_FILES = $(filter-out %-private.h, $(addprefix $(srcdir)/,$(pangocairo_introspection_files)))

# ------------------- libpangowin32 -------------------

if HAVE_WIN32
pangoinclude_HEADERS += pangowin32.h
lib_LTLIBRARIES += libpangowin32-1.0.la
endif

libpangowin32_1_0_la_LDFLAGS = $(LIBRARY_LIBTOOL_OPTIONS)
libpangowin32_1_0_la_LIBADD =			\
	libpango-$(PANGO_API_VERSION).la	\
	$(GLIB_LIBS)				\
	-lgdi32 -lusp10
libpangowin32_1_0_la_DEPENDENCIES =		\
	libpango-$(PANGO_API_VERSION).la
libpangowin32_1_0_la_SOURCES =	\
	pangowin32.h		\
	pangowin32.c		\
	pangowin32-private.h	\
	pangowin32-fontcache.c	\
	pangowin32-fontmap.c	\
	pangowin32-shape.c

if PLATFORM_WIN32
libpangowin32_1_0_la_LDFLAGS += -export-symbols $(srcdir)/pangowin32.def -Wl,pangowin32-win32-res.o
libpangowin32_1_0_la_DEPENDENCIES += pangowin32-win32-res.o pangowin32.def
endif
pangowin32-win32-res.o: pangowin32.rc
	$(AM_V_GEN) $(WINDRES) $< $@


# ------------------- introspection -------------------

if HAVE_INTROSPECTION

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibsdir = $(libdir)/girepository-1.0
typelibs_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

endif # HAVE_INTROSPECTION


EXTRA_DIST =					\
	pango-enum-types.h			\
	pango-enum-types.h.template		\
	pango-enum-types.c.template		\
	pango.def				\
	pangocairo.def				\
	pangowin32.def				\
	pangoft2.def				\
	pangoxft.def				\
	pango.rc				\
	pango.rc.in				\
	pangoft2.rc				\
	pangoft2.rc.in				\
	pangowin32.rc				\
	pangowin32.rc.in			\
	pangocairo.rc				\
	pangocairo.rc.in			\
	pangoxft.rc				\
	pangoxft.rc.in				\
	break-arabic.c				\
	break-indic.c				\
	break-thai.c				\
	check.defs

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
	pango-features.h

# ------------------- MSVC Build Items ----------------

extra_fribidi_srcs_for_msvc =	\
	mini-fribidi/fribidi.c			\
	mini-fribidi/fribidi_char_type.c	\
	mini-fribidi/fribidi_types.c

MSVCPROJS = pango pangowin32 pangoft2 pangocairo

pango_FILES =	\
	$(extra_fribidi_srcs_for_msvc)	\
	$(libpango_1_0_la_SOURCES)
pango_EXCLUDES = dummy
pango_HEADERS_DIR = $(pangoincludedir)

# We can't just use $(pangoinclude_HEADERS), due to different platforms this may run on.
# We don't include the pangoft2 headers here, as it is optional on Windows Builds.
pango_HEADERS_INST = 	\
	$(pango_headers)	\
	$(pangocairo_headers)	\
	pangowin32.h		\
	pango-features.h	\
	pango-enum-types.h

pango_HEADERS_EXCLUDES = dummy

pangowin32_FILES = $(libpangowin32_1_0_la_SOURCES)
pangowin32_EXCLUDES = dummy

pangoft2_FILES = $(libpangoft2_1_0_la_SOURCES)
pangoft2_EXCLUDES = dummy

# Again, can't use $(libpangocairo_1_0_la_SOURCES), due to different platforms this may run on.
pangocairo_FILES = $(pangocairo_core_sources)
pangocairo_EXCLUDES = dummy

include $(top_srcdir)/build/Makefile.msvcproj

dist-hook: \
	$(top_builddir)/build/win32/vs9/pango.vcproj		\
	$(top_builddir)/build/win32/vs9/pangowin32.vcproj	\
	$(top_builddir)/build/win32/vs9/pangoft2.vcproj		\
	$(top_builddir)/build/win32/vs9/pangocairo.vcproj	\
	$(top_builddir)/build/win32/vs9/pango.headers

TESTS = check.defs

-include $(top_srcdir)/git.mk
