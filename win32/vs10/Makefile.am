GENERATED_ITEMS =			\
	pango.vcxproj			\
	pango.vcxproj.filters		\
	pangowin32.vcxproj		\
	pangowin32.vcxproj.filters	\
	pangoft2.vcxproj		\
	pangoft2.vcxproj.filters	\
	pangocairo.vcxproj		\
	pangocairo.vcxproj.filters	\
	pango-install.props

EXTRA_DIST =	\
	pango.sln				\
	pango-build-defines.props		\
	pango-ignore-fc.props			\
	pango-install-bin.props			\
	pango-install.propsin			\
	pango-prebuild.props			\
	pango-version-paths.props		\
	pango.vcxprojin				\
	pango.vcxproj.filtersin			\
	pangoft2.vcxprojin			\
	pangoft2.vcxproj.filtersin		\
	pangowin32.vcxprojin			\
	pangowin32.vcxproj.filtersin		\
	pangocairo.vcxprojin			\
	pangocairo.vcxproj.filtersin		\
	pango-install.vcxproj			\
	pango-install.vcxproj.filters		\
	pango-introspect.vcxproj		\
	README.txt				\
	$(GENERATED_ITEMS)

pango-install.props: $(top_srcdir)/win32/vs10/pango-install.propsin pango.vs10.headers
	-$(RM) $(top_builddir)/win32/vs11/pango-install.props
	-$(RM) $(top_builddir)/win32/vs12/pango-install.props
	-$(RM) $(top_builddir)/win32/vs14/pango-install.props
	-$(RM) $(top_builddir)/win32/vs15/pango-install.props
	$(CPP) -P - <$(top_srcdir)/win32/vs10/pango-install.propsin >$@
	rm pango.vs10.headers

DISTCLEANFILES = $(GENERATED_ITEMS)

-include $(top_srcdir)/git.mk
