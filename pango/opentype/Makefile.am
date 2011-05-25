# Process this file with automake to produce Makefile.in

NULL =

# The following warning options are useful for debugging: -Wpadded -Wcast-align
#AM_CXXFLAGS = 

noinst_LTLIBRARIES = libharfbuzz.la

HBSOURCES =  \
	hb-blob.c \
	hb-buffer.c \
	hb-buffer-private.h \
	hb-font.cc \
	hb-font-private.h \
	hb-glib.h \
	hb-glib.c \
	hb-private.h \
	hb-shape.h \
	hb-shape.c \
	hb-unicode.c \
	hb-unicode.h \
	hb-unicode-private.h \
	hb-open-file-private.hh \
	hb-open-type-private.hh \
	hb-ot-layout.cc \
	hb-ot-layout-common-private.hh \
	hb-ot-layout-gdef-private.hh \
	hb-ot-layout-gpos-private.hh \
	hb-ot-layout-gsubgpos-private.hh \
	hb-ot-layout-gsub-private.hh \
	hb-ot-layout-private.h \
	hb-object-private.h \
	$(NULL)

HBHEADERS = \
	hb.h \
	hb-blob.h \
	hb-buffer.h \
	hb-common.h \
	hb-font.h \
	hb-ot.h \
	hb-ot-layout.h \
	$(NULL)

libharfbuzz_la_SOURCES = $(HBSOURCES) $(HBHEADERS)
libharfbuzz_la_CPPFLAGS = -DHAVE_GLIB $(GLIB_CFLAGS)
libharfbuzz_la_CXXFLAGS = -fPIC
libharfbuzz_la_LIBADD = $(GLIB_LIBS)

noinst_PROGRAMS = main

main_SOURCES = main.cc
main_CPPFLAGS = -DHAVE_GLIB $(GLIB_CFLAGS)
main_LDADD = libharfbuzz.la $(GLIB_LIBS)

EXTRA_DIST = README COPYING

dist-hook: libharfbuzz.sourcefiles libharfbuzz.vs10.sourcefiles libharfbuzz.vs10.sourcefiles.filters

libharfbuzz.sourcefiles: Makefile
	for F in $(libharfbuzz_la_SOURCES); do \
		case $$F in \
		*.c|*.cc) echo '   <File RelativePath="..\..\..\pango\opentype\'$$F'" />' \
		     ;; \
		esac; \
	done >libharfbuzz.sourcefiles

libharfbuzz.vs10.sourcefiles: Makefile
	for F in $(libharfbuzz_la_SOURCES); do \
		case $$F in \
		*.c|*.cc) echo '   <ClCompile Include="..\..\..\pango\opentype\'$$F'" />' \
		     ;; \
		esac; \
	done >libharfbuzz.vs10.sourcefiles

libharfbuzz.vs10.sourcefiles.filters: Makefile
	for F in $(libharfbuzz_la_SOURCES); do \
		case $$F in \
		*.c|*.cc) echo '   <ClCompile Include="..\..\..\pango\opentype\'$$F'"><Filter>Source Files</Filter></ClCompile>' \
		     ;; \
		esac; \
	done >libharfbuzz.vs10.sourcefiles.filters
	
DISTCLEANFILES = libharfbuzz.sourcefiles libharfbuzz.vs10.sourcefiles libharfbuzz.vs10.sourcefiles.filters

-include $(top_srcdir)/git.mk
