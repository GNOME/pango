## Process this file with automake to produce Makefile.in

INCLUDES = 					\
	-DSYSCONFDIR=\"$(sysconfdir)\" 		\
	-DLIBDIR=\"$(libdir)\" 			\
	-DG_DISABLE_DEPRECATED			\
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
	fribidi_types.h		\
	fribidi_get_type.c	\
	fribidi.c

EXTRA_DIST = 			\
	README			\
	fribidi_tables.i	\
	fribidi.patch
