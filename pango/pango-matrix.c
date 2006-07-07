/* Pango
 * pango-matrix.c: Matrix manipulation routines
 *
 * Copyright (C) 2000 Red Hat Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <math.h>

#include "pango-types.h"
#include "pango-impl-utils.h"

GType
pango_matrix_get_type (void)
{
  static GType our_type = 0;
  
  if (our_type == 0)
    our_type = g_boxed_type_register_static (I_("PangoMatrix"),
					     (GBoxedCopyFunc) pango_matrix_copy,
					     (GBoxedFreeFunc) pango_matrix_free);

  return our_type;
}

/**
 * pango_matrix_copy:
 * @matrix: a #PangoMatrix, can be %NULL
 * 
 * Copies a #PangoMatrix.
 * 
 * Return value: the newly allocated #PangoMatrix, which should
 *               be freed with pango_matrix_free(), or %NULL if
 *               @matrix was %NULL.
 *
 * Since: 1.6
 **/
PangoMatrix *
pango_matrix_copy (const PangoMatrix *matrix)
{
  PangoMatrix *new_matrix;

  if (matrix)
    {
      new_matrix = g_slice_new (PangoMatrix);
      *new_matrix = *matrix;
    }
  else
    new_matrix = NULL;

  return new_matrix;
}

/**
 * pango_matrix_free:
 * @matrix: a #PangoMatrix, or %NULL
 * 
 * Free a #PangoMatrix created with pango_matrix_copy().
 * Does nothing if @matrix is %NULL.
 *
 * Since: 1.6
 **/
void
pango_matrix_free (PangoMatrix *matrix)
{
  if (matrix)
    g_slice_free (PangoMatrix, matrix);
}

/**
 * pango_matrix_translate:
 * @matrix: a #PangoMatrix
 * @tx: amount to translate in the X direction
 * @ty: amount to translate in the Y direction
 * 
 * Changes the transformation represented by @matrix to be the
 * transformation given by first translating by (@tx, @ty)
 * then applying the original transformation.
 *
 * Since: 1.6
 **/
void
pango_matrix_translate (PangoMatrix *matrix,
			double       tx,
			double       ty)
{
  g_return_if_fail (matrix != NULL);

  matrix->x0  = matrix->xx * tx + matrix->xy * ty + matrix->x0;
  matrix->y0  = matrix->yx * tx + matrix->yy * ty + matrix->y0;
}

/**
 * pango_matrix_scale:
 * @matrix: a #PangoMatrix
 * @scale_x: amount to scale by in X direction
 * @scale_y: amount to scale by in Y direction
 * 
 * Changes the transformation represented by @matrix to be the
 * transformation given by first scaling by @sx in the X direction
 * and @sy in the Y direction then applying the original
 * transformation.
 *
 * Since: 1.6
 **/
void
pango_matrix_scale (PangoMatrix *matrix,
		    double       scale_x,
		    double       scale_y)
{
  g_return_if_fail (matrix != NULL);

  matrix->xx *= scale_x;
  matrix->xy *= scale_y;
  matrix->yx *= scale_x;
  matrix->yy *= scale_y;
}

/**
 * pango_matrix_rotate:
 * @matrix: a #PangoMatrix
 * @degrees: degrees to rotate counter-clockwise
 * 
 * Changes the transformation represented by @matrix to be the
 * transformation given by first rotating by @degrees degrees
 * counter-clokwise then applying the original transformation.
 *
 * Since: 1.6
 **/
void
pango_matrix_rotate (PangoMatrix *matrix,
		     double       degrees)
{
  PangoMatrix tmp;
  gdouble r, s, c;

  g_return_if_fail (matrix != NULL);

  r = degrees * (G_PI / 180.);
  s = sin (r);
  c = cos (r);

  tmp.xx = c;
  tmp.xy = s;
  tmp.yx = -s;
  tmp.yy = c;
  tmp.x0 = 0;
  tmp.y0 = 0;

  pango_matrix_concat (matrix, &tmp);
}

/**
 * pango_matrix_concat:
 * @matrix: a #PangoMatrix
 * @new_matrix: a #PangoMatrix
 * 
 * Changes the transformation represented by @matrix to be the
 * transformation given by first applying transformation
 * given by @new_matrix then applying the original transformation.
 *
 * Since: 1.6
 **/
void
pango_matrix_concat (PangoMatrix       *matrix,
		     const PangoMatrix *new_matrix)
{
  PangoMatrix tmp;
  
  g_return_if_fail (matrix != NULL);

  tmp = *matrix;

  matrix->xx = tmp.xx * new_matrix->xx + tmp.xy * new_matrix->yx;
  matrix->xy = tmp.xx * new_matrix->xy + tmp.xy * new_matrix->yy;
  matrix->yx = tmp.yx * new_matrix->xx + tmp.yy * new_matrix->yx;
  matrix->yy = tmp.yx * new_matrix->xy + tmp.yy * new_matrix->yy;
  matrix->x0  = tmp.xx * new_matrix->x0 + tmp.xy * new_matrix->y0 + tmp.x0;
  matrix->y0  = tmp.yx * new_matrix->y0 + tmp.yy * new_matrix->y0 + tmp.y0;
}

/**
 * pango_matrix_get_font_scale_factor:
 * @matrix: a #PangoMatrix, may be %NULL
 * 
 * Returns the scale factor of a matrix on the height of the font.
 * That is, the scale factor in the direction perpendicular to the
 * vector that the X coordinate is mapped to.
 *
 * Return value: the scale factor of @matrix on the height of the font,
 * or 1.0 if @matrix is %NULL.
 *
 * Since: 1.12
 **/
double
pango_matrix_get_font_scale_factor (const PangoMatrix *matrix)
{
/*
 * Based on cairo-matrix.c:_cairo_matrix_compute_scale_factors()
 *
 * Copyright 2005, Keith Packard
 */
  double det;
  
  if (!matrix)
    return 1.0;
  
  det = matrix->xx * matrix->yy - matrix->yx * matrix->xy;

  if (det == 0)
    {
      return 0.0;
    }
  else
    {
      double x = matrix->xx;
      double y = matrix->yx;
      double major, minor;

      major = sqrt (x*x + y*y);
      
      /*
       * ignore mirroring
       */
      if (det < 0)
	det = - det;
      
      if (major)
	minor = det / major;
      else 
	minor = 0.0;

      return minor;
    }
}
