## Process this file with automake to create Makefile.in.

INCLUDES =					\
	-I$(top_srcdir)				\
	$(GLIB_CFLAGS)				\
	$(XFT_CFLAGS)				\
	$(FONTCONFIG_CFLAGS)			\
	$(FREETYPE_CFLAGS)			\
	$(X_CFLAGS)				\
	-DG_DISABLE_DEPRECATED			\
	$(PANGO_DEBUG_FLAGS)

if HAVE_XFT
INCLUDES += -DHAVE_X
endif
if HAVE_XFT
INCLUDES += -DHAVE_XFT
endif
if HAVE_FREETYPE
INCLUDES += -DHAVE_FREETYPE
endif
if HAVE_WIN32
INCLUDES += -DHAVE_WIN32
endif


TESTS=runtests.sh

noinst_PROGRAMS = gen-all-unicode dump-boundaries

check_PROGRAMS = testboundaries testcolor

gen_all_unicode_SOURCES = gen-all-unicode.c

testboundaries_SOURCES = testboundaries.c

testcolor_SOURCES = testcolor.c

dump_boundaries_SOURCES = dump-boundaries.c

gen_all_unicode_LDADD = $(GLIB_LIBS)

testboundaries_LDADD = ../pango/libpango-$(PANGO_API_VERSION).la

testcolor_LDADD = ../pango/libpango-$(PANGO_API_VERSION).la

dump_boundaries_LDADD = ../pango/libpango-$(PANGO_API_VERSION).la

if HAVE_CXX
check_PROGRAMS += cxx-test
endif
cxx_test_SOURCES = cxx-test.C


EXTRA_DIST = 			\
	all-unicode.txt		\
	boundaries.utf8		\
	runtests.sh

all-unicode.txt: gen-all-unicode
	./gen-all-unicode > all-unicode.txt

all-local: all-unicode.txt

DISTCLEANFILES = all-unicode.txt runtests.log
