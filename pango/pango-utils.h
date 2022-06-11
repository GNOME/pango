/*
 * Copyright (C) 2000 Red Hat Software
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <stdio.h>
#include <glib.h>
#include <pango/pango-font.h>

G_BEGIN_DECLS

/* Unicode characters that are zero-width and should not be rendered
 * normally.
 */
PANGO_AVAILABLE_IN_ALL
gboolean pango_is_zero_width (gunichar ch) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
void     pango_find_paragraph_boundary (const char *text,
                                        int         length,
                                        int        *paragraph_delimiter_index,
                                        int        *next_paragraph_start);

/* Pango version checking */

/* Encode a Pango version as an integer */
/**
 * PANGO_VERSION_ENCODE:
 * @major: the major component of the version number
 * @minor: the minor component of the version number
 * @micro: the micro component of the version number
 *
 * This macro encodes the given Pango version into an integer.  The numbers
 * returned by %PANGO_VERSION and pango_version() are encoded using this macro.
 * Two encoded version numbers can be compared as integers.
 */
#define PANGO_VERSION_ENCODE(major, minor, micro) (     \
          ((major) * 10000)                             \
        + ((minor) *   100)                             \
        + ((micro) *     1))

/* Encoded version of Pango at compile-time */
/**
 * PANGO_VERSION:
 *
 * The version of Pango available at compile-time, encoded using PANGO_VERSION_ENCODE().
 */
/**
 * PANGO_VERSION_STRING:
 *
 * A string literal containing the version of Pango available at compile-time.
 */
/**
 * PANGO_VERSION_MAJOR:
 *
 * The major component of the version of Pango available at compile-time.
 */
/**
 * PANGO_VERSION_MINOR:
 *
 * The minor component of the version of Pango available at compile-time.
 */
/**
 * PANGO_VERSION_MICRO:
 *
 * The micro component of the version of Pango available at compile-time.
 */
#define PANGO_VERSION PANGO_VERSION_ENCODE(     \
        PANGO_VERSION_MAJOR,                    \
        PANGO_VERSION_MINOR,                    \
        PANGO_VERSION_MICRO)

/* Check that compile-time Pango is as new as required */
/**
 * PANGO_VERSION_CHECK:
 * @major: the major component of the version number
 * @minor: the minor component of the version number
 * @micro: the micro component of the version number
 *
 * Checks that the version of Pango available at compile-time is not older than
 * the provided version number.
 */
#define PANGO_VERSION_CHECK(major,minor,micro)    \
        (PANGO_VERSION >= PANGO_VERSION_ENCODE(major,minor,micro))


/* Return encoded version of Pango at run-time */
PANGO_AVAILABLE_IN_ALL
int pango_version (void) G_GNUC_CONST;

/* Return run-time Pango version as an string */
PANGO_AVAILABLE_IN_ALL
const char * pango_version_string (void) G_GNUC_CONST;

/* Check that run-time Pango is as new as required */
PANGO_AVAILABLE_IN_ALL
const char * pango_version_check (int required_major,
                                  int required_minor,
                                  int required_micro) G_GNUC_CONST;

G_END_DECLS
