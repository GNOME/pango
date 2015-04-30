## Process this file with automake to create Makefile.in.

INCLUDES =					\
	-I$(top_srcdir)				\
	$(GLIB_CFLAGS)				\
	$(FONTCONFIG_CFLAGS)			\
	-DPANGO_DISABLE_DEPRECATED		\
	$(PANGO_DEBUG_FLAGS)

EXTRA_PROGRAMS = 		\
	gen-script-for-lang

EXTRA_DIST= 			\
	gen-color-table.pl	\
	rgb.txt

gen_script_for_lang_SOURCES = gen-script-for-lang.c
gen_script_for_lang_LDADD = ../pango/libpango-$(PANGO_API_VERSION).la $(FONTCONFIG_LIBS) $(GLIB_LIBS)

-include $(top_srcdir)/git.mk
