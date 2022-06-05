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

typedef struct _PangoMatrix    PangoMatrix;

/**
 * PangoMatrix:
 * @xx: 1st component of the transformation matrix
 * @xy: 2nd component of the transformation matrix
 * @yx: 3rd component of the transformation matrix
 * @yy: 4th component of the transformation matrix
 * @x0: x translation
 * @y0: y translation
 *
 * A `PangoMatrix` specifies a transformation between user-space
 * and device coordinates.
 *
 * The transformation is given by
 *
 * ```
 * x_device = x_user * matrix->xx + y_user * matrix->xy + matrix->x0;
 * y_device = x_user * matrix->yx + y_user * matrix->yy + matrix->y0;
 * ```
 */
struct _PangoMatrix
{
  double xx;
  double xy;
  double yx;
  double yy;
  double x0;
  double y0;
};

#define PANGO_TYPE_MATRIX (pango_matrix_get_type ())

/**
 * PANGO_MATRIX_INIT:
 *
 * Constant that can be used to initialize a `PangoMatrix` to
 * the identity transform.
 *
 * ```
 * PangoMatrix matrix = PANGO_MATRIX_INIT;
 * pango_matrix_rotate (&matrix, 45.);
 * ```
 */
#define PANGO_MATRIX_INIT { 1., 0., 0., 1., 0., 0. }

/* for PangoRectangle */
#include <pango/pango-types.h>

PANGO_AVAILABLE_IN_ALL
GType pango_matrix_get_type (void) G_GNUC_CONST;

PANGO_AVAILABLE_IN_ALL
PangoMatrix *pango_matrix_copy   (const PangoMatrix *matrix);
PANGO_AVAILABLE_IN_ALL
void         pango_matrix_free   (PangoMatrix *matrix);

PANGO_AVAILABLE_IN_ALL
gboolean     pango_matrix_equal (const PangoMatrix *m1,
                                 const PangoMatrix *m2);

PANGO_AVAILABLE_IN_ALL
void pango_matrix_translate (PangoMatrix *matrix,
                             double       tx,
                             double       ty);
PANGO_AVAILABLE_IN_ALL
void pango_matrix_scale     (PangoMatrix *matrix,
                             double       scale_x,
                             double       scale_y);
PANGO_AVAILABLE_IN_ALL
void pango_matrix_rotate    (PangoMatrix *matrix,
                             double       degrees);
PANGO_AVAILABLE_IN_ALL
void pango_matrix_concat    (PangoMatrix       *matrix,
                             const PangoMatrix *new_matrix);
PANGO_AVAILABLE_IN_ALL
void pango_matrix_transform_point    (const PangoMatrix *matrix,
                                      double            *x,
                                      double            *y);
PANGO_AVAILABLE_IN_ALL
void pango_matrix_transform_distance (const PangoMatrix *matrix,
                                      double            *dx,
                                      double            *dy);
PANGO_AVAILABLE_IN_ALL
void pango_matrix_transform_rectangle (const PangoMatrix *matrix,
                                       PangoRectangle    *rect);
PANGO_AVAILABLE_IN_ALL
void pango_matrix_transform_pixel_rectangle (const PangoMatrix *matrix,
                                             PangoRectangle    *rect);
PANGO_AVAILABLE_IN_ALL
double pango_matrix_get_font_scale_factor (const PangoMatrix *matrix) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
void pango_matrix_get_font_scale_factors (const PangoMatrix *matrix,
                                          double *xscale, double *yscale);
PANGO_AVAILABLE_IN_ALL
double pango_matrix_get_rotation (const PangoMatrix *matrix) G_GNUC_PURE;
PANGO_AVAILABLE_IN_ALL
double pango_matrix_get_slant_ratio (const PangoMatrix *matrix) G_GNUC_PURE;


G_END_DECLS
