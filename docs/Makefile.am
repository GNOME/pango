## Process this file with automake to create Makefile.in.

# The name of the module.
DOC_MODULE=pango

# The top-level SGML file.
DOC_MAIN_SGML_FILE=pango-docs.sgml

# The directory containing the source code
DOC_SOURCE_DIR=$(top_srcdir)/pango

# Extra options to supply to gtkdoc-fixref
FIXXREF_OPTIONS=

# Headers to ignore
IGNORE_HFILES=			\
	mini-fribidi		\
	opentype		\
	modules.h		\
	pango-intset.h		\
	pango-utils.h		\
	pangoft2-private.h	\
	pangowin32-private.h	\
	pangox-private.h

tmpl_sources =				\
	tmpl/coverage-maps.sgml		\
	tmpl/engines.sgml		\
	tmpl/fonts.sgml			\
	tmpl/freetype-fonts.sgml	\
	tmpl/glyphs.sgml		\
	tmpl/layout.sgml		\
	tmpl/main.sgml			\
	tmpl/modules.sgml		\
	tmpl/tab-stops.sgml		\
	tmpl/text-attributes.sgml	\
	tmpl/win32-fonts.sgml		\
	tmpl/x-fonts.sgml		\
	tmpl/x-rendering.sgml

content_files =				\
	pango_markup.sgml

extra_files =				\
	layout.gif			\
	layout.fig			\
	layout.eps


####################################
# Everything below here is generic #
####################################

TARGET_DIR=$(HTML_DIR)/$(DOC_MODULE)

EXTRA_DIST = 				\
	$(content_files)		\
	$(extra_files)			\
	$(DOC_MAIN_SGML_FILE)		\
	$(DOC_MODULE)-decl.txt		\
	$(DOC_MODULE)-sections.txt	\
	$(DOC_MODULE)-overrides.txt

if ENABLE_GTK_DOC
all-local: html/index.html

$(DOC_MODULE)-decl.txt: 
	$(MAKE) scan

$(tmpl_sources): $(DOC_MODULE)-decl.txt $(DOC_MODULE)-sections.txt $(DOC_MODULE)-overrides.txt
	$(MAKE) templates

sgml/$(DOC_MODULE)-doc.bottom: $(tmpl_sources) $(DOC_MODULE)-decl.txt
	$(MAKE) sgml

html/index.html: sgml/$(DOC_MODULE)-doc.bottom $(DOC_MAIN_SGML_FILE)
	$(MAKE) html
endif

scan:
	-(cd $(srcdir) \
	&& gtkdoc-scan --module=$(DOC_MODULE) --source-dir=$(DOC_SOURCE_DIR) --ignore-headers="$(IGNORE_HFILES)" )

templates: 
	cd $(srcdir) && gtkdoc-mktmpl --module=$(DOC_MODULE)

sgml:
	cd $(srcdir) \
	&& gtkdoc-mkdb --module=$(DOC_MODULE) --source-dir=$(DOC_SOURCE_DIR)

html:
	test -d $(srcdir)/html || mkdir $(srcdir)/html
	-cd $(srcdir)/html && gtkdoc-mkhtml $(DOC_MODULE) ../$(DOC_MAIN_SGML_FILE)
	@echo '-- Fixing Crossreferences' 
	cd $(srcdir) && gtkdoc-fixxref --module-dir=html --html-dir=$(HTML_DIR) $(FIXXREF_OPTIONS)

clean-local:
	rm -f *~ *.bak *.signals *-unused.txt

maintainer-clean-local: clean
	cd $(srcdir) && rm -rf sgml html $(DOC_MODULE)-decl-list.txt $(DOC_MODULE)-decl.txt

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(TARGET_DIR)
	(installfiles=`echo $(srcdir)/html/*.html`; \
	if test "$$installfiles" = '$(srcdir)/html/*.html'; \
	then echo '-- Nothing to install' ; \
	else \
	  for i in $$installfiles; do \
	    echo '-- Installing '$$i ; \
	    $(INSTALL_DATA) $$i $(DESTDIR)$(TARGET_DIR); \
	  done; \
	  echo '-- Installing $(srcdir)/html/index.sgml' ; \
	  $(INSTALL_DATA) $(srcdir)/html/index.sgml $(DESTDIR)$(TARGET_DIR); \
	fi)

#
# Require gtk-doc when making dist
#
if ENABLE_GTK_DOC
dist-check-gtkdoc:
else
dist-check-gtkdoc:
	@echo "*** gtk-doc must be installed and enabled in order to make dist"
	@false
endif

dist-hook: dist-check-gtkdoc
	mkdir $(distdir)/tmpl
	mkdir $(distdir)/sgml
	mkdir $(distdir)/html
	-cp $(srcdir)/tmpl/*.sgml $(distdir)/tmpl
	-cp $(srcdir)/sgml/*.sgml $(distdir)/sgml
	-cp $(srcdir)/sgml/*.bottom $(srcdir)/sgml/*.top $(distdir)/sgml
	-cp $(srcdir)/html/index.sgml $(distdir)/html
	-cp $(srcdir)/html/*.html $(srcdir)/html/*.css $(distdir)/html
	mkdir $(distdir)/TEXT;          \
	for f in $(srcdir)/TEXT/* ; do  \
	  test -f $$f && cp -p $$f $(distdir)/TEXT; \
	done

.PHONY : html sgml templates scan
