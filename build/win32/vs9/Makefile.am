EXTRA_DIST =	\
	pango.sln	\
	pango_fc.sln	\
	pango.vspropsin	\
	pango.vsprops	\
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

pango.vsprops: $(top_srcdir)/build/win32/vs9/pango.vspropsin pango.vs9.headers
	$(CPP) -P - <$(top_srcdir)/build/win32/vs9/pango.vspropsin >$@
	rm pango.vs9.headers

DISTCLEANFILES = pango.vsprops

-include $(top_srcdir)/git.mk
