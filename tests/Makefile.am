## Process this file with automake to create Makefile.in.

if HAVE_XFT
X_DEFINES=-DHAVE_X
else
X_DEFINES=
endif

if HAVE_XFT
XFT_DEFINES=-DHAVE_XFT
else
XFT_DEFINES=
endif

if HAVE_FREETYPE
FT2_DEFINES=-DHAVE_FREETYPE
else
FT2_DEFINES=
endif

if HAVE_WIN32
WIN32_DEFINES=-DHAVE_WIN32
else
WIN32_DEFINES=
endif

INCLUDES =					\
	-I$(top_srcdir)				\
	$(GLIB_CFLAGS)				\
	$(X_CFLAGS)				\
	$(FREETYPE_CFLAGS)			\
	$(X_DEFINES)				\
	$(XFT_DEFINES)				\
	$(FT2_DEFINES)				\
	$(WIN32_DEFINES)			\
	-DG_DISABLE_DEPRECATED

TESTS=runtests.sh

noinst_PROGRAMS = gen-all-unicode dump-boundaries

check_PROGRAMS = testboundaries $(CXX_TEST)

gen_all_unicode_SOURCES = gen-all-unicode.c

testboundaries_SOURCES = testboundaries.c

dump_boundaries_SOURCES = dump-boundaries.c

gen_all_unicode_LDADD = $(GLIB_LIBS)

testboundaries_LDADD = ../pango/libpango.la

dump_boundaries_LDADD = ../pango/libpango.la

if HAVE_CXX
CXX_TEST = cxx-test
cxx_test_SOURCES = cxx-test.C
else
CXX_TEST =
endif

EXTRA_DIST = 			\
	all-unicode.txt		\
	boundaries.utf8		\
	runtests.sh

all-unicode.txt: gen-all-unicode
	./gen-all-unicode > all-unicode.txt

all-local: all-unicode.txt
