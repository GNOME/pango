# NMake Makefile to build Introspection Files for Pango

!include detectenv-msvc.mak

APIVERSION = 1.0

CHECK_PACKAGE = gobject-2.0 cairo

!include introspection-msvc.mak

!if "$(BUILD_INTROSPECTION)" == "TRUE"
!if "$(BUILD_PANGOFT2_INTROSPECTION)" == "1"

# Build of PangoFT2 introspection files is not currently supported.
PangoFT2LIBS = --library=pangoft2-1.0
PangoFT2GIR = --include-uninstalled=./PangoFT2-$(APIVERSION)

all: setbuildenv Pango-$(APIVERSION).gir Pango-$(APIVERSION).typelib PangoFT2-$(APIVERSION).gir PangoFT2-$(APIVERSION).typelib PangoCairo-$(APIVERSION).gir PangoCairo-$(APIVERSION).typelib

!else

PangoFT2LIBS =
PangoFT2GIR =

all: setbuildenv Pango-$(APIVERSION).gir Pango-$(APIVERSION).typelib PangoCairo-$(APIVERSION).gir PangoCairo-$(APIVERSION).typelib

install-introspection: setbuildenv Pango-$(APIVERSION).gir Pango-$(APIVERSION).typelib PangoCairo-$(APIVERSION).gir PangoCairo-$(APIVERSION).typelib
	@-copy Pango-$(APIVERSION).gir $(G_IR_INCLUDEDIR)
	@-copy /b Pango-$(APIVERSION).typelib $(G_IR_TYPELIBDIR)
	@-copy PangoCairo-$(APIVERSION).gir $(G_IR_INCLUDEDIR)
	@-copy /b PangoCairo-$(APIVERSION).typelib $(G_IR_TYPELIBDIR)
!endif

setbuildenv:
	@set PYTHONPATH=$(BASEDIR)\lib\gobject-introspection
	@set PATH=win32\vs$(VSVER)\$(CFG)\$(PLAT)\bin;$(BASEDIR)\bin;$(PATH)
	@set PKG_CONFIG_PATH=$(PKG_CONFIG_PATH)
	@set LIB=win32\vs$(VSVER)\$(CFG)\$(PLAT)\bin;$(BASEDIR)\lib;$(LIB)

Pango-$(APIVERSION).gir: pango_list
	@-echo Generating Pango-$(APIVERSION).gir...
	$(PYTHON2) $(G_IR_SCANNER) --verbose -I..	\
	-I$(BASEDIR)\include\glib-2.0 -I$(BASEDIR)\lib\glib-2.0\include	\
	--namespace=Pango --nsversion=$(APIVERSION)	\
	--include=GObject-2.0 --include=cairo-1.0	\
	--no-libtool --pkg=gobject-2.0 --pkg=cairo --pkg=glib-2.0 --library=pango-1.0	\
	--reparse-validate --add-include-path=$(G_IR_INCLUDEDIR)	\
	--pkg-export pango --warn-all --c-include "pango/pango.h"	\
	-DG_LOG_DOMAIN=\"Pango\" -DPANGO_ENABLE_BACKEND -DPANGO_ENABLE_ENGINE	\
	-DSYSCONFDIR=\"/dummy/etc\" -DLIBDIR=\"/dummy/lib\"	\
	--filelist=pango_list -o Pango-$(APIVERSION).gir

PangoCairo-$(APIVERSION).gir: Pango-$(APIVERSION).gir
	@-echo Generating PangoCairo-$(APIVERSION).gir...
	$(PYTHON2) $(G_IR_SCANNER) --verbose -I..	\
	-I$(BASEDIR)\include\glib-2.0 -I$(BASEDIR)\lib\glib-2.0\include -I$(BASEDIR)\include	\
	--namespace=PangoCairo --nsversion=$(APIVERSION)	\
	--include=GObject-2.0 --include=cairo-1.0	\
	--no-libtool --pkg=gobject-2.0 --pkg=cairo --library=pangocairo-1.0 $(PangoFT2LIBS) --library=pango-1.0	\
	--reparse-validate --add-include-path=$(G_IR_INCLUDEDIR) --add-include-path=.	\
	--pkg-export pangocairo --warn-all $(PangoFT2GIR) --include-uninstalled=./Pango-$(APIVERSION).gir	\
	--c-include "pango/pangocairo.h"	\
	-I..\.. -DG_LOG_DOMAIN=\"Pango\" -DPANGO_ENABLE_BACKEND	\
	-DPANGO_ENABLE_ENGINE -DSYSCONFDIR=\"/dummy/etc\" -DLIBDIR=\"/dummy/lib\"	\
	--filelist=pangocairo_list -o PangoCairo-$(APIVERSION).gir

pangocairo_list: pango_list

pango_list:
	@-echo Generating Filelist to Introspect for Pango...
	$(PYTHON2) gen-file-list-pango.py

Pango-$(APIVERSION).typelib: Pango-$(APIVERSION).gir
	@-echo Compiling Pango-$(APIVERSION).typelib...
	$(G_IR_COMPILER) --includedir=. --debug --verbose Pango-$(APIVERSION).gir -o Pango-$(APIVERSION).typelib

PangoCairo-$(APIVERSION).typelib: PangoCairo-$(APIVERSION).gir Pango-$(APIVERSION).typelib
	@-echo Compiling PangoCairo-$(APIVERSION).typelib...
	$(G_IR_COMPILER) --includedir=. --debug --verbose PangoCairo-$(APIVERSION).gir -o PangoCairo-$(APIVERSION).typelib

!else
all:
	@-echo $(ERROR_MSG)
!endif

clean:
	@-del /f/q PangoCairo-$(APIVERSION).typelib
	@-del /f/q PangoCairo-$(APIVERSION).gir
	@-del /f/q PangoFT2-$(APIVERSION).typelib
	@-del /f/q PangoFT2-$(APIVERSION).gir
	@-del /f/q Pango-$(APIVERSION).typelib
	@-del /f/q Pango-$(APIVERSION).gir
	@-del /f/q pangocairoft_list
	@-del /f/q pangocairo_list
	@-del /f/q pangoft_list
	@-del /f/q pango_list
	@-del /f/q *.pyc
