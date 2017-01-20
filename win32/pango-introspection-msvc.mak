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
	@set PYTHONPATH=$(PREFIX)\lib\gobject-introspection
	@set PATH=vs$(VSVER)\$(CFG)\$(PLAT)\bin;$(PREFIX)\bin;$(PATH)
	@set PKG_CONFIG_PATH=$(PKG_CONFIG_PATH)
	@set LIB=vs$(VSVER)\$(CFG)\$(PLAT)\bin;$(PREFIX)\lib;$(LIB)

!include introspection.body.mak

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
