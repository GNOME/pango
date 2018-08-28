#!/bin/sh
# Run this to generate all the initial makefiles, etc.

test -n "$srcdir" || srcdir=`dirname "$0"`
test -n "$srcdir" || srcdir=.

olddir=`pwd`
cd "$srcdir"

if ! command -v gtkdocize >/dev/null 2>&1; then
	echo "*** No GTK-Doc found, please install it ***"
	exit 1
else
	gtkdocize || exit $?
fi

if ! command -v autoreconf >/dev/null 2>&1; then
	echo "*** No autoreconf found, please install it ***"
	exit 1
else
	ACLOCAL="aclocal $ACLOCAL_FLAGS" autoreconf --force --install || exit $?
fi


cd "$olddir"
test -n "$NOCONFIGURE" || "$srcdir/configure" "$@"
