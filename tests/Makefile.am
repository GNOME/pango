## Process this file with automake to create Makefile.in.

INCLUDES = -I$(top_srcdir) $(GLIB_CFLAGS)

TESTS=runtests.sh

noinst_PROGRAMS = gen-all-unicode dump-boundaries

check_PROGRAMS = testboundaries

gen_all_unicode_SOURCES = gen-all-unicode.c

testboundaries_SOURCES = testboundaries.c

dump_boundaries_SOURCES = dump-boundaries.c

gen_all_unicode_LDADD = $(GLIB_LIBS)

testboundaries_LDADD = ../pango/libpango.la

dump_boundaries_LDADD = ../pango/libpango.la

EXTRA_DIST = 			\
	all-unicode.txt		\
	boundaries.utf8		\
	runtests.sh

all-unicode.txt: gen-all-unicode
	./gen-all-unicode > all-unicode.txt

all-local: all-unicode.txt
