## Process this file with automake to create Makefile.in.

INCLUDES =					\
	-I$(top_srcdir)				\
	$(GLIB_CFLAGS)				\
	$(XFT_CFLAGS)				\
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

TESTS_ENVIRONMENT =                    \
   srcdir=$(srcdir)                    \
   PANGO_RC_FILE=./pangorc

noinst_PROGRAMS = gen-all-unicode dump-boundaries

check_PROGRAMS = testboundaries testcolor testscript

gen_all_unicode_SOURCES = gen-all-unicode.c

testboundaries_SOURCES = testboundaries.c

testcolor_SOURCES = testcolor.c

testscript_SOURCES = testscript.c

dump_boundaries_SOURCES = dump-boundaries.c

gen_all_unicode_LDADD = $(GLIB_LIBS)

testboundaries_LDADD = ../pango/libpango-$(PANGO_API_VERSION).la

testcolor_LDADD = ../pango/libpango-$(PANGO_API_VERSION).la

testscript_LDADD = ../pango/libpango-$(PANGO_API_VERSION).la

dump_boundaries_LDADD = ../pango/libpango-$(PANGO_API_VERSION).la

if HAVE_CXX
check_PROGRAMS += cxx-test
endif
cxx_test_SOURCES = cxx-test.C

if CROSS_COMPILING
all-local: all-unicode.txt
else
all-local: all-unicode.txt pango.modules
endif

pango.modules:
	if find ../modules -name '*.so' | grep 'so' > /dev/null ; then \
          echo "Writing a pango.modules file to use when running tests before installing Pango."; \
	  (cd ../modules && \
           ../pango/pango-querymodules `find . -name '*.la' | grep -v \.libs` > ../tests/pango.modules ) ;\
	else \
          echo "No dynamic modules found; will use only static modules for uninstalled test programs."; \
	  touch pango.modules; \
	fi

EXTRA_DIST = 			\
	all-unicode.txt		\
	boundaries.utf8		\
	runtests.sh

all-unicode.txt: gen-all-unicode
	./gen-all-unicode > all-unicode.txt

CLEANFILES = pango.modules
DISTCLEANFILES = all-unicode.txt runtests.log
