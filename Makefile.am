## Process this file with automake to create Makefile.in.

SUBDIRS=modules pango examples docs tools fonts

EXTRA_DIST = 		\
	pango.spec.in	\
	pango.pc.in	\
	pangox.pc.in	\
	pangoxft.pc.in	\
	pangoft2.pc.in	\
	TODO.xml	\
	README.win32

if HAVE_X
X_PC=pangox.pc
endif

if HAVE_XFT
XFT_PC=pangoxft.pc
endif

if HAVE_FREETYPE
FT2_PC=pangoft2.pc
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pango.pc $(X_PC) $(XFT_PC) $(FT2_PC)

dist-hook: pango.spec
	cp pango.spec $(distdir)

