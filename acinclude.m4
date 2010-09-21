
# Checks the location of the XML Catalog
# Usage:
#   JH_PATH_XML_CATALOG([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
# Defines XMLCATALOG and XML_CATALOG_FILE substitutions
AC_DEFUN([JH_PATH_XML_CATALOG],
[
  # check for the presence of the XML catalog
  AC_ARG_WITH([xml-catalog],
              AC_HELP_STRING([--with-xml-catalog=CATALOG],
                             [path to xml catalog to use]),,
              [with_xml_catalog=/etc/xml/catalog])
  jh_found_xmlcatalog=true
  XML_CATALOG_FILE="$with_xml_catalog"
  AC_SUBST([XML_CATALOG_FILE])
  AC_MSG_CHECKING([for XML catalog ($XML_CATALOG_FILE)])
  if test -f "$XML_CATALOG_FILE"; then
    AC_MSG_RESULT([found])
  else
    jh_found_xmlcatalog=false
    AC_MSG_RESULT([not found])
  fi

  # check for the xmlcatalog program
  AC_PATH_PROG(XMLCATALOG, xmlcatalog, no)
  if test "x$XMLCATALOG" = xno; then
    jh_found_xmlcatalog=false
  fi

  if $jh_found_xmlcatalog; then
    ifelse([$1],,[:],[$1])
  else
    ifelse([$2],,[AC_MSG_ERROR([could not find XML catalog])],[$2])
  fi
])

# Checks if a particular URI appears in the XML catalog
# Usage:
#   JH_CHECK_XML_CATALOG(URI, [FRIENDLY-NAME], [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
AC_DEFUN([JH_CHECK_XML_CATALOG],
[
  AC_REQUIRE([JH_PATH_XML_CATALOG],[JH_PATH_XML_CATALOG(,[:])])dnl
  AC_MSG_CHECKING([for ifelse([$2],,[$1],[$2]) in XML catalog])
  if $jh_found_xmlcatalog && \
     AC_RUN_LOG([$XMLCATALOG --noout "$XML_CATALOG_FILE" "$1" >&2]); then
    AC_MSG_RESULT([found])
    ifelse([$3],,,[$3
])dnl
  else
    AC_MSG_RESULT([not found])
    ifelse([$4],,
       [AC_MSG_ERROR([could not find ifelse([$2],,[$1],[$2]) in XML catalog])],
       [$4])
  fi
])


dnl REMOVE THIS WHEN introspection.m4 is widely available
dnl
dnl -*- mode: autoconf -*-
dnl Copyright 2009 Johan Dahlin
dnl
dnl This file is free software; the author(s) gives unlimited
dnl permission to copy and/or distribute it, with or without
dnl modifications, as long as this notice is preserved.
dnl

# serial 1

m4_define([_GOBJECT_INTROSPECTION_CHECK_INTERNAL],
[
    AC_BEFORE([AC_PROG_LIBTOOL],[$0])dnl setup libtool first
    AC_BEFORE([AM_PROG_LIBTOOL],[$0])dnl setup libtool first
    AC_BEFORE([LT_INIT],[$0])dnl setup libtool first

    dnl enable/disable introspection
    m4_if([$2], [require],
    [dnl
        enable_introspection=yes
    ],[dnl
        AC_ARG_ENABLE(introspection,
                  AS_HELP_STRING([--enable-introspection[=@<:@no/auto/yes@:>@]],
                                 [Enable introspection for this build]),, 
                                 [enable_introspection=auto])
    ])dnl

    AC_MSG_CHECKING([for gobject-introspection])

    dnl presence/version checking
    AS_CASE([$enable_introspection],
    [no], [dnl
        found_introspection="no (disabled, use --enable-introspection to enable)"
    ],dnl
    [yes],[dnl
        PKG_CHECK_EXISTS([gobject-introspection-1.0],,
                         AC_MSG_ERROR([gobject-introspection-1.0 is not installed]))
        PKG_CHECK_EXISTS([gobject-introspection-1.0 >= $1],
                         found_introspection=yes,
                         AC_MSG_ERROR([You need to have gobject-introspection >= $1 installed to build AC_PACKAGE_NAME]))
    ],dnl
    [auto],[dnl
        PKG_CHECK_EXISTS([gobject-introspection-1.0 >= $1], found_introspection=yes, found_introspection=no)
    ],dnl
    [dnl	
        AC_MSG_ERROR([invalid argument passed to --enable-introspection, should be one of @<:@no/auto/yes@:>@])
    ])dnl

    AC_MSG_RESULT([$found_introspection])

    INTROSPECTION_SCANNER=
    INTROSPECTION_COMPILER=
    INTROSPECTION_GENERATE=
    INTROSPECTION_GIRDIR=
    INTROSPECTION_TYPELIBDIR=
    if test "x$found_introspection" = "xyes"; then
       INTROSPECTION_SCANNER=`$PKG_CONFIG --variable=g_ir_scanner gobject-introspection-1.0`
       INTROSPECTION_COMPILER=`$PKG_CONFIG --variable=g_ir_compiler gobject-introspection-1.0`
       INTROSPECTION_GENERATE=`$PKG_CONFIG --variable=g_ir_generate gobject-introspection-1.0`
       INTROSPECTION_GIRDIR=`$PKG_CONFIG --variable=girdir gobject-introspection-1.0`
       INTROSPECTION_TYPELIBDIR="$($PKG_CONFIG --variable=typelibdir gobject-introspection-1.0)"
       INTROSPECTION_CFLAGS=`$PKG_CONFIG --cflags gobject-introspection-1.0`
       INTROSPECTION_LIBS=`$PKG_CONFIG --libs gobject-introspection-1.0`
       INTROSPECTION_MAKEFILE=`$PKG_CONFIG --variable=datadir gobject-introspection-1.0`/gobject-introspection-1.0/Makefile.introspection
    fi
    AC_SUBST(INTROSPECTION_SCANNER)
    AC_SUBST(INTROSPECTION_COMPILER)
    AC_SUBST(INTROSPECTION_GENERATE)
    AC_SUBST(INTROSPECTION_GIRDIR)
    AC_SUBST(INTROSPECTION_TYPELIBDIR)
    AC_SUBST(INTROSPECTION_CFLAGS)
    AC_SUBST(INTROSPECTION_LIBS)
    AC_SUBST(INTROSPECTION_MAKEFILE)

    AM_CONDITIONAL(HAVE_INTROSPECTION, test "x$found_introspection" = "xyes")
])


dnl Usage:
dnl   GOBJECT_INTROSPECTION_CHECK([minimum-g-i-version])

AC_DEFUN([GOBJECT_INTROSPECTION_CHECK],
[
  _GOBJECT_INTROSPECTION_CHECK_INTERNAL([$1])
])

dnl Usage:
dnl   GOBJECT_INTROSPECTION_REQUIRE([minimum-g-i-version])


AC_DEFUN([GOBJECT_INTROSPECTION_REQUIRE],
[
  _GOBJECT_INTROSPECTION_CHECK_INTERNAL([$1], [require])
])
