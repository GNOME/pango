## Process this file with automake to create Makefile.in.

AUTOMAKE_OPTIONS = 1.7

SUBDIRS= pango modules examples docs tools tests

EXTRA_DIST = 			\
	pango.pc.in		\
	pangocairo.pc.in	\
	pangox.pc.in		\
	pangoxft.pc.in		\
	pangoft2.pc.in		\
	pangowin32.pc.in	\
	pango-uninstalled.pc.in		\
	pangocairo-uninstalled.pc.in	\
	pangox-uninstalled.pc.in	\
	pangoxft-uninstalled.pc.in	\
	pangoft2-uninstalled.pc.in	\
	pangowin32-uninstalled.pc.in	\
	TODO.xml	\
	README.win32	\
	pango-zip.sh	\
	sanitize-la.sh

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pango.pc

if HAVE_CAIRO
pkgconfig_DATA += pangocairo.pc
endif

if HAVE_X
pkgconfig_DATA += pangox.pc
endif

if HAVE_XFT
pkgconfig_DATA += pangoxft.pc
endif

if HAVE_FREETYPE
pkgconfig_DATA += pangoft2.pc
endif

if HAVE_WIN32
pkgconfig_DATA += pangowin32.pc
endif

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc
