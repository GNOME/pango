/* Pango
 * pangoft2-render.c: Rendering routines to FT_Bitmap objects
 *
 * Copyright (C) 2004 Red Hat Software
 * Copyright (C) 2000 Tor Lillqvist
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

#include <math.h>

#include "pangoft2-private.h"

typedef struct {
  double y;
  double x1;
  double x2;
} Position;

static void
draw_simple_trap (FT_Bitmap *bitmap,
		  Position  *t,
		  Position  *b)
{
  int iy = floor (t->y);
  int x1, x2, x;
  double dy = b->y - t->y;
  guchar *dest;

  if (iy < 0 || iy >= bitmap->rows)
    return;
  dest = bitmap->buffer + iy * bitmap->pitch;

  if (t->x1 < b->x1)
    x1 = floor (t->x1);
  else
    x1 = floor (b->x1);

  if (t->x2 > b->x2)
    x2 = ceil (t->x2);
  else
    x2 = ceil (b->x2);

  x1 = CLAMP (x1, 0, bitmap->width);
  x2 = CLAMP (x2, 0, bitmap->width);

  for (x = x1; x < x2; x++)
    {
      double top_left = MAX (t->x1, x);
      double top_right = MIN (t->x2, x + 1);
      double bottom_left = MAX (b->x1, x);
      double bottom_right = MIN (b->x2, x + 1);
      double c = 0.5 * dy * ((top_right - top_left) + (bottom_right - bottom_left));

      /* When converting to [0,255], we round up. This is intended
       * to prevent the problem of pixels that get divided into
       * multiple slices not being fully black.
       */
      int ic = c * 256;

      dest[x] = MIN (dest[x] + ic, 255);
    }
}

static void
interpolate_position (Position *result,
		      Position *top,
		      Position *bottom,
		      double    val,
		      double    val1,
		      double    val2)
{
  result->y  = (top->y *  (val2 - val) + bottom->y *  (val - val1)) / (val2 - val1);
  result->x1 = (top->x1 * (val2 - val) + bottom->x1 * (val - val1)) / (val2 - val1);
  result->x2 = (top->x2 * (val2 - val) + bottom->x2 * (val - val1)) / (val2 - val1);
}

/* This draws a trapezoid with the parallel sides aligned with
 * the X axis. We do this by subdividing the trapezoid vertically
 * into thin slices (themselves trapezoids) where two edge sides are each
 * contained within a single pixel and then rasterizing each
 * slice. There are frequently multiple slices within a single
 * line so we have to accumulate to get the final result.
 */
static void
draw_trap (FT_Bitmap *bitmap,
	   double     y1,
	   double     x11,
	   double     x21,
	   double     y2,
	   double     x12,
	   double     x22)
{
  Position pos;
  Position t;
  Position b;
  gboolean done = FALSE;

  if (y1 == y2)
    return;

  pos.y = t.y = y1;
  pos.x1 = t.x1 = x11;
  pos.x2 = t.x2 = x21;
  b.y = y2;
  b.x1 = x12;
  b.x2 = x22;

  while (!done)
    {
      Position pos_next;
      double y_next, x1_next, x2_next;
      double ix1, ix2;

      /* The algorithm here is written to emphasize simplicity and
       * numerical stability as opposed to speed.
       *
       * While the end result is slicing up the polygon vertically,
       * conceptually we aren't walking in the X direction, rather we
       * are walking along the edges. When we compute crossing of
       * horizontal pixel boundaries, we use the X coordinate as the
       * interpolating variable, when we compute crossing for vertical
       * pixel boundaries, we use the Y coordinate.
       *
       * This allows us to handle almost exactly horizontal edges without
       * running into difficulties. (Almost exactly horizontal edges
       * come up frequently due to inexactness in computing, say,
       * a 90 degree rotation transformation)
       */

      pos_next = b;
      done = TRUE;

      /* Check for crossing vertical pixel boundaries */
      y_next = floor (pos.y) + 1;
      if (y_next < pos_next.y)
	{
	  interpolate_position (&pos_next, &t, &b,
				y_next, t.y, b.y);
	  pos_next.y = y_next;
	  done = FALSE;
	}

      /* Check left side for crossing horizontal pixel boundaries */
      ix1 = floor (pos.x1);

      if (b.x1 < t.x1)
	{
	  if (ix1 == pos.x1)
	    x1_next = ix1 - 1;
	  else
	    x1_next = ix1;

	  if (x1_next > pos_next.x1)
	    {
	      interpolate_position (&pos_next, &t, &b,
				    x1_next, t.x1, b.x1);
	      pos_next.x1 = x1_next;
	      done = FALSE;
	    }
	}
      else if (b.x1 > t.x1)
	{
	  x1_next = ix1 + 1;

	  if (x1_next < pos_next.x1)
	    {
	      interpolate_position (&pos_next, &t, &b,
				    x1_next, t.x1, b.x1);
	      pos_next.x1 = x1_next;
	      done = FALSE;
	    }
	}

      /* Check right side for crossing horizontal pixel boundaries */
      ix2 = floor (pos.x2);

      if (b.x2 < t.x2)
	{
	  if (ix2 == pos.x2)
	    x2_next = ix2 - 1;
	  else
	    x2_next = ix2;

	  if (x2_next > pos_next.x2)
	    {
	      interpolate_position (&pos_next, &t, &b,
				    x2_next, t.x2, b.x2);
	      pos_next.x2 = x2_next;
	      done = FALSE;
	    }
	}
      else if (x22 > x21)
	{
	  x2_next = ix2 + 1;

	  if (x2_next < pos_next.x2)
	    {
	      interpolate_position (&pos_next, &t, &b,
				    x2_next, t.x2, b.x2);
	      pos_next.x2 = x2_next;
	      done = FALSE;
	    }
	}

      draw_simple_trap (bitmap, &pos, &pos_next);
      pos = pos_next;
    }
}

typedef struct
{
  double x, y;
} Point;

static void
to_device (const PangoMatrix *matrix,
	   double             x,
	   double             y,
	   Point             *result)
{
  result->x = (x * matrix->xx + y * matrix->xy) / PANGO_SCALE + matrix->x0;
  result->y = (x * matrix->yx + y * matrix->yy) / PANGO_SCALE + matrix->y0;
}

static int
compare_points (const void *a,
		const void *b)
{
  const Point *pa = a;
  const Point *pb = b;

  if (pa->y < pb->y)
    return -1;
  else if (pa->y > pb->y)
    return 1;
  else if (pa->x < pb->x)
    return -1;
  else if (pa->x > pb->x)
    return 1;
  else
    return 0;
}

/**
 * _pango_ft2_draw_rect:
 * @bitmap: a #FT_Bitmap
 * @matrix: a #PangoMatrix giving the user to device transformation,
 *          or %NULL for the identity.
 * @x: X coordinate of rectangle, in Pango units in user coordinate system
 * @y: Y coordinate of rectangle, in Pango units in user coordinate system
 * @width: width of rectangle, in Pango units in user coordinate system
 * @height: height of rectangle, in Pango units in user coordinate system
 *
 * Render an axis aligned rectangle in user coordinates onto
 * a bitmap after transformation by the given matrix. Rendering
 * is done anti-aliased.
 **/
void
_pango_ft2_draw_rect (FT_Bitmap          *bitmap,
		      const PangoMatrix  *matrix,
		      int                 x,
		      int                 y,
		      int                 width,
		      int                 height)
{
  static const PangoMatrix identity = PANGO_MATRIX_INIT;
  Point points[4];

  if (!matrix)
    matrix = &identity;

  /* Convert the points to device coordinates, and sort
   * in ascending Y order. (Ordering by X for ties)
   */
  to_device (matrix, x, y, &points[0]);
  to_device (matrix, x + width, y, &points[1]);
  to_device (matrix, x, y + height, &points[2]);
  to_device (matrix, x + width, y + height, &points[3]);

  qsort (points, 4, sizeof (Point), compare_points);

  /* There are essentially three cases. (There is a fourth
   * case where trapezoid B is degenerate and we just have
   * two triangles, but we don't need to handle it separately.)
   *
   *     1            2             3
   *
   *     ______       /\           /\
   *    /     /      /A \         /A \
   *   /  B  /      /____\       /____\
   *  /_____/      /  B  /       \  B  \
   *              /_____/         \_____\
   *              \ C  /           \ C  /
   *               \  /             \  /
   *                \/               \/
   */
  if (points[0].y == points[1].y)
    {
     /* Case 1 (pure shear) */
      draw_trap (bitmap,                                 /* B */
		 points[0].y, points[0].x, points[1].x,
		 points[2].y, points[2].x, points[3].x);
    }
  else if (points[1].x < points[2].x)
    {
      /* Case 2 */
      double tmp_width = ((points[2].x - points[0].x) * (points[1].y - points[0].y)) / (points[2].y - points[0].y);
      double base_width = tmp_width + points[0].x - points[1].x;

      draw_trap (bitmap,                                              /* A */
		 points[0].y, points[0].x, points[0].x,
		 points[1].y, points[1].x, points[1].x + base_width);
      draw_trap (bitmap,
		 points[1].y, points[1].x, points[1].x + base_width,  /* B */
		 points[2].y, points[2].x - base_width, points[2].x);
      draw_trap (bitmap,
		 points[2].y, points[2].x - base_width, points[2].x,  /* C */
		 points[3].y, points[3].x, points[3].x);
    }
  else
    {
      /* case 3 */
      double tmp_width = ((points[0].x - points[2].x) * (points[1].y - points[0].y)) / (points[2].y - points[0].y);
      double base_width = tmp_width + points[1].x - points[0].x;

      draw_trap (bitmap,                                             /* A */
		 points[0].y, points[0].x, points[0].x,
		 points[1].y,  points[1].x - base_width, points[1].x);
      draw_trap (bitmap,
		 points[1].y, points[1].x - base_width, points[1].x, /* B */
		 points[2].y, points[2].x, points[2].x + base_width);
      draw_trap (bitmap,
		 points[2].y, points[2].x, points[2].x + base_width, /* C */
		 points[3].y, points[3].x, points[3].x);
    }
}

