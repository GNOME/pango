## Process this file with automake to create Makefile.in.

SUBDIRS= pango modules examples docs tools tests

EXTRA_DIST = 		\
	pango.spec.in	\
	pango.pc.in	\
	pangox.pc.in	\
	pangoxft.pc.in	\
	pangoft2.pc.in	\
	pangowin32.pc.in	\
	pango-uninstalled.pc.in		\
	pangox-uninstalled.pc.in	\
	pangoxft-uninstalled.pc.in	\
	pangoft2-uninstalled.pc.in	\
	pangowin32-uninstalled.pc.in	\
	TODO.xml	\
	README.win32	\
	pango-zip.sh	\
	sanitize-la.sh

if HAVE_X
X_PC=pangox.pc
endif

if HAVE_XFT
XFT_PC=pangoxft.pc
endif

if HAVE_FREETYPE
FT2_PC=pangoft2.pc
endif

if HAVE_WIN32
WIN32_PC=pangowin32.pc
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pango.pc $(X_PC) $(XFT_PC) $(FT2_PC) $(WIN32_PC)

dist-hook: pango.spec
	cp pango.spec $(distdir)

# This is a version of the automake-1.4 distcheck rule modified
# to pass --enable-gtk-doc to ./configure
#
mydistcheck: dist
	-rm -rf $(distdir)
	GZIP=$(GZIP_ENV) $(TAR) zxf $(distdir).tar.gz
	mkdir $(distdir)/=build
	mkdir $(distdir)/=inst
	dc_install_base=`cd $(distdir)/=inst && pwd`; \
	cd $(distdir)/=build \
	  && ../configure --srcdir=.. --prefix=$$dc_install_base --enable-gtk-doc \
	  && $(MAKE) $(AM_MAKEFLAGS) \
	  && $(MAKE) $(AM_MAKEFLAGS) dvi \
	  && $(MAKE) $(AM_MAKEFLAGS) check \
	  && $(MAKE) $(AM_MAKEFLAGS) install \
	  && $(MAKE) $(AM_MAKEFLAGS) installcheck \
	  && $(MAKE) $(AM_MAKEFLAGS) dist
	-rm -rf $(distdir)
	@banner="$(distdir).tar.gz is ready for distribution"; \
	dashes=`echo "$$banner" | sed s/./=/g`; \
	echo "$$dashes"; \
	echo "$$banner"; \
	echo "$$dashes"
