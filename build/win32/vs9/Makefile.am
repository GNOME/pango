GENERATED_ITEMS =		\
	pango.vcproj		\
	pangowin32.vcproj	\
	pangoft2.vcproj		\
	pangocairo.vcproj	\
	pango-install.vsprops

EXTRA_DIST =	\
	pango.sln			\
	pango-build-defines.vsprops	\
	pango-ignore-fc.vsprops		\
	pango-install-bin.vsprops	\
	pango-install.vspropsin		\
	pango-prebuild.vsprops		\
	pango-version-paths.vsprops	\
	pango.vcprojin			\
	pangoft2.vcprojin		\
	pangocairo.vcprojin		\
	pangowin32.vcprojin		\
	pango-install.vcproj		\
	stdint.h			\
	README.txt			\
	$(GENERATED_ITEMS)

pango-install.vsprops: $(top_srcdir)/build/win32/vs9/pango-install.vspropsin pango.headers
	$(CPP) -P - <$(top_srcdir)/build/win32/vs9/pango-install.vspropsin >$@
	rm pango.headers

DISTCLEANFILES = $(GENERATED_ITEMS)

-include $(top_srcdir)/git.mk
