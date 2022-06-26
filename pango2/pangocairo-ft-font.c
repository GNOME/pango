/*
 * pangocairo-ft-font.c: Freetype font handling
 *
 * Copyright (C) 2022 Red Hat, Inc.
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
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include "pango-features.h"
#include "pangocairo-private.h"
#include "pango-hbfont-private.h"
#include "pango-hbface-private.h"

#ifdef CAIRO_HAS_FT_FONT

#include <hb-ot.h>
#include <cairo-ft.h>
#include <freetype/ftmm.h>

cairo_font_face_t *
create_cairo_ft_font_face (Pango2Font *font)
{
  static FT_Library ft_library;

  Pango2HbFace *face = PANGO2_HB_FACE (font->face);
  hb_blob_t *blob;
  const char *blob_data;
  unsigned int blob_length;
  FT_Face ft_face;
  hb_font_t *hb_font;
  unsigned int num_coords;
  const int *coords;
  cairo_font_face_t *cairo_face;
  static const cairo_user_data_key_t key;
  static const cairo_user_data_key_t key2;
  FT_Error error;

  if (g_once_init_enter (&ft_library))
    {
      FT_Library library;
      FT_Init_FreeType (&library);
      g_once_init_leave (&ft_library, library);
    }

  hb_font = pango2_font_get_hb_font (font);
  blob = hb_face_reference_blob (hb_font_get_face (hb_font));
  blob_data = hb_blob_get_data (blob, &blob_length);

  if ((error = FT_New_Memory_Face (ft_library,
                                   (const FT_Byte *) blob_data,
                                   blob_length,
                                   hb_face_get_index (face->face),
                                   &ft_face)) != 0)
    {
      hb_blob_destroy (blob);
      g_warning ("FT_New_Memory_Face failed: %d %s", error, FT_Error_String (error));
      return NULL;
    }

  coords = hb_font_get_var_coords_normalized (hb_font, &num_coords);
  if (num_coords > 0)
    {
      FT_Fixed *ft_coords = (FT_Fixed *) g_alloca (num_coords * sizeof (FT_Fixed));

      for (unsigned int i = 0; i < num_coords; i++)
        ft_coords[i] = coords[i] << 2;

      FT_Set_Var_Blend_Coordinates (ft_face, num_coords, ft_coords);
    }

  cairo_face = cairo_ft_font_face_create_for_ft_face (ft_face, FT_LOAD_NO_HINTING | FT_LOAD_COLOR);

  if (face->embolden)
    cairo_ft_font_face_set_synthesize (cairo_face, CAIRO_FT_SYNTHESIZE_BOLD);

  cairo_font_face_set_user_data (cairo_face, &key,
                                 ft_face, (cairo_destroy_func_t) FT_Done_Face);
  cairo_font_face_set_user_data (cairo_face, &key2,
                                 blob, (cairo_destroy_func_t) hb_blob_destroy);

  return cairo_face;
}

#endif
