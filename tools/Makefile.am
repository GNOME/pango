## Process this file with automake to create Makefile.in.

INCLUDES =					\
	-I$(top_srcdir)				\
	$(GLIB_CFLAGS)				\
	-DPANGO_DISABLE_DEPRECATED		\
	$(PANGO_DEBUG_FLAGS)

noinst_PROGRAMS = gen-script-for-lang

EXTRA_DIST= 			\
	add-copyright 		\
	compress-table.pl 	\
	gen-color-table.pl	\
	gen-script-table.pl	\
	maps/README		\
	maps/tis-620

gen_script_for_lang_SOURCES = gen-script-for-lang.c
gen_script_for_lang_LDADD = ../pango/libpango-$(PANGO_API_VERSION).la
