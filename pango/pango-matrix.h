/*
 * Copyright (C) 2002, 2006 Red Hat Software
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

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _Pango2Matrix    Pango2Matrix;

/**
 * Pango2Matrix:
 * @xx: 1st component of the transformation matrix
 * @xy: 2nd component of the transformation matrix
 * @yx: 3rd component of the transformation matrix
 * @yy: 4th component of the transformation matrix
 * @x0: x translation
 * @y0: y translation
 *
 * A `Pango2Matrix` specifies a transformation between user-space
 * and device coordinates.
 *
 * The transformation is given by
 *
 * ```
 * x_device = x_user * matrix->xx + y_user * matrix->xy + matrix->x0;
 * y_device = x_user * matrix->yx + y_user * matrix->yy + matrix->y0;
 * ```
 */
struct _Pango2Matrix
{
  double xx;
  double xy;
  double yx;
  double yy;
  double x0;
  double y0;
};

#define PANGO2_TYPE_MATRIX (pango2_matrix_get_type ())

/**
 * PANGO2_MATRIX_INIT:
 *
 * Constant that can be used to initialize a `Pango2Matrix` to
 * the identity transform.
 *
 * ```
 * Pango2Matrix matrix = PANGO2_MATRIX_INIT;
 * pango2_matrix_rotate (&matrix, 45.);
 * ```
 */
#define PANGO2_MATRIX_INIT { 1., 0., 0., 1., 0., 0. }

/* for Pango2Rectangle */
#include <pango/pango-types.h>

PANGO2_AVAILABLE_IN_ALL
GType                   pango2_matrix_get_type                  (void) G_GNUC_CONST;

PANGO2_AVAILABLE_IN_ALL
Pango2Matrix *          pango2_matrix_copy                      (const Pango2Matrix *matrix);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_matrix_free                      (Pango2Matrix       *matrix);

PANGO2_AVAILABLE_IN_ALL
gboolean                pango2_matrix_equal                     (const Pango2Matrix *m1,
                                                                 const Pango2Matrix *m2);

PANGO2_AVAILABLE_IN_ALL
void                    pango2_matrix_translate                 (Pango2Matrix       *matrix,
                                                                 double              tx,
                                                                 double              ty);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_matrix_scale                     (Pango2Matrix       *matrix,
                                                                 double              scale_x,
                                                                 double              scale_y);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_matrix_rotate                    (Pango2Matrix       *matrix,
                                                                 double              degrees);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_matrix_concat                    (Pango2Matrix       *matrix,
                                                                 const Pango2Matrix *new_matrix);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_matrix_transform_point           (const Pango2Matrix *matrix,
                                                                 double            *x,
                                                                 double            *y);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_matrix_transform_distance        (const Pango2Matrix *matrix,
                                                                 double            *dx,
                                                                 double            *dy);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_matrix_transform_rectangle       (const Pango2Matrix *matrix,
                                                                 Pango2Rectangle    *rect);
PANGO2_AVAILABLE_IN_ALL
void                    pango2_matrix_transform_pixel_rectangle (const Pango2Matrix *matrix,
                                                                 Pango2Rectangle    *rect);
PANGO2_AVAILABLE_IN_ALL
double                  pango2_matrix_get_font_scale_factor     (const Pango2Matrix *matrix) G_GNUC_PURE;
PANGO2_AVAILABLE_IN_ALL
void                    pango2_matrix_get_font_scale_factors    (const Pango2Matrix *matrix,
                                                                 double             *xscale,
                                                                 double             *yscale);
PANGO2_AVAILABLE_IN_ALL
double                  pango2_matrix_get_rotation              (const Pango2Matrix *matrix) G_GNUC_PURE;
PANGO2_AVAILABLE_IN_ALL
double                  pango2_matrix_get_slant_ratio           (const Pango2Matrix *matrix) G_GNUC_PURE;

G_DEFINE_AUTOPTR_CLEANUP_FUNC(Pango2Matrix, pango2_matrix_free)


G_END_DECLS
