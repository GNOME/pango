## Process this file with automake to create Makefile.in.

INCLUDES =					\
	-I$(top_srcdir)				\
	$(GLIB_CFLAGS)				\
	-DG_DISABLE_DEPRECATED			\
	-DPANGO_DISABLE_DEPRECATED		\
	$(PANGO_DEBUG_FLAGS)

noinst_PROGRAMS = gen-script-for-lang gen-easy-scripts-table

EXTRA_DIST= 			\
	add-copyright 		\
	compress-table.pl 	\
	gen-color-table.pl	\
	gen-script-table.pl	\
	maps/README		\
	maps/tis-620

gen_script_for_lang_SOURCES = gen-script-for-lang.c
gen_script_for_lang_LDADD = ../pango/libpango-$(PANGO_API_VERSION).la

gen_easy_scripts_table_SOURCES = gen-easy-scripts-table.c
gen_easy_scripts_table_LDADD = ../pango/libpango-$(PANGO_API_VERSION).la
