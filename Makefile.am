## Process this file with automake to create Makefile.in.

SUBDIRS=libpango modules examples docs tools fonts

EXTRA_DIST = \
	pango.spec.in

dist-hook: pango.spec
	cp pango.spec $(distdir)

