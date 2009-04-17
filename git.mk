# git.mk
#
# Copyright 2009 Red Hat, Inc.
# Written by Behdad Esfahbod
#
# To use in your project, import this file in your git repo's toplevel,
# then do "make -f git.mk".  This modifies all Makefile.am files in
# your project to include git.mk.
#
# This enables automatic .gitignore generation.  If you need to ignore
# more files, add them to the GITIGNOREFILES variable in your Makefile.am.
# Note that for files like editor backup, etc, there are better places to
# ignore them.  See "man gitignore".
#

git-all: git-mk-install

git-mk-install:
	@echo Installing git makefile
	@any_failed=; find $(top_srcdir) -name Makefile.am | while read x; do \
		if grep 'include .*/git.mk' $$x >/dev/null; then \
			echo $$x already includes git.mk; \
		else \
			failed=; \
			echo "Updating $$x"; \
			{ cat $$x; \
			  echo ''; \
			  echo '-include $$(top_srcdir)/git.mk'; \
			} > $$x.tmp || failed=1; \
			if test x$$failed = x; then \
				mv $$x.tmp $$x || failed=1; \
			fi; \
			if test x$$failed = x; then : else \
				echo Failed updating $$x; >&2 \
				any_failed=1; \
			fi; \
	fi; done; test x$$any_failed = x


### .gitignore generation

.gitignore: Makefile.am $(top_srcdir)/git.mk
	@echo Generating $@
	@GTKDOCIGNOREFILES=; test "x$(DOC_MODULE)" = x || \
	GTKDOCIGNOREFILES=" \
		$(DOC_MODULE)-decl-list.txt $(DOC_MODULE)-decl.txt \
		tmpl/$(DOC_MODULE)-unused.sgml tmpl/*.bak \
		xml html"; \
	for x in .gitignore \
		$(GITIGNOREFILES) \
		$$GTKDOCIGNOREFILES \
		$(CLEANFILES) \
		$(PROGRAMS) \
		$(EXTRA_PROGRAMS) \
		$(LTLIBRARIES) \
		so_locations \
		.libs _libs \
		$(MOSTLYCLEANFILES) \
		"*.$(OBJEXT)" \
		"*.lo" \
		$(DISTCLEANFILES) \
		$(am__CONFIG_DISTCLEAN_FILES) \
		$(CONFIG_CLEAN_FILES) \
		TAGS ID GTAGS GRTAGS GSYMS GPATH tags \
		"*.tab.c" \
		$(top_builddir)/config.h $(top_builddir)/stamp-h1 \
		$(top_builddir)/libtool $(top_builddir)/config.lt \
		$(MAINTAINERCLEANFILES) \
		$(BUILT_SOURCES) \
		$(DEPDIR) \
		$(top_srcdir)/autom4te.cache \
		Makefile \
		Makefile.in \
		$(top_srcdir)/configure \
		$(top_srcdir)/gtk-doc.make \
		$(top_srcdir)/git.mk \
		"*.orig" "*.rej" "*.bak" "*~" \
	; do echo /$$x; done | \
	grep -v '/[.][.]/' | \
	sed 's@/[.]/@/@g' | \
	LANG=C sort | uniq > $@.tmp && \
	mv $@.tmp $@

Makefile.in: $(top_srcdir)/git.mk
all: .gitignore
maintainer-clean-local: gitignore-clean
gitignore-clean:
	rm -f .gitignore
.PHONY: gitignore-clean

