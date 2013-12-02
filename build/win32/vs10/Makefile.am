EXTRA_DIST =	\
	pango.sln	\
	pango.propsin	\
	pango.props	\
	pango-ignore-fc.props	\
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
	gengir.vcxproj	\
	README.txt

pango.props: $(top_srcdir)/build/win32/vs10/pango.propsin pango.vs10.headers
	$(CPP) -P - <$(top_srcdir)/build/win32/vs10/pango.propsin >$@
	rm pango.vs10.headers

DISTCLEANFILES = pango.props

-include $(top_srcdir)/git.mk
