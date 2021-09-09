/* viewer-pangoft2.c: PangoFT2 viewer backend.
 *
 * Copyright (C) 1999,2004,2005 Red Hat, Inc.
 * Copyright (C) 2001 Sun Microsystems
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

#include "config.h"
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "viewer-render.h"
#include "viewer.h"

#include <pango/pangoft2.h>

static void
substitute_func (FcPattern *pattern,
		 gpointer   data G_GNUC_UNUSED)
{
  if (opt_antialias != ANTIALIAS_DEFAULT)
    {
      FcPatternDel (pattern, FC_ANTIALIAS);
      FcPatternAddBool (pattern, FC_ANTIALIAS, opt_antialias != ANTIALIAS_NONE);
    }
  if (opt_hinting != HINT_DEFAULT)
    {
      FcPatternDel (pattern, FC_HINTING);
      FcPatternAddBool (pattern, FC_HINTING, opt_hinting != HINT_NONE);

      FcPatternDel (pattern, FC_AUTOHINT);
      FcPatternAddBool (pattern, FC_AUTOHINT, opt_hinting == HINT_AUTO);
    }
}

static gpointer
pangoft2_view_create (const PangoViewer *klass G_GNUC_UNUSED)
{
  PangoFontMap *fontmap;
  fontmap = pango_ft2_font_map_new ();

  pango_ft2_font_map_set_resolution (PANGO_FT2_FONT_MAP (fontmap), opt_dpi, opt_dpi);
  pango_fc_font_map_set_default_substitute (PANGO_FC_FONT_MAP (fontmap), substitute_func, NULL, NULL);

  return fontmap;
}

static void
pangoft2_view_destroy (gpointer instance)
{
  g_object_unref (instance);
}

static PangoContext *
pangoft2_view_get_context (gpointer instance)
{
  return pango_font_map_create_context (PANGO_FONT_MAP (instance));
}

static gpointer
pangoft2_view_create_surface (gpointer instance G_GNUC_UNUSED,
			      int      width,
			      int      height)
{
  FT_Bitmap *bitmap;

  bitmap = g_slice_new (FT_Bitmap);
  bitmap->width = width;
  bitmap->pitch = (bitmap->width + 3) & ~3;
  bitmap->rows = height;
  bitmap->buffer = g_malloc (bitmap->pitch * bitmap->rows);
  bitmap->num_grays = 256;
  bitmap->pixel_mode = ft_pixel_mode_grays;
  memset (bitmap->buffer, 0x00, bitmap->pitch * bitmap->rows);

  return bitmap;
}

static void
pangoft2_view_destroy_surface (gpointer instance G_GNUC_UNUSED,
			       gpointer surface)
{
  FT_Bitmap *bitmap = (FT_Bitmap *) surface;

  g_free (bitmap->buffer);
  g_slice_free (FT_Bitmap, bitmap);
}

static void
render_callback (PangoLayout *layout,
		 int          x,
		 int          y,
		 gpointer     context,
		 gpointer     state G_GNUC_UNUSED)
{
  pango_ft2_render_layout ((FT_Bitmap *)context,
			   layout,
			   x, y);
}

static void
pangoft2_view_render (gpointer      instance G_GNUC_UNUSED,
		      gpointer      surface,
		      PangoContext *context,
		      int          *width,
		      int          *height,
		      gpointer      state)
{
  int pix_idx;
  FT_Bitmap *bitmap = (FT_Bitmap *) surface;

  do_output (context, render_callback, NULL, surface, state, width, height);

  for (pix_idx=0; pix_idx<bitmap->pitch * bitmap->rows; pix_idx++)
    bitmap->buffer[pix_idx] = 255 - bitmap->buffer[pix_idx];
}

static void
pangoft2_view_write (gpointer instance G_GNUC_UNUSED,
		     gpointer surface,
		     FILE    *stream,
		     int      width,
		     int      height)
{
  int row;
  FT_Bitmap *bitmap = (FT_Bitmap *) surface;

  /* Write it as pgm to output */
  fprintf(stream,
	  "P5\n"
	  "%d %d\n"
	  "255\n", width, height);
  for (row = 0; row < height; row++)
    fwrite (bitmap->buffer + row * bitmap->pitch, 1, width, stream);
}

const PangoViewer pangoft2_viewer = {
  "PangoFT2",
  "ft2",
  ".pgm",
  pangoft2_view_create,
  pangoft2_view_destroy,
  pangoft2_view_get_context,
  pangoft2_view_create_surface,
  pangoft2_view_destroy_surface,
  pangoft2_view_render,
  pangoft2_view_write,
  NULL,
  NULL,
  NULL
};
