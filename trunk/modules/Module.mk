pangolibs =								\
	$(GLIB_LIBS)							\
	$(top_builddir)/pango/libpango-$(PANGO_API_VERSION).la
pangoxlibs =								\
	$(pangolibs)							\
	$(top_builddir)/pango/libpangox-$(PANGO_API_VERSION).la		\
	$(X_LIBS)
pangoft2libs =								\
	$(pangolibs)							\
	$(top_builddir)/pango/libpangoft2-$(PANGO_API_VERSION).la	\
	$(FREETYPE_LIBS)
pangowin32libs =							\
	$(pangolibs)							\
	$(top_builddir)/pango/libpangowin32-$(PANGO_API_VERSION).la
pangoatsuilibs =							\
	$(pangolibs)							\
	$(top_builddir)/pango/libpangocairo-$(PANGO_API_VERSION).la

INCLUDES = 				\
	-DG_LOG_DOMAIN=\"Pango\"	\
	-DPANGO_ENABLE_ENGINE		\
	$(PANGO_DEBUG_FLAGS)		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/pango		\
	$(GLIB_CFLAGS)

moduledir = $(libdir)/pango/$(PANGO_MODULE_VERSION)/modules
module_LTLIBRARIES =
noinst_LTLIBRARIES =


included-modules: $(noinst_LTLIBRARIES)
dynamic-modules: $(module_LTLIBRARIES)

.PHONY: included-modules dynamic-modules