/* We are drawing an error underline that looks like one of:
 *
 *  /\      /\      /\        /\      /\               -
 * /  \    /  \    /  \      /  \    /  \              |
 * \   \  /\   \  /   /      \   \  /\   \             |
 *  \   \/B \   \/ C /        \   \/B \   \            | height = HEIGHT_SQUARES * square
 *   \ A \  /\ A \  /          \ A \  /\ A \           | 
 *    \   \/  \   \/            \   \/  \   \          |
 *     \  /    \  /              \  /    \  /          |
 *      \/      \/                \/      \/           -
 *
 * |----|
 *   unit_width = (HEIGHT_SQUARES - 1) * square
 *
 * To do this conveniently, we work in a coordinate system where A,B,C
 * are axis aligned rectangles. (If fonts were square, the diagrams
 * would be clearer)
 *
 *             (0,0)    
 *              /\      /\        
 *             /  \    /  \      
 *            /\  /\  /\  /      
 *           /  \/  \/  \/       
 *          /    \  /\  /        
 *      Y axis    \/  \/         
 *                 \  /\          
 *                  \/  \         
 *                       \ X axis
 *
 * Note that the long side in this coordinate system is HEIGHT_SQUARES + 1
 * units long
 *
 * The diagrams above are shown with HEIGHT_SQUARES an integer, but
 * that is actually incidental; the value 2.5 below seems better than
 * either HEIGHT_SQUARES=3 (a little long and skinny) or
 * HEIGHT_SQUARES=2 (a bit short and stubby)
 */
 
#define HEIGHT_SQUARES 2.5

static void
get_total_matrix (PangoMatrix        *total,
		  const PangoMatrix  *global,
		  int                 x,
		  int                 y,
		  int                 square)
{
  PangoMatrix local;
  gdouble scale = 0.5 * square;

  /* The local matrix translates from the axis aligned coordinate system
   * to the original user space coordinate system.
   */
  local.xx = scale;
  local.xy = - scale;
  local.yx = scale;
  local.yy = scale;
  local.x0 = 0;
  local.y0 = 0;

  *total = *global;
  pango_matrix_concat (total, &local);

  total->x0 = (global->xx * x + global->xy * y) / PANGO_SCALE + global->x0;
  total->y0 = (global->yx * x + global->yy * y) / PANGO_SCALE + global->y0;
}

/**
 * _pango_ft2_draw_error_underline:
 * @bitmap: a #FT_Bitmap
 * @matrix: a #PangoMatrix giving the user to device transformation,
 *          or %NULL for the identity.
 * @x: X coordinate of underline, in Pango units in user coordinate system
 * @y: Y coordinate of underline, in Pango units in user coordinate system
 * @width: width of underline, in Pango units in user coordinate system
 * @height: height of underline, in Pango units in user coordinate system
 *
 * Draw a squiggly line that approximately covers the given rectangle
 * in the style of an underline used to indicate a spelling error.
 * (The width of the underline is rounded to an integer number
 * of up/down segments and the resulting rectangle is centered
 * in the original rectangle)
 **/
void
_pango_ft2_draw_error_underline (FT_Bitmap          *bitmap,
				 const PangoMatrix  *matrix,
				 int                 x,
				 int                 y,
				 int                 width,
				 int                 height)
{

  int square = height / HEIGHT_SQUARES;
  int unit_width = (HEIGHT_SQUARES - 1) * square;
  int width_units = (width + unit_width / 2) / unit_width;
  static const PangoMatrix identity = PANGO_MATRIX_INIT;

  x += (width - width_units * unit_width) / 2;
  width = width_units * unit_width;

  if (!matrix)
    matrix = &identity;

  while (TRUE)
    {
      PangoMatrix total;
      get_total_matrix (&total, matrix, x, y, square);

      _pango_ft2_draw_rect (bitmap, &total,              /* A */
			    0,                      0,
			    HEIGHT_SQUARES * 2 - 1, 1);

      if (width_units > 2)
	{
	  _pango_ft2_draw_rect (bitmap, &total,
				HEIGHT_SQUARES * 2 - 2, - (HEIGHT_SQUARES * 2 - 3),
				1,                      HEIGHT_SQUARES * 2 - 3); /* B */
	  width_units -= 2;
	  x += unit_width * 2;
	}
      else if (width_units == 2)
	{
	  _pango_ft2_draw_rect (bitmap, &total,
				HEIGHT_SQUARES * 2 - 2, - (HEIGHT_SQUARES * 2 - 2),
				1,                      HEIGHT_SQUARES * 2 - 2); /* C */
	  break;
	}
      else
	break;
    }
}
