/* Pango - Internationalized text layout and rendering library
 * Copyright (C) 1999 Red Hat Software
 * Copyright (C) 2012 Ryan Lortie, Matthias Clasen and Emmanuele Bassi
 * Copyright (C) 2016 Chun-wei Fan
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

#ifndef __PANGO_VERSION_H__
#define __PANGO_VERSION_H__

#include <pango/pango-features.h>

#include <glib.h>

#ifndef _PANGO_EXTERN
#define _PANGO_EXTERN extern
#endif

#define PANGO_AVAILABLE_IN_ALL                   _PANGO_EXTERN

/* XXX: Every new stable minor release bump should add a macro here */

/**
 * PANGO_VERSION_1_90
 *
 * A macro that evaluates to the 1.90 version of Pango, in a format
 * that can be used by the C pre-processor.
 */
#define PANGO_VERSION_1_90       (G_ENCODE_VERSION (1, 90))

/**
 * PANGO_VERSION_2_0
 *
 * A macro that evaluates to the 2.0 version of Pango, in a format
 * that can be used by the C pre-processor.
 */
#define PANGO_VERSION_2_0       (G_ENCODE_VERSION (2, 0))

/* evaluates to the current stable version; for development cycles,
 * this means the next stable target
 */
#if (PANGO_VERSION_MINOR % 2)
#define PANGO_VERSION_CUR_STABLE         (G_ENCODE_VERSION (PANGO_VERSION_MAJOR, PANGO_VERSION_MINOR + 1))
#else
#define PANGO_VERSION_CUR_STABLE         (G_ENCODE_VERSION (PANGO_VERSION_MAJOR, PANGO_VERSION_MINOR))
#endif

/* evaluates to the previous stable version */
#if (PANGO_VERSION_MINOR % 2)
#define PANGO_VERSION_PREV_STABLE        (G_ENCODE_VERSION (PANGO_VERSION_MAJOR, PANGO_VERSION_MINOR - 1))
#else
#define PANGO_VERSION_PREV_STABLE        (G_ENCODE_VERSION (PANGO_VERSION_MAJOR, PANGO_VERSION_MINOR - 2))
#endif

/**
 * PANGO_VERSION_MIN_REQUIRED:
 *
 * A macro that should be defined by the user prior to including
 * the pango.h header.
 * The definition should be one of the predefined Pango version
 * macros: %PANGO_VERSION_2_0,…
 *
 * This macro defines the earliest version of Pango that the package is
 * required to be able to compile against.
 *
 * If the compiler is configured to warn about the use of deprecated
 * functions, then using functions that were deprecated in version
 * %PANGO_VERSION_MIN_REQUIRED or earlier will cause warnings (but
 * using functions deprecated in later releases will not).
 */
/* If the package sets PANGO_VERSION_MIN_REQUIRED to some future
 * PANGO_VERSION_X_Y value that we don't know about, it will compare as
 * 0 in preprocessor tests.
 */
#ifndef PANGO_VERSION_MIN_REQUIRED
# define PANGO_VERSION_MIN_REQUIRED      (PANGO_VERSION_CUR_STABLE)
#elif PANGO_VERSION_MIN_REQUIRED == 0
# undef  PANGO_VERSION_MIN_REQUIRED
# define PANGO_VERSION_MIN_REQUIRED      (PANGO_VERSION_CUR_STABLE + 2)
#endif

/**
 * PANGO_VERSION_MAX_ALLOWED:
 *
 * A macro that should be defined by the user prior to including
 * the glib.h header.
 * The definition should be one of the predefined Pango version
 * macros: %PANGO_VERSION_2_0,…
 *
 * This macro defines the latest version of the Pango API that the
 * package is allowed to make use of.
 *
 * If the compiler is configured to warn about the use of deprecated
 * functions, then using functions added after version
 * %PANGO_VERSION_MAX_ALLOWED will cause warnings.
 *
 * Unless you are using PANGO_VERSION_CHECK() or the like to compile
 * different code depending on the Pango version, then this should be
 * set to the same value as %PANGO_VERSION_MIN_REQUIRED.
 */
#if !defined (PANGO_VERSION_MAX_ALLOWED) || (PANGO_VERSION_MAX_ALLOWED == 0)
# undef PANGO_VERSION_MAX_ALLOWED
# define PANGO_VERSION_MAX_ALLOWED      (PANGO_VERSION_CUR_STABLE)
#endif

/* sanity checks */
#if PANGO_VERSION_MIN_REQUIRED > PANGO_VERSION_CUR_STABLE
#error "PANGO_VERSION_MIN_REQUIRED must be <= PANGO_VERSION_CUR_STABLE"
#endif
#if PANGO_VERSION_MAX_ALLOWED < PANGO_VERSION_MIN_REQUIRED
#error "PANGO_VERSION_MAX_ALLOWED must be >= PANGO_VERSION_MIN_REQUIRED"
#endif
#if PANGO_VERSION_MIN_REQUIRED < PANGO_VERSION_1_90
#error "PANGO_VERSION_MIN_REQUIRED must be >= PANGO_VERSION_1_90"
#endif

/* These macros are used to mark deprecated functions in Pango headers,
 * and thus have to be exposed in installed headers.
 */
#ifdef PANGO_DISABLE_DEPRECATION_WARNINGS
# define PANGO_DEPRECATED                       _PANGO_EXTERN
# define PANGO_DEPRECATED_FOR(f)                _PANGO_EXTERN
# define PANGO_UNAVAILABLE(maj,min)             _PANGO_EXTERN
#else
# define PANGO_DEPRECATED                       G_DEPRECATED _PANGO_EXTERN
# define PANGO_DEPRECATED_FOR(f)                G_DEPRECATED_FOR(f) _PANGO_EXTERN
# define PANGO_UNAVAILABLE(maj,min)             G_UNAVAILABLE(maj,min) _PANGO_EXTERN
#endif

/* XXX: Every new stable minor release should add a set of macros here */

#if PANGO_VERSION_MIN_REQUIRED >= PANGO_VERSION_2_0
# define PANGO_DEPRECATED_IN_2_0                PANGO_DEPRECATED
# define PANGO_DEPRECATED_IN_2_0_FOR(f)         PANGO_DEPRECATED_FOR(f)
#else
# define PANGO_DEPRECATED_IN_2_0                _PANGO_EXTERN
# define PANGO_DEPRECATED_IN_2_0_FOR(f)         _PANGO_EXTERN
#endif

#if PANGO_VERSION_MAX_ALLOWED < PANGO_VERSION_2_0
# define PANGO_AVAILABLE_IN_2_0                 PANGO_UNAVAILABLE(2, 0)
#else
# define PANGO_AVAILABLE_IN_2_0                 _PANGO_EXTERN
#endif

#endif /* __PANGO_VERSION_H__ */
