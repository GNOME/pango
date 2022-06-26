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
#include <pango2/pango-features.h>
#include <pango2/pango-font.h>

G_BEGIN_DECLS

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_is_zero_width           (gunichar ch) G_GNUC_CONST;

PANGO2_AVAILABLE_IN_ALL
void                    pango2_find_paragraph_boundary (const char *text,
                                                        int         length,
                                                        int        *paragraph_delimiter_index,
                                                        int        *next_paragraph_start);

/* Encode a Pango2 version as an integer */
/* Pango2 version checking */

/**
 * PANGO2_VERSION_ENCODE:
 * @major: the major component of the version number
 * @minor: the minor component of the version number
 * @micro: the micro component of the version number
 *
 * This macro encodes the given Pango2 version into an integer.  The numbers
 * returned by %PANGO2_VERSION and pango2_version() are encoded using this macro.
 * Two encoded version numbers can be compared as integers.
 */
#define PANGO2_VERSION_ENCODE(major, minor, micro) (     \
          ((major) * 10000)                             \
        + ((minor) *   100)                             \
        + ((micro) *     1))

/* Encoded version of Pango2 at compile-time */
/**
 * PANGO2_VERSION:
 *
 * The version of Pango2 available at compile-time, encoded using PANGO2_VERSION_ENCODE().
 */

/**
 * PANGO2_VERSION_STRING:
 *
 * A string literal containing the version of Pango2 available at compile-time.
 */

/**
 * PANGO2_VERSION_MAJOR:
 *
 * The major component of the version of Pango2 available at compile-time.
 */
/**
 * PANGO2_VERSION_MINOR:
 *
 * The minor component of the version of Pango2 available at compile-time.
 */
/**
 * PANGO2_VERSION_MICRO:
 *
 * The micro component of the version of Pango2 available at compile-time.
 */
#define PANGO2_VERSION PANGO2_VERSION_ENCODE(     \
        PANGO2_VERSION_MAJOR,                    \
        PANGO2_VERSION_MINOR,                    \
        PANGO2_VERSION_MICRO)


/* Check that compile-time Pango2 is as new as required */
/**
 * PANGO2_VERSION_CHECK:
 * @major: the major component of the version number
 * @minor: the minor component of the version number
 * @micro: the micro component of the version number
 *
 * Checks that the version of Pango2 available at compile-time is not older than
 * the provided version number.
 */
#define PANGO2_VERSION_CHECK(major,minor,micro)    \
        (PANGO2_VERSION >= PANGO2_VERSION_ENCODE(major,minor,micro))

PANGO2_AVAILABLE_IN_ALL
int                     pango2_version        (void) G_GNUC_CONST;

PANGO2_AVAILABLE_IN_ALL
const char *            pango2_version_string (void) G_GNUC_CONST;

PANGO2_AVAILABLE_IN_ALL
const char *            pango2_version_check  (int required_major,
                                               int required_minor,
                                               int required_micro) G_GNUC_CONST;

G_END_DECLS
