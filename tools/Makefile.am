## Process this file with automake to create Makefile.in.

INCLUDES =					\
	-I$(top_srcdir)				\
	$(GLIB_CFLAGS)				\
	$(FREETYPE_CFLAGS)			\
	-DPANGO_DISABLE_DEPRECATED		\
	$(PANGO_DEBUG_FLAGS)

EXTRA_PROGRAMS = 		\
	gen-script-for-lang	\
	gen-script-for-lang-new

EXTRA_DIST= 			\
	add-copyright 		\
	compress-table.pl 	\
	gen-color-table.pl	\
	maps/README		\
	maps/tis-620

gen_script_for_lang_new_SOURCES = gen-script-for-lang-new.c
gen_script_for_lang_new_LDADD = ../pango/libpangoft2-$(PANGO_API_VERSION).la

gen_script_for_lang_SOURCES = gen-script-for-lang.c
gen_script_for_lang_LDADD = ../pango/libpango-$(PANGO_API_VERSION).la
