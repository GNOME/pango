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


if CROSS_COMPILING
TESTS=
else
TESTS=runtests.sh
endif

TESTS_ENVIRONMENT =                    \
   srcdir=$(srcdir)                    \
   PANGO_RC_FILE=./pangorc

noinst_PROGRAMS = gen-all-unicode dump-boundaries 

check_PROGRAMS = testboundaries testboundaries_ucd testcolor testscript

if HAVE_CAIRO
check_PROGRAMS += testiter
endif
if HAVE_FREETYPE
check_PROGRAMS += test-ot-tags
endif

TEST_PANGO_LIBS = $(GLIB_LIBS) ../pango/libpango-$(PANGO_API_VERSION).la
TEST_PANGOFT2_LIBS = $(TEST_PANGO_LIBS) ../pango/libpangoft2-$(PANGO_API_VERSION).la
TEST_PANGOCAIRO_LIBS = $(TEST_PANGO_LIBS) ../pango/libpangocairo-$(PANGO_API_VERSION).la

gen_all_unicode_LDADD = $(GLIB_LIBS)
testboundaries_LDADD = $(TEST_PANGO_LIBS)
testboundaries_ucd_LDADD = $(TEST_PANGO_LIBS)
testcolor_LDADD = $(TEST_PANGO_LIBS)
testiter_LDADD = $(TEST_PANGOCAIRO_LIBS)
testscript_LDADD = $(TEST_PANGO_LIBS)
test_ot_tags_LDADD = $(TEST_PANGOFT2_LIBS)
dump_boundaries_LDADD = $(TEST_PANGO_LIBS)

if HAVE_CXX
check_PROGRAMS += cxx-test
endif
cxx_test_SOURCES = cxx-test.C

all-unicode.txt: gen-all-unicode$(EXEEXT)
	./gen-all-unicode > all-unicode.txt

pangorc: $(srcdir)/../modules/pangorc
	cp $(srcdir)/../modules/pangorc $@

$(noinst_PROGRAMS) $(check_PROGRAMS): pangorc

-include $(top_srcdir)/git.mk
