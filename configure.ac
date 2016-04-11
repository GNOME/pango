dnl Process this file with autoconf to create configure.
AC_PREREQ(2.56)

dnl ==========================================================================
dnl                               Versioning
dnl ==========================================================================

dnl Making releases:
dnl   PANGO_VERSION_MICRO += 1;
dnl   PANGO_INTERFACE_AGE += 1;
dnl   PANGO_BINARY_AGE += 1;
dnl if any functions have been added, set PANGO_INTERFACE_AGE to 0.
dnl if backwards compatibility has been broken,
dnl set PANGO_BINARY_AGE _and_ PANGO_INTERFACE_AGE to 0.

dnl
dnl We do the version number components as m4 macros
dnl so that we can base configure --help output off
dnl of them.
dnl

dnl The triplet
m4_define([pango_version_major], [1])
m4_define([pango_version_minor], [40])
m4_define([pango_version_micro], [1])

m4_define([pango_version],
          [pango_version_major.pango_version_minor.pango_version_micro])
dnl The X.Y in -lpango-X.Y line. This is expected to stay 1.0 until Pango 2.
m4_define([pango_api_version], [1.0])
m4_if(m4_eval(pango_version_minor % 2), [1],
      [
	dnl for unstable releases
	m4_define([pango_interface_age], [0])
      ],
      [
	dnl for stable releases
	m4_define([pango_interface_age], pango_version_micro)
      ])
dnl Number of releases since we've broken binary compatibility.
m4_define([pango_binary_age],
          [m4_eval(100 * pango_version_minor + pango_version_micro)])


AC_INIT(pango, pango_version(),
        [http://bugzilla.gnome.org/enter_bug.cgi?product=pango])

AC_CONFIG_SRCDIR([pango.pc.in])

dnl -Wno-portability tells automake to stop complaining about the
dnl usage of GNU Make specific features.
AM_INIT_AUTOMAKE(1.9 gnits dist-xz no-dist-gzip -Wno-portability)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])
AC_CONFIG_HEADERS([config.h])


PANGO_VERSION_MAJOR=pango_version_major()
PANGO_VERSION_MINOR=pango_version_minor()
PANGO_VERSION_MICRO=pango_version_micro()
PANGO_VERSION=pango_version()
PANGO_API_VERSION=pango_api_version()
PANGO_INTERFACE_AGE=pango_interface_age()
PANGO_BINARY_AGE=pango_binary_age()

AC_SUBST(PANGO_VERSION_MAJOR)
AC_SUBST(PANGO_VERSION_MINOR)
AC_SUBST(PANGO_VERSION_MICRO)
AC_SUBST(PANGO_VERSION)
AC_SUBST(PANGO_API_VERSION)
AC_SUBST(PANGO_INTERFACE_AGE)
AC_SUBST(PANGO_BINARY_AGE)

AC_DEFINE(PANGO_BINARY_AGE, pango_binary_age(), [PANGO binary age])
AC_DEFINE(PANGO_INTERFACE_AGE, pango_interface_age(), [PANGO interface age])
AC_DEFINE(PANGO_VERSION_MAJOR, pango_version_major(), [PANGO major version])
AC_DEFINE(PANGO_VERSION_MINOR, pango_version_minor(), [PANGO minor version])
AC_DEFINE(PANGO_VERSION_MICRO, pango_version_micro(), [PANGO micro version])

dnl libtool versioning
m4_define([lt_current], [m4_eval(100 * pango_version_minor + pango_version_micro - pango_interface_age)])
m4_define([lt_revision], [pango_interface_age])
m4_define([lt_age], [m4_eval(pango_binary_age - pango_interface_age)])
VERSION_INFO="lt_current():lt_revision():lt_age()"
PANGO_CURRENT_MINUS_AGE=m4_eval(lt_current - lt_age)

AC_SUBST(PANGO_CURRENT_MINUS_AGE)

dnl ==========================================================================

AC_CANONICAL_HOST

AC_MSG_CHECKING([for native Win32])
case "$host" in
  *-*-mingw*)
    pango_os_win32=yes
    case "$host" in
      x86_64-*-*)
	LIB_EXE_MACHINE_FLAG=X64
	;;
      *)
	LIB_EXE_MACHINE_FLAG=X86
	;;
    esac
    ;;
  *)
    pango_os_win32=no
    ;;
