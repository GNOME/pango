## Process this file with automake to create Makefile.in.

SUBDIRS=modules pango examples docs tools fonts

bin_SCRIPTS = pango-config

EXTRA_DIST = 		\
	pango.spec.in	\
	pango-config.in

dist-hook: pango.spec
	cp pango.spec $(distdir)

