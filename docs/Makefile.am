## Process this file with automake to create Makefile.in.

if HAVE_GTK_DOC

# The name of the module.
DOC_MODULE=pango

# The top-level SGML file.
DOC_MAIN_SGML_FILE=pango-docs.sgml

INCLUDE_DIR=../pango
SOURCE_DIR=../pango

scan:
	gtkdoc-scan --module=$(DOC_MODULE) $(INCLUDE_DIR)/pango*.h

templates: scan
	gtkdoc-mktmpl --module=$(DOC_MODULE)

sgml:
	gtkdoc-mkdb --module=$(DOC_MODULE) --source-dir=$(SOURCE_DIR)

html:
	if ! test -d html ; then mkdir html ; fi
	-cd html && gtkdoc-mkhtml $(DOC_MODULE) ../$(DOC_MAIN_SGML_FILE)
	cp layout.gif html

clean-local:
	rm -f *~ *.bak *.hierarchy *.signals *.args *-unused.txt

maintainer-clean-local: clean
	rm -rf sgml html $(DOC_MODULE)-decl-list.txt $(DOC_MODULE)-decl.txt 

.PHONY: scan sgml html templates 

endif

EXTRA_DIST=pango-sections.txt pango-docs.sgml layout.fig layout.eps layout.fig

dist-hook:
	mkdir $(distdir)/tmpl
	cp -p $(srcdir)/tmpl/*.sgml $(distdir)/tmpl
	mkdir $(distdir)/TEXT
	for i in $(srcdir)/TEXT/* ; do \
	  test -f $$i && cp -p $$i $(distdir)/TEXT ;\
        done
