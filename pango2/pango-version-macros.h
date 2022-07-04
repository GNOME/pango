/*
 * Copyright (C) 1999 Red Hat Software
 * Copyright (C) 2012 Ryan Lortie, Matthias Clasen and Emmanuele Bassi
 * Copyright (C) 2016 Chun-wei Fan
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <pango2/pango-features.h>

#include <glib.h>

#ifndef _PANGO2_EXTERN
#define _PANGO2_EXTERN extern
#endif

#define PANGO2_AVAILABLE_IN_ALL                   _PANGO2_EXTERN

/* XXX: Every new stable minor release bump should add a macro here */

/**
 * PANGO2_VERSION_0_90
 *
 * A macro that evaluates to the 0.90 version of Pango2, in a format
 * that can be used by the C pre-processor.
 */
#define PANGO2_VERSION_0_90       (G_ENCODE_VERSION (0, 90))

/**
 * PANGO2_VERSION_1_0
 *
 * A macro that evaluates to the 1.0 version of Pango2, in a format
 * that can be used by the C pre-processor.
 */
#define PANGO2_VERSION_1_0       (G_ENCODE_VERSION (1, 0))

/* evaluates to the current stable version; for development cycles,
 * this means the next stable target
 */
#if (PANGO2_VERSION_MINOR % 2)
#define PANGO2_VERSION_CUR_STABLE         (G_ENCODE_VERSION (PANGO2_VERSION_MAJOR, PANGO2_VERSION_MINOR + 1))
#else
#define PANGO2_VERSION_CUR_STABLE         (G_ENCODE_VERSION (PANGO2_VERSION_MAJOR, PANGO2_VERSION_MINOR))
#endif

/* evaluates to the previous stable version */
#if (PANGO2_VERSION_MINOR % 2)
#define PANGO2_VERSION_PREV_STABLE        (G_ENCODE_VERSION (PANGO2_VERSION_MAJOR, PANGO2_VERSION_MINOR - 1))
#else
#define PANGO2_VERSION_PREV_STABLE        (G_ENCODE_VERSION (PANGO2_VERSION_MAJOR, PANGO2_VERSION_MINOR - 2))
#endif

/**
 * PANGO2_VERSION_MIN_REQUIRED:
 *
 * A macro that should be defined by the user prior to including
 * the pango.h header.
 * The definition should be one of the predefined Pango2 version
 * macros: %PANGO2_VERSION_2_0,…
 *
 * This macro defines the earliest version of Pango2 that the package is
 * required to be able to compile against.
 *
 * If the compiler is configured to warn about the use of deprecated
 * functions, then using functions that were deprecated in version
 * %PANGO2_VERSION_MIN_REQUIRED or earlier will cause warnings (but
 * using functions deprecated in later releases will not).
 */
/* If the package sets PANGO2_VERSION_MIN_REQUIRED to some future
 * PANGO2_VERSION_X_Y value that we don't know about, it will compare as
 * 0 in preprocessor tests.
 */
#ifndef PANGO2_VERSION_MIN_REQUIRED
# define PANGO2_VERSION_MIN_REQUIRED      (PANGO2_VERSION_CUR_STABLE)
#elif PANGO2_VERSION_MIN_REQUIRED == 0
# undef  PANGO2_VERSION_MIN_REQUIRED
# define PANGO2_VERSION_MIN_REQUIRED      (PANGO2_VERSION_CUR_STABLE + 2)
#endif

/**
 * PANGO2_VERSION_MAX_ALLOWED:
 *
 * A macro that should be defined by the user prior to including
 * the glib.h header.
 * The definition should be one of the predefined Pango2 version
 * macros: %PANGO2_VERSION_2_0,…
 *
 * This macro defines the latest version of the Pango2 API that the
 * package is allowed to make use of.
 *
 * If the compiler is configured to warn about the use of deprecated
 * functions, then using functions added after version
 * %PANGO2_VERSION_MAX_ALLOWED will cause warnings.
 *
 * Unless you are using PANGO2_VERSION_CHECK() or the like to compile
 * different code depending on the Pango2 version, then this should be
 * set to the same value as %PANGO2_VERSION_MIN_REQUIRED.
 */
#if !defined (PANGO2_VERSION_MAX_ALLOWED) || (PANGO2_VERSION_MAX_ALLOWED == 0)
# undef PANGO2_VERSION_MAX_ALLOWED
# define PANGO2_VERSION_MAX_ALLOWED      (PANGO2_VERSION_CUR_STABLE)
#endif

/* sanity checks */
#if PANGO2_VERSION_MIN_REQUIRED > PANGO2_VERSION_CUR_STABLE
#error "PANGO2_VERSION_MIN_REQUIRED must be <= PANGO2_VERSION_CUR_STABLE"
#endif
#if PANGO2_VERSION_MAX_ALLOWED < PANGO2_VERSION_MIN_REQUIRED
#error "PANGO2_VERSION_MAX_ALLOWED must be >= PANGO2_VERSION_MIN_REQUIRED"
#endif
#if PANGO2_VERSION_MIN_REQUIRED < PANGO2_VERSION_0_90
#error "PANGO2_VERSION_MIN_REQUIRED must be >= PANGO2_VERSION_0_90"
#endif

/* These macros are used to mark deprecated functions in Pango2 headers,
 * and thus have to be exposed in installed headers.
 */
#ifdef PANGO2_DISABLE_DEPRECATION_WARNINGS
# define PANGO2_DEPRECATED                       _PANGO2_EXTERN
# define PANGO2_DEPRECATED_FOR(f)                _PANGO2_EXTERN
# define PANGO2_UNAVAILABLE(maj,min)             _PANGO2_EXTERN
#else
# define PANGO2_DEPRECATED                       G_DEPRECATED _PANGO2_EXTERN
# define PANGO2_DEPRECATED_FOR(f)                G_DEPRECATED_FOR(f) _PANGO2_EXTERN
# define PANGO2_UNAVAILABLE(maj,min)             G_UNAVAILABLE(maj,min) _PANGO2_EXTERN
#endif

/* XXX: Every new stable minor release should add a set of macros here */

#if PANGO2_VERSION_MIN_REQUIRED >= PANGO2_VERSION_1_0
# define PANGO2_DEPRECATED_IN_1_0                PANGO2_DEPRECATED
# define PANGO2_DEPRECATED_IN_1_0_FOR(f)         PANGO2_DEPRECATED_FOR(f)
#else
# define PANGO2_DEPRECATED_IN_1_0                _PANGO2_EXTERN
# define PANGO2_DEPRECATED_IN_1_0_FOR(f)         _PANGO2_EXTERN
#endif

#if PANGO2_VERSION_MAX_ALLOWED < PANGO2_VERSION_1_0
# define PANGO2_AVAILABLE_IN_1_0                 PANGO2_UNAVAILABLE(2, 0)
#else
# define PANGO2_AVAILABLE_IN_1_0                 _PANGO2_EXTERN
#endif
