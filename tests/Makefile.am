## Process this file with automake to create Makefile.in.

INCLUDES = -I$(top_srcdir) $(GLIB_CFLAGS)

TESTS=runtests.sh

check_PROGRAMS =	testboundaries

testboundaries_SOURCES = testboundaries.c

## pangox should not actually be required, this is broken
testboundaries_LDADD = ../pango/libpango.la ../pango/libpangox.la $(X_LIBS)

EXTRA_DIST = 			\
	all-unicode.txt		\
	boundaries.utf8		\
	runtests.sh

