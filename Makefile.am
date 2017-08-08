## Process this file with automake to create Makefile.in.

SUBDIRS= pango pango-view examples docs tools tests win32

meson_dist_files = \
	pango/pango-features.h.meson \
	pango-view/meson.build \
	meson.build \
	meson_options.txt \
	pango/mini-fribidi/meson.build \
	pango/meson.build \
	docs/meson.build \
	docs/xml/meson.build \
	docs/xml/gtkdocentities.ent.in \
	examples/meson.build \
	tools/meson.build \
	tests/meson.build \
	tests/gen-all-unicode.py \
	tests/gen-installed-test.py \
	$()

EXTRA_DIST = 			\
	autogen.sh		\
	pango.pc.in		\
	pangocairo.pc.in	\
	pangoxft.pc.in		\
	pangoft2.pc.in		\
	pangowin32.pc.in	\
	MAINTAINERS		\
	HACKING			\
	README.win32		\
	config.h.win32		\
	pango.doap		\
	$(meson_dist_files)

MAINTAINERCLEANFILES = \
	$(GITIGNORE_MAINTAINERCLEANFILES_TOPLEVEL) \
	$(GITIGNORE_MAINTAINERCLEANFILES_MAKEFILE_IN) \
	$(srcdir)/INSTALL \
	$(srcdir)/omf.make \
	$(srcdir)/xmldocs.make \
	$(srcdir)/gtk-doc.make \
	$(srcdir)/ChangeLog

### ChangeLog generation

CHANGELOG_START = "`git describe --abbrev=0 | sed -e 's|\([0-9]*\)\.\([0-9]*\)\.[0-9]*|\1.\2.0^^|'`"

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
