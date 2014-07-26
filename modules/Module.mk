INCLUDES = 				\
	-DG_LOG_DOMAIN=\"Pango\"	\
	-DPANGO_ENABLE_ENGINE		\
	$(PANGO_DEBUG_FLAGS)		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/pango		\
	$(GLIB_CFLAGS)

noinst_LTLIBRARIES =


included-modules: $(noinst_LTLIBRARIES)

.PHONY: included-modules