esac
AC_MSG_RESULT([$pango_os_win32])

AC_SUBST(LIB_EXE_MACHINE_FLAG)

AM_CONDITIONAL(OS_WIN32, test "$pango_os_win32" = "yes")

AC_PROG_CC

dnl
dnl Check for a working C++ compiler, but do not bail out, if none is found.
dnl We use this for an automated test for C++ header correctness.
dnl
AC_CHECK_TOOLS(CXX, [$CCC c++ g++ gcc CC cxx cc++ cl], gcc)
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
AC_TRY_COMPILE(,[class a { int b; } c;], ,CXX=)
AM_CONDITIONAL(HAVE_CXX, test "$CXX" != "")
AC_LANG_RESTORE




AC_CHECK_FUNCS(sysconf getpagesize)
AC_CHECK_HEADERS(unistd.h sys/mman.h)


#
# Win32 stuff
#

AC_LIBTOOL_WIN32_DLL
AM_DISABLE_STATIC

AM_PROG_LIBTOOL
dnl when using libtool 2.x create libtool early, because it's used in configure
m4_ifdef([LT_OUTPUT], [LT_OUTPUT])

AC_MSG_CHECKING([for some Win32 platform])
case "$host" in
  *-*-mingw*|*-*-cygwin*)
    pango_platform_win32=yes
    ;;
  *)
    pango_platform_win32=no
    ;;
esac
AC_MSG_RESULT([$pango_platform_win32])
AM_CONDITIONAL(PLATFORM_WIN32, test "$pango_platform_win32" = "yes")

if test "$pango_platform_win32" = "yes"; then
  AC_CHECK_TOOL(WINDRES, windres, no)
  if test "$WINDRES" = no; then
    AC_MSG_ERROR([*** Could not find an implementation of windres in your PATH.])
  fi
  AC_CHECK_PROG(ms_librarian, lib.exe, yes, no)
fi
AM_CONDITIONAL(MS_LIB_AVAILABLE, test x$ms_librarian = xyes)

changequote(,)dnl
if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
  *[\ \	]-Wall[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac
fi
changequote([,])dnl

dnl declare --enable-* args and collect ac_help strings

m4_define([debug_default],
          m4_if(m4_eval(pango_version_minor() % 2), [1], [yes], [minimum]))

AC_ARG_ENABLE(debug,
              AC_HELP_STRING([--enable-debug=@<:@no/minimum/yes@:>@],
                             [turn on debugging @<:@default=debug_default()@:>@]),
	      ,
              enable_debug=debug_default())

if test "x$enable_debug" = "xyes"; then
  PANGO_DEBUG_FLAGS="-DPANGO_ENABLE_DEBUG"
else
  PANGO_DEBUG_FLAGS="-DG_DISABLE_CAST_CHECKS"

  if test "x$enable_debug" = "xno"; then
    PANGO_DEBUG_FLAGS="$GLIB_DEBUG_FLAGS -DG_DISABLE_ASSERT -DG_DISABLE_CHECKS $PANGO_DEBUG_FLAGS"
  fi
fi

AC_SUBST(PANGO_DEBUG_FLAGS)

AC_ARG_ENABLE(rebuilds,
              [AC_HELP_STRING([--disable-rebuilds],
                              [disable all source autogeneration rules])],,
              [enable_rebuilds=yes])

AM_CONDITIONAL(CROSS_COMPILING, [test $cross_compiling = yes])


#
# Checks for HarfBuzz
#
have_harfbuzz=false
PKG_CHECK_MODULES(HARFBUZZ, harfbuzz >= 0.9.30, have_harfbuzz=true, :)
AM_CONDITIONAL(HAVE_HARFBUZZ, $have_harfbuzz)

#
# Check for fontconfig
#
have_fontconfig=false
if $have_harfbuzz ; then
  PKG_CHECK_MODULES(FONTCONFIG, fontconfig >= 2.10.91, have_fontconfig=true, :)
fi
if $have_fontconfig ; then
  pango_save_libs=$LIBS
  LIBS="$LIBS $FONTCONFIG_LIBS"
  AC_CHECK_FUNCS(FcWeightFromOpenType)
  LIBS=$pango_save_libs
fi

#
# Checks for FreeType
#
have_freetype=false
if $have_fontconfig ; then
  # The first version of freetype with a .pc file is 2.1.5.  That's recent
  # enough for all the functions we use I guess.  No version check needed.
  PKG_CHECK_MODULES(FREETYPE, freetype2, have_freetype=true, :)
fi
if $have_freetype ; then
  AC_DEFINE(HAVE_FREETYPE, 1, [Have FreeType 2 library])
fi
AM_CONDITIONAL(HAVE_FREETYPE, $have_freetype)

#
# Checks for Xft/XRender
#
have_xft=false
AC_ARG_WITH(xft,
  AS_HELP_STRING([--with-xft], [build xft backend]),
  [], [with_xft=yes])
if $have_freetype ; then
  PKG_CHECK_MODULES(XFT, xft >= 2.0.0 xrender, have_xft=true, :)
fi
if test "x$with_xft" = xyes && $have_xft ; then
  AC_DEFINE(HAVE_XFT, 1, [Have Xft library])
fi
AM_CONDITIONAL(HAVE_XFT, test "x$with_xft" = xyes && $have_xft )

#
# Checks for Win32
#
have_win32=false
# The following doesn't work with autoconf-2.13, so we check $host instead
# AC_CHECK_LIB(gdi32, GetTextMetricsA@8, have_win32=true, : )
case "$host" in
  *-*-mingw*|*-*-cygwin*) have_win32=true ;;
