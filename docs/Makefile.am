## Process this file with automake to create Makefile.in.

pangoheaders = 	   	   		\
	../pango/pango.h   		\
	../pango/pango-attributes.h	\
	../pango/pango-context.h	\
	../pango/pango-coverage.h	\
	../pango/pango-engine.h		\
	../pango/pango-font.h       	\
	../pango/pango-fontmap.h	\
	../pango/pango-glyph.h	   	\
	../pango/pango-item.h	   	\
	../pango/pango-layout.h	   	\
	../pango/pango-types.h	   	\
	../pango/pangox.h	

if HAVE_GTK_DOC

# The name of the module.
DOC_MODULE=pango

# The top-level SGML file.
DOC_MAIN_SGML_FILE=pango-docs.sgml

SOURCE_DIR=../pango

scan:
	gtkdoc-scan --module=$(DOC_MODULE) $(pangoheaders)

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
	for i in $(srcdir)/TEXT/* ; do 			\
	  test -f $$i && cp -p $$i $(distdir)/TEXT ;	\
        done
