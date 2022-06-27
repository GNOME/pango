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

/* Get the font data as a blob. Either, the hb_face_t was
 * created from a blob to begin with, or we receate one using
 * hb_face_builder_create(). Unfortunately we can't rely on
 * hb_face_get_table_tags(), so we just have to list all possible
 * OpenType tables here and try them all.
 *
 * This is not ideal from a memory perspective, but cairo does
 * not have a scaled font implementation that takes individual
 * tables.
 */
static hb_blob_t *
face_get_blob (hb_face_t *face)
{
  hb_blob_t *blob;
  hb_face_t *builder;
  static const hb_tag_t ot_tables[] = {
    HB_TAG ('a','v','a','r'),
    HB_TAG ('B','A','S','E'),
    HB_TAG ('C','B','D','T'),
    HB_TAG ('C','B','L','C'),
    HB_TAG ('C','F','F',' '),
    HB_TAG ('C','F','F','2'),
    HB_TAG ('c','m','a','p'),
    HB_TAG ('C','O','L','R'),
    HB_TAG ('C','P','A','L'),
    HB_TAG ('c','v','a','r'),
    HB_TAG ('c','v','t',' '),
    HB_TAG ('D','S','I','G'),
    HB_TAG ('E','B','D','T'),
    HB_TAG ('E','B','L','C'),
    HB_TAG ('E','B','S','C'),
    HB_TAG ('f','p','g','m'),
    HB_TAG ('f','v','a','r'),
    HB_TAG ('g','a','s','p'),
    HB_TAG ('G','D','E','F'),
    HB_TAG ('g','l','y','f'),
    HB_TAG ('G','P','O','S'),
    HB_TAG ('G','S','U','B'),
    HB_TAG ('g','v','a','r'),
    HB_TAG ('h','d','m','x'),
    HB_TAG ('h','e','a','d'),
    HB_TAG ('h','h','e','a'),
    HB_TAG ('h','m','t','x'),
    HB_TAG ('H','V','A','R'),
    HB_TAG ('J','S','T','F'),
    HB_TAG ('k','e','r','n'),
    HB_TAG ('l','o','c','a'),
    HB_TAG ('L','T','S','H'),
    HB_TAG ('M','A','T','H'),
    HB_TAG ('m','a','x','p'),
    HB_TAG ('M','E','R','G'),
    HB_TAG ('m','e','t','a'),
    HB_TAG ('M','V','A','R'),
    HB_TAG ('n','a','m','e'),
    HB_TAG ('O','S','/','2'),
    HB_TAG ('P','C','L','T'),
    HB_TAG ('p','o','s','t'),
    HB_TAG ('p','r','e','p'),
    HB_TAG ('s','b','i','x'),
    HB_TAG ('S','T','A','T'),
    HB_TAG ('S','V','G',' '),
    HB_TAG ('V','D','M','X'),
    HB_TAG ('v','h','e','a'),
    HB_TAG ('v','m','t','x'),
    HB_TAG ('V','O','R','G'),
    HB_TAG ('V','V','A','R'),
  };

  blob = hb_face_reference_blob (face);

  if (blob != hb_blob_get_empty ())
    return blob;

  builder = hb_face_builder_create ();

  for (unsigned int i = 0; i < G_N_ELEMENTS (ot_tables); i++)
    {
      hb_blob_t *table;

      table = hb_face_reference_table (face, ot_tables[i]);

      if (table != hb_blob_get_empty ())
        {
          hb_face_builder_add_table (builder, ot_tables[i], table);
          hb_blob_destroy (table);
        }
    }

  blob = hb_face_reference_blob (builder);

  hb_face_destroy (builder);

  return blob;
}

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
  hb_face_t *hbface;
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
  hbface = hb_font_get_face (hb_font);
  blob = face_get_blob (hbface);
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