esac
AM_CONDITIONAL(HAVE_WIN32, $have_win32)

# Ensure MSVC-compatible struct packing convention is used when
# compiling for Win32 with gcc.
# What flag to depends on gcc version: gcc3 uses "-mms-bitfields", while
# gcc2 uses "-fnative-struct".
if test "$pango_os_win32" = "yes"; then
  if test x"$GCC" = xyes; then
    msnative_struct=''
    AC_MSG_CHECKING([how to get MSVC-compatible struct packing])
    if test -z "$ac_cv_prog_CC"; then
      our_gcc="$CC"
    else
      our_gcc="$ac_cv_prog_CC"
    fi
    case `$our_gcc --version | sed -e 's,\..*,.,' -e q` in
      2.)
	if $our_gcc -v --help 2>/dev/null | grep fnative-struct >/dev/null; then
	  msnative_struct='-fnative-struct'
	fi
	;;
      *)
	if $our_gcc -v --help 2>/dev/null | grep ms-bitfields >/dev/null; then
	  msnative_struct='-mms-bitfields'
	fi
	;;
    esac
    if test x"$msnative_struct" = x ; then
      AC_MSG_RESULT([no way])
      AC_MSG_WARN([produced libraries might be incompatible with MSVC-compiled code])
    else
      CFLAGS="$CFLAGS $msnative_struct"
      AC_MSG_RESULT([${msnative_struct}])
    fi
  fi
fi

#
# Checks for CoreText
#

