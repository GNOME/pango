## Process this file with automake to create Makefile.in.

SUBDIRS= pango modules pango-view examples docs tools tests build

EXTRA_DIST = 			\
	autogen.sh		\
	pango.pc.in		\
	pangocairo.pc.in	\
	pangox.pc.in		\
	pangoxft.pc.in		\
	pangoft2.pc.in		\
	pangowin32.pc.in	\
	pango-uninstalled.pc.in		\
	pangocairo-uninstalled.pc.in	\
	pangox-uninstalled.pc.in	\
	pangoxft-uninstalled.pc.in	\
	pangoft2-uninstalled.pc.in	\
	pangowin32-uninstalled.pc.in	\
	ChangeLog.pre-1-0	\
	ChangeLog.pre-1-2	\
	ChangeLog.pre-1-4	\
	ChangeLog.pre-1-6	\
	ChangeLog.pre-1-8	\
	ChangeLog.pre-1-10	\
	ChangeLog.pre-1-12	\
	ChangeLog.pre-1-14	\
	ChangeLog.pre-1-16	\
	ChangeLog.pre-1-18	\
	ChangeLog.pre-1-20	\
	ChangeLog.pre-1-23	\
	MAINTAINERS		\
	HACKING			\
	README.win32		\
	config.h.win32		\
	pango-zip.sh.in		\
	sanitize-la.sh		\
	pango.doap

MAINTAINERCLEANFILES = \
	$(srcdir)/INSTALL \
	$(srcdir)/aclocal.m4 \
	$(srcdir)/autoscan.log \
	$(srcdir)/compile \
	$(srcdir)/config.guess \
	$(srcdir)/config.h.in \
	$(srcdir)/config.sub \
	$(srcdir)/configure.scan \
	$(srcdir)/depcomp \
	$(srcdir)/install-sh \
	$(srcdir)/ltmain.sh \
	$(srcdir)/missing \
	$(srcdir)/mkinstalldirs \
	$(srcdir)/omf.make \
	$(srcdir)/xmldocs.make \
	$(srcdir)/gtk-doc.make \
	$(srcdir)/ChangeLog \
	`find "$(srcdir)" -type f -name Makefile.in -print`

### ChangeLog generation

CHANGELOG_START = 1.23.0^^

ChangeLog: $(srcdir)/ChangeLog
$(srcdir)/ChangeLog:
	$(AM_V_GEN) if test -d "$(srcdir)/.git"; then \
	  (GIT_DIR=$(top_srcdir)/.git ./missing --run \
	   git log $(CHANGELOG_START).. --stat) | fmt --split-only > $@.tmp \
	  && mv -f $@.tmp $@ \
	  || ($(RM) $@.tmp; \
	      echo Failed to generate ChangeLog, your ChangeLog may be outdated >&2; \
	      (test -f $@ || echo git-log is required to generate this file >> $@)); \
	else \
	  test -f $@ || \
	  (echo A git checkout and git-log is required to generate ChangeLog >&2 && \
	  echo A git checkout and git-log is required to generate this file >> $@); \
	fi
.PHONY: $(srcdir)/ChangeLog


pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = pango.pc

if HAVE_CAIRO
pkgconfig_DATA += pangocairo.pc
endif

if HAVE_X
pkgconfig_DATA += pangox.pc
endif

if HAVE_XFT
pkgconfig_DATA += pangoxft.pc
endif

if HAVE_FREETYPE
pkgconfig_DATA += pangoft2.pc
endif

if HAVE_WIN32
pkgconfig_DATA += pangowin32.pc
endif

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc --enable-man --disable-doc-cross-references --enable-introspection

-include $(top_srcdir)/git.mk
