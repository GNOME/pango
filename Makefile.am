## Process this file with automake to create Makefile.in.

SUBDIRS=modules pango examples docs tools fonts

bin_SCRIPTS = pango-config

EXTRA_DIST = 		\
	pango.spec.in	\
	pango-config.in	\
	pango.pc.in	\
	pangox.pc.in	\
	pangoxft.pc.in	\
	TODO.xml	\
	README.win32

if HAVE_XFT
XFT_PC=pangoxft.pc
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pango.pc pangox.pc $(XFT_PC)

dist-hook: pango.spec
	cp pango.spec $(distdir)

