## Process this file with automake to produce Makefile.in

INCLUDES = 					\
	-DSYSCONFDIR=\"$(sysconfdir)\" 		\
	-DLIBDIR=\"$(libdir)\" 			\
	-I$(top_srcdir)

LDADDS = @STRIP_BEGIN@ 	\
	@x_ldflags@	\
	@x_libs@	\
	@GLIB_LIBS@	\
	-lm		\
@STRIP_END@

noinst_LTLIBRARIES = libmini-fribidi.la

libmini_fribidi_la_SOURCES =    \
	fribidi_types.h		\
	fribidi_get_type.c	\
	fribidi.c

EXTRA_DIST = 			\
	README			\
	fribidi_tables.i	\
	fribidi-0.1.12.patch
