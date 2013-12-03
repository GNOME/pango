EXTRA_DIST =	\
	pango.sln	\
	pango-build-defines.vsprops	\
	pango-gengir.vsprops	\
	pango-ignore-fc.vsprops	\
	pango-install-bin.vsprops	\
	pango-install.vsprops	\
	pango-install.vspropsin	\
	pango-prebuild.vsprops	\
	pango-version-paths.vsprops	\
	pango.vcprojin	\
	pango.vcproj	\
	pangoft2.vcprojin	\
	pangoft2.vcproj	\
	pangocairo.vcprojin	\
	pangocairo.vcproj	\
	pangowin32.vcproj	\
	install.vcproj	\
	stdint.h	\
	gengir.vcproj	\
	README.txt

pango-install.vsprops: $(top_srcdir)/build/win32/vs9/pango-install.vspropsin pango.vs9.headers
	$(CPP) -P - <$(top_srcdir)/build/win32/vs9/pango-install.vspropsin >$@
	rm pango.vs9.headers

DISTCLEANFILES = pango-install.vsprops

-include $(top_srcdir)/git.mk
