## Process this file with automake to create Makefile.in.

basic_sources = basic-x.c

if HAVE_XFT
if INCLUDE_BASIC_XFT
XFT_INCLUDED=libpango-basic-xft.la
XFT_MODULES=
XFT_PREFIX=-DXFT_MODULE_PREFIX
else
XFT_INCLUDED=
XFT_MODULES=pango-basic-xft.la
XFT_PREFIX=
endif
else
XFT_MODULES=
XFT_INCLUDED=
XFT_PREFIX=
endif

if HAVE_FREETYPE
if INCLUDE_BASIC_FT2
FT2_INCLUDED=libpango-basic-ft2.la
FT2_MODULES=
FT2_PREFIX=-DFT2_MODULE_PREFIX
else
FT2_INCLUDED=
FT2_MODULES=pango-basic-ft2.la
FT2_PREFIX=
endif
else
FT2_MODULES=
FT2_INCLUDED=
FT2_PREFIX=
endif

if HAVE_X
if INCLUDE_BASIC_X
X_INCLUDED=libpango-basic-x.la
X_MODULES=
X_PREFIX=-DX_MODULE_PREFIX
else
X_INCLUDED=
X_MODULES=pango-basic-x.la
X_PREFIX=
endif
else
X_INCLUDED=
X_MODULES=
X_PREFIX=
endif

noinst_LTLIBRARIES = $(X_INCLUDED) $(XFT_INCLUDED) $(FT2_INCLUDED)
module_LTLIBRARIES = $(X_MODULES) $(XFT_MODULES) $(FT2_MODULES)
moddefine = $(X_PREFIX) $(XFT_PREFIX) $(FT2_PREFIX)
moduledir = $(libdir)/pango/modules
moduleflags=-rpath $(libdir)

INCLUDES = 			\
	-I$(top_srcdir) 	\
	-I$(top_srcdir)/pango/ 	\
	$(X_CFLAGS) 		\
	$(FREETYPE_CFLAGS)	\
	$(moddefine)

pango_basic_x_la_LDFLAGS = $(moduleflags) -export-dynamic -avoid-version -module
pango_basic_x_la_LIBADD = 
pango_basic_x_la_SOURCES = $(basic_sources)
libpango_basic_x_la_SOURCES = $(basic_sources)

pango_basic_xft_la_LDFLAGS = -rpath $(libdir) -export-dynamic -avoid-version -module
pango_basic_xft_la_LIBADD = 
pango_basic_xft_la_SOURCES = basic-xft.c
libpango_basic_xft_la_SOURCES = basic-xft.c

pango_basic_ft2_la_LDFLAGS = -rpath $(libdir) -export-dynamic -avoid-version -module
pango_basic_ft2_la_LIBADD =  $(FREETYPE_LIBS)
pango_basic_ft2_la_SOURCES = basic-ft2.c
libpango_basic_ft2_la_SOURCES = basic-ft2.c

EXTRA_DIST =		\
	tables-big.i	\
	tables-small.i	\
	basic-win32.c	\
	makefile.mingw	\
	makefile.mingw.in