AC_MSG_CHECKING([for CoreText availability])
pango_save_libs=$LIBS
LIBS="$LIBS -framework ApplicationServices"
AC_TRY_LINK([#include <Carbon/Carbon.h>], [CTGetCoreTextVersion ();],
[have_core_text=yes], [have_core_text=no])
LIBS=$pango_save_libs

if test "$have_core_text" = "yes"; then
  AC_DEFINE(HAVE_CORE_TEXT, 1, [Whether CoreText is available on the system])
fi

AC_MSG_RESULT([$have_core_text])

AM_CONDITIONAL(HAVE_CORE_TEXT, test "$have_core_text" = "yes")

#
# Checks for Cairo
#
have_cairo=false
have_cairo_png=false
have_cairo_ps=false
have_cairo_pdf=false
have_cairo_xlib=false
have_cairo_freetype=false
have_cairo_win32=false
have_cairo_quartz=false
cairo_required=1.12.10

AC_ARG_WITH(cairo,
	    AS_HELP_STRING([--without-cairo], [Do not use cairo]),
	    :, with_cairo=auto)
AS_IF([test x$with_cairo != xno], [
  PKG_CHECK_MODULES(CAIRO, cairo >= $cairo_required, have_cairo=true, :)
])

if $have_cairo ; then
  pango_save_libs=$LIBS
  LIBS="$LIBS $CAIRO_LIBS"
  pango_save_ldflags=$LDFLAGS
  LDFLAGS="$LDFLAGS $CAIRO_LIBS"

  have_cairo=false
  cairo_font_backends=""

  if $have_freetype; then
    PKG_CHECK_EXISTS(cairo-ft >= $cairo_required, have_cairo_freetype=true, :)
  fi
  if $have_cairo_freetype; then
    AC_DEFINE(HAVE_CAIRO_FREETYPE, 1, [Whether Cairo can use FreeType for fonts])
    cairo_font_backends="freetype $cairo_font_backends"
    have_cairo=true
  fi

  if $have_win32; then
    PKG_CHECK_EXISTS(cairo-win32 >= $cairo_required, have_cairo_win32=true, :)
  fi
  if $have_cairo_win32; then
    AC_DEFINE(HAVE_CAIRO_WIN32, 1, [Whether Cairo can use the Win32 GDI for fonts])
    cairo_font_backends="win32 $cairo_font_backends"
    have_cairo=true
  fi

  if test $have_core_text; then
    PKG_CHECK_EXISTS(cairo-quartz-font >= $cairo_required, have_cairo_quartz=true, :)
  fi
  if $have_cairo_quartz; then
    AC_DEFINE(HAVE_CAIRO_QUARTZ, 1, [Whether Cairo can use Quartz for fonts])
    cairo_font_backends="quartz $cairo_font_backends"
    have_cairo=true
  fi

  AC_MSG_CHECKING([which cairo font backends could be used])
  if $have_cairo ; then
    AC_MSG_RESULT([$cairo_font_backends])
  else
    if test x${with_cairo} != xauto; then
      AC_MSG_ERROR([cairo support requested, but not present])
    fi
    AC_MSG_RESULT([none])
    AC_MSG_NOTICE([Disabling cairo support])
  fi

  if $have_cairo ; then
    AC_DEFINE(HAVE_CAIRO, 1, [Have usable Cairo library and font backend])

    PKG_CHECK_EXISTS(cairo-png >= $cairo_required, have_cairo_png=true, :)
    if $have_cairo_png; then
      AC_DEFINE(HAVE_CAIRO_PNG, 1, [Whether Cairo has PNG support])
    fi
  
    PKG_CHECK_EXISTS(cairo-ps >= $cairo_required, have_cairo_ps=true, :)
    if $have_cairo_ps; then
      AC_DEFINE(HAVE_CAIRO_PS, 1, [Whether Cairo has PS support])
    fi
  
    PKG_CHECK_EXISTS(cairo-pdf >= $cairo_required, have_cairo_pdf=true, :)
    if $have_cairo_pdf; then
      AC_DEFINE(HAVE_CAIRO_PDF, 1, [Whether Cairo has PDF support])
    fi
  
    PKG_CHECK_EXISTS(cairo-xlib >= $cairo_required, have_cairo_xlib=true, :)
    if $have_cairo_xlib; then
      AC_DEFINE(HAVE_CAIRO_XLIB, 1, [Whether Cairo has Xlib support])
    fi
  fi

  LIBS=$pango_save_libs
  LDFLAGS=$pango_save_ldflags

  PKGCONFIG_CAIRO_REQUIRES=
  if $have_cairo_freetype;  then
    PKGCONFIG_CAIRO_REQUIRES="$PKGCONFIG_CAIRO_REQUIRES pangoft2"
  fi
  if $have_cairo_win32;  then
    PKGCONFIG_CAIRO_REQUIRES="$PKGCONFIG_CAIRO_REQUIRES pangowin32"
  fi
  AC_SUBST(PKGCONFIG_CAIRO_REQUIRES)
fi

AM_CONDITIONAL(HAVE_CAIRO, $have_cairo)
AM_CONDITIONAL(HAVE_CAIRO_PNG, $have_cairo_png)
AM_CONDITIONAL(HAVE_CAIRO_PS, $have_cairo_ps)
AM_CONDITIONAL(HAVE_CAIRO_PDF, $have_cairo_pdf)
AM_CONDITIONAL(HAVE_CAIRO_XLIB, $have_cairo_xlib)
AM_CONDITIONAL(HAVE_CAIRO_WIN32, $have_cairo_win32)
AM_CONDITIONAL(HAVE_CAIRO_FREETYPE, $have_cairo_freetype)
AM_CONDITIONAL(HAVE_CAIRO_QUARTZ, $have_cairo_quartz)


#
# We must have some backend defined
#
if $have_freetype || `test "x$with_xft" = xyes && $have_xft` || $have_cairo || $have_win32 ; then : ; else
  AC_MSG_ERROR([*** Could not enable any backends.
*** Must have at least one backend to build Pango.])
fi

#
# Checks for GLib
#
GLIB_REQUIRED_VERSION=2.33.12
GLIB_MODULES="glib-2.0 >= $GLIB_REQUIRED_VERSION gobject-2.0 gthread-2.0"

PKG_CHECK_MODULES(GLIB, $GLIB_MODULES, :,
  AC_MSG_ERROR([
*** Glib $GLIB_REQUIRED_VERSION or better is required. The latest version of 
*** Glib is always available from ftp://ftp.gtk.org/.]))

# Setup GLIB_MKENUMS to use glib-mkenums even if GLib is uninstalled.
GLIB_MKENUMS=`$PKG_CONFIG --variable=glib_mkenums glib-2.0`
AC_SUBST(GLIB_MKENUMS)

#
# Checks for LibThai
#
LIBTHAI_REQUIRED_VERSION=0.1.9
have_libthai=false
PKG_CHECK_MODULES(LIBTHAI, libthai >= $LIBTHAI_REQUIRED_VERSION, have_libthai=true, :)
if $have_libthai; then
	AC_DEFINE(HAVE_LIBTHAI, 1, [Whether libthai is available])
fi

#
# Checks for GObject Introspection
#

GOBJECT_INTROSPECTION_CHECK([0.9.5])


#
# We use flockfile to implement pango_getline() - should be moved to GLib
# strtok_r isn't present on some systems
#
AC_CHECK_FUNCS(flockfile strtok_r)

dnl **************************
dnl *** Checks for gtk-doc ***
dnl **************************

if $have_cairo ; then : ; else
   if  test x$enable_gtk_doc = xyes ; then
      AC_MSG_WARN([Cairo not present, disabling doc building])      
      enable_gtk_doc=no
   fi
fi

GTK_DOC_CHECK([1.15], [--flavour no-tmpl])

AC_ARG_ENABLE(doc-cross-references,
              AC_HELP_STRING([--disable-doc-cross-references],
                             [cross reference glib and cairo symbols @<:@default=yes@:>@]),
	      enable_doc_cross_references=$enableval,
	      enable_doc_cross_references=yes)

if test "x$enable_doc_cross_references" != xno ; then
	GLIB_PREFIX="`$PKG_CONFIG --variable=prefix glib-2.0`"
	CAIRO_PREFIX="`$PKG_CONFIG --variable=prefix cairo`"
	AC_SUBST(GLIB_PREFIX)
	AC_SUBST(CAIRO_PREFIX)
fi
AM_CONDITIONAL(ENABLE_DOC_CROSS_REFERENCES, test x$enable_doc_cross_references != xno)

dnl ********************************************************
dnl * Options to pass to libtool
dnl ********************************************************

# Note that -module isn't included here since automake needs to see it to know
# that something like pango-basic-fc.la is a valid libtool archive
#
LIBRARY_LIBTOOL_OPTIONS="-version-info $VERSION_INFO"
if test "$pango_platform_win32" = yes; then
    # We currently use .def files on Windows
    LIBRARY_LIBTOOL_OPTIONS="$LIBRARY_LIBTOOL_OPTIONS -no-undefined"
else
    # libtool option to control which symbols are exported
    # right now, symbols starting with '_' are not exported
    LIBRARY_LIBTOOL_OPTIONS="$LIBRARY_LIBTOOL_OPTIONS "'-export-symbols-regex "^pango_.*"'
fi
AC_SUBST(LIBRARY_LIBTOOL_OPTIONS)

dnl
dnl Check for -Bsymbolic-functions linker flag used to avoid
dnl intra-library PLT jumps, if available.
dnl
AC_ARG_ENABLE(Bsymbolic,
              [AS_HELP_STRING([--disable-Bsymbolic],
                              [avoid linking with -Bsymbolic])],,
              [SAVED_LDFLAGS="${LDFLAGS}"
               AC_MSG_CHECKING([for -Bsymbolic-functions linker flag])
               LDFLAGS=-Wl,-Bsymbolic-functions
               AC_TRY_LINK([], [int main (void) { return 0; }],
                           AC_MSG_RESULT(yes)
                           enable_Bsymbolic=yes,
                           AC_MSG_RESULT(no)
                           enable_Bsymbolic=no)
               LDFLAGS="${SAVED_LDFLAGS}"])

if test "x${enable_Bsymbolic}" = "xyes"; then
  LIBRARY_LIBTOOL_OPTIONS="$LIBRARY_LIBTOOL_OPTIONS -Wl,-Bsymbolic-functions"
fi

dnl ********************************************************
dnl * Installed test                                       *
dnl ********************************************************

AC_ARG_ENABLE(installed_tests,
              AS_HELP_STRING([--enable-installed-tests],
                             [Install test programs (default: no)]),,
              [enable_installed_tests=no])
AM_CONDITIONAL(BUILDOPT_INSTALL_TESTS, test x$enable_installed_tests = xyes)


AC_HEADER_DIRENT
AC_CHECK_HEADERS(unistd.h)

# Honor aclocal flags
ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"
dnl ===========================================================================
AC_CONFIG_COMMANDS([pango/pango-features.h],
          [featuresfile=pango/pango-features.h
           outfile=$featuresfile.tmp
           AC_MSG_NOTICE([creating $featuresfile])
           cat >$outfile <<_EOF
/* Generated by configure. Do not edit */
#ifndef PANGO_FEATURES_H
#define PANGO_FEATURES_H

#define PANGO_VERSION_MAJOR $PANGO_VERSION_MAJOR
#define PANGO_VERSION_MINOR $PANGO_VERSION_MINOR
#define PANGO_VERSION_MICRO $PANGO_VERSION_MICRO

#define PANGO_VERSION_STRING "$PANGO_VERSION_MAJOR.$PANGO_VERSION_MINOR.$PANGO_VERSION_MICRO"

_EOF
	# TODO: define macros for enabled features/backends here

        echo '' >>$outfile
        echo '#endif' >>$outfile

        if cmp -s $outfile $featuresfile; then
          AC_MSG_NOTICE([$featuresfile is unchanged])
          rm -f $outfile
        else
          mv $outfile $featuresfile
        fi
],[
        PANGO_VERSION_MAJOR=$PANGO_VERSION_MAJOR
        PANGO_VERSION_MINOR=$PANGO_VERSION_MINOR
        PANGO_VERSION_MICRO=$PANGO_VERSION_MICRO
])

AC_CONFIG_FILES([
Makefile
pango/Makefile
pango/mini-fribidi/Makefile
pango/pango.rc
pango/pangoft2.rc
pango/pangowin32.rc
pango/pangocairo.rc
pango/pangoxft.rc
pango-view/Makefile
examples/Makefile
docs/Makefile
docs/version.xml
tools/Makefile
tests/Makefile
build/Makefile
build/win32/Makefile
build/win32/vs9/Makefile
build/win32/vs10/Makefile
build/win32/vs11/Makefile
build/win32/vs12/Makefile
build/win32/vs14/Makefile
pango.pc
pangowin32.pc
pangoft2.pc
pangoxft.pc
pangocairo.pc
pango-uninstalled.pc
pangowin32-uninstalled.pc
pangoft2-uninstalled.pc
pangoxft-uninstalled.pc
pangocairo-uninstalled.pc
config.h.win32
])

AC_OUTPUT

backends=""
if $have_cairo ; then backends="$backends Cairo"; fi
if $have_win32 ; then backends="$backends Win32"; fi
if test "x$with_xft" = xyes && $have_xft ; then backends="$backends Xft"; fi
if $have_freetype && $have_fontconfig ; then backends="$backends FreeType"; fi

echo "configuration:
	backends:$backends"
