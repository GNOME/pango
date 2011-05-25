## Process this file with automake to produce Makefile.in

INCLUDES = 					\
	-DSYSCONFDIR=\"$(sysconfdir)\" 		\
	-DLIBDIR=\"$(libdir)\" 			\
	$(PANGO_DEBUG_FLAGS)			\
	-I$(top_srcdir)				\
	$(GLIB_CFLAGS)

LDADDS = @STRIP_BEGIN@ 	\
	@x_ldflags@	\
	@x_libs@	\
	@GLIB_LIBS@	\
	-lm		\
	@STRIP_END@

noinst_LTLIBRARIES = libmini-fribidi.la

libmini_fribidi_la_SOURCES =    \
	fribidi.c		\
	fribidi_char_type.c 	\
	fribidi_config.h 	\
	fribidi.h		\
	fribidi_types.c 	\
	fribidi_types.h

EXTRA_DIST = 			\
	README	                \
	fribidi_tab_char_type_2.i \
        fribidi_types.i


dist-hook: libmini-fribidi.sourcefiles libmini-fribidi.vs10.sourcefiles libmini-fribidi.vs10.sourcefiles.filters

libmini-fribidi.sourcefiles: Makefile
	for F in $(libmini_fribidi_la_SOURCES); do \
		case $$F in \
		*.c) echo '			<File RelativePath="..\..\..\pango\mini-fribidi\'$$F'" />' \
		     ;; \
		esac; \
	done >libmini-fribidi.sourcefiles

libmini-fribidi.vs10.sourcefiles: Makefile
	for F in $(libmini_fribidi_la_SOURCES); do \
		case $$F in \
		*.c) echo '			<ClCompile Include="..\..\..\pango\mini-fribidi\'$$F'" />' \
		     ;; \
		esac; \
	done >libmini-fribidi.vs10.sourcefiles

libmini-fribidi.vs10.sourcefiles.filters: Makefile
	for F in $(libmini_fribidi_la_SOURCES); do \
		case $$F in \
		*.c) echo '			<ClCompile Include="..\..\..\pango\mini-fribidi\'$$F'"><Filter>Source Files</Filter></ClCompile>' \
		     ;; \
		esac; \
	done >libmini-fribidi.vs10.sourcefiles.filters
	
DISTCLEANFILES = libmini-fribidi.sourcefiles libmini-fribidi.vs10.sourcefiles libmini-fribidi.vs10.sourcefiles.filters

-include $(top_srcdir)/git.mk
