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

