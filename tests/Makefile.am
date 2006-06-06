## Process this file with automake to create Makefile.in.

EXTRA_DIST = 			\
	all-unicode.txt		\
	boundaries.utf8		\
	runtests.sh

CLEANFILES = pangorc
DISTCLEANFILES = all-unicode.txt runtests.log

INCLUDES =					\
	-I$(top_srcdir)				\
	$(PANGO_DEBUG_FLAGS)			\
	$(GLIB_CFLAGS)				\
	$(XFT_CFLAGS)				\
	$(CAIRO_CFLAGS)				\
	$(FREETYPE_CFLAGS)			\
	$(X_CFLAGS)

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

TESTS_ENVIRONMENT =                    \
   srcdir=$(srcdir)                    \
   PANGO_RC_FILE=./pangorc

noinst_PROGRAMS = gen-all-unicode dump-boundaries 

check_PROGRAMS = testboundaries testcolor testscript

if HAVE_CAIRO
check_PROGRAMS += testiter
endif

gen_all_unicode_SOURCES = gen-all-unicode.c

testboundaries_SOURCES = testboundaries.c

testcolor_SOURCES = testcolor.c

testiter_SOURCES = testiter.c

testscript_SOURCES = testscript.c


dump_boundaries_SOURCES = dump-boundaries.c

gen_all_unicode_LDADD = $(GLIB_LIBS)

testboundaries_LDADD = ../pango/libpango-$(PANGO_API_VERSION).la

testcolor_LDADD = ../pango/libpango-$(PANGO_API_VERSION).la

testiter_LDADD = ../pango/libpango-$(PANGO_API_VERSION).la ../pango/libpangocairo-$(PANGO_API_VERSION).la 

testscript_LDADD = ../pango/libpango-$(PANGO_API_VERSION).la

dump_boundaries_LDADD = ../pango/libpango-$(PANGO_API_VERSION).la

if HAVE_CXX
check_PROGRAMS += cxx-test
endif
cxx_test_SOURCES = cxx-test.C

all-unicode.txt: gen-all-unicode$(EXEEXT)
	./gen-all-unicode > all-unicode.txt

pangorc: $(srcdir)/../modules/pangorc
	cp $< $@

$(noinst_PROGRAMS) $(check_PROGRAMS): pangorc
