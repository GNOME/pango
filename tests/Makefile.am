## Process this file with automake to create Makefile.in.

INCLUDES = -I$(top_srcdir) $(GLIB_CFLAGS)

TESTS=runtests.sh

noinst_PROGRAMS = dump-boundaries

check_PROGRAMS =	testboundaries

testboundaries_SOURCES = testboundaries.c

dump_boundaries_SOURCES = dump-boundaries.c

if OS_WIN32
else
## pangox should not actually be required, this is broken
pangox_la = ../pango/libpangox.la
endif
testboundaries_LDADD = ../pango/libpango.la $(pangox_la) $(X_LIBS)

dump_boundaries_LDADD = ../pango/libpango.la $(pangox_la) $(X_LIBS)

EXTRA_DIST = 			\
	all-unicode.txt		\
	boundaries.utf8		\
	runtests.sh

