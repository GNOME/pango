## Process this file with automake to create Makefile.in.

INCLUDES = -I$(top_srcdir) $(GLIB_CFLAGS)

TESTS=runtests.sh

noinst_PROGRAMS = dump-boundaries

check_PROGRAMS = testboundaries

testboundaries_SOURCES = testboundaries.c

dump_boundaries_SOURCES = dump-boundaries.c

testboundaries_LDADD = ../pango/libpango.la

dump_boundaries_LDADD = ../pango/libpango.la

EXTRA_DIST = 			\
	all-unicode.txt		\
	boundaries.utf8		\
	runtests.sh

