EXTRA_DIST =	\
	pango.sln	\
	pango-build-defines.props	\
	pango-ignore-fc.props	\
	pango-install-bin.props	\
	pango-install.props	\
	pango-install.propsin	\
	pango-prebuild.props	\
	pango-version-paths.props	\
	pango.vcxproj	\
	pango.vcxproj.filters	\
	pango.vcxprojin	\
	pango.vcxproj.filtersin	\
	pangoft2.vcxproj	\
	pangoft2.vcxproj.filters	\
	pangoft2.vcxprojin	\
	pangoft2.vcxproj.filtersin	\
	pangowin32.vcxproj	\
	pangowin32.vcxproj.filters	\
	pangocairo.vcxproj	\
	pangocairo.vcxproj.filters	\
	pangocairo.vcxprojin	\
	pangocairo.vcxproj.filtersin	\
	install.vcxproj	\
	README.txt

pango-install.props: $(top_srcdir)/build/win32/vs10/pango-install.propsin pango.vs10.headers
	$(CPP) -P - <$(top_srcdir)/build/win32/vs10/pango-install.propsin >$@
	rm pango.vs10.headers

DISTCLEANFILES = pango-install.props

-include $(top_srcdir)/git.mk
