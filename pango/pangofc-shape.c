/* Pango
 * pangofc-shape.c: Basic shaper for FreeType-based backends
 *
 * Copyright (C) 2000, 2007, 2009 Red Hat Software
 * Authors:
 *   Owen Taylor <otaylor@redhat.com>
 *   Behdad Esfahbod <behdad@behdad.org>
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
#include <string.h>
#include <math.h>

#include "pangohb-private.h"
#include "pango-impl-utils.h"

#include <hb-glib.h>

/* cache a single hb_buffer_t */
static hb_buffer_t *cached_buffer = NULL; /* MT-safe */
G_LOCK_DEFINE_STATIC (cached_buffer);

static hb_buffer_t *
acquire_buffer (gboolean *free_buffer)
{
  hb_buffer_t *buffer;

  if (G_LIKELY (G_TRYLOCK (cached_buffer)))
    {
      if (G_UNLIKELY (!cached_buffer))
	cached_buffer = hb_buffer_create ();

      buffer = cached_buffer;
      *free_buffer = FALSE;
    }
  else
    {
      buffer = hb_buffer_create ();
      *free_buffer = TRUE;
    }

  return buffer;
}

static void
release_buffer (hb_buffer_t *buffer, gboolean free_buffer)
{
  if (G_LIKELY (!free_buffer))
    {
      hb_buffer_reset (buffer);
      G_UNLOCK (cached_buffer);
    }
  else
    hb_buffer_destroy (buffer);
}

static void
apply_extra_attributes (GSList       *attrs,
                        hb_feature_t *features,
                        guint         length,
                        guint        *num_features)
{
  GSList *l;

  for (l = attrs; l && *num_features < length; l = l->next)
    {
      PangoAttribute *attr = l->data;
      if (attr->klass->type == PANGO_ATTR_FONT_FEATURES)
        {
          PangoAttrFontFeatures *fattr = (PangoAttrFontFeatures *) attr;
          const gchar *feat;
          const gchar *end;
          int len;

          feat = fattr->features;

          while (feat != NULL && *num_features < length)
            {
              end = strchr (feat, ',');
              if (end)
                len = end - feat;
              else
                len = -1;
              if (hb_feature_from_string (feat, len, &features[*num_features]))
                {
                  features[*num_features].start = attr->start_index;
                  features[*num_features].end = attr->end_index;
                  (*num_features)++;
                }

              if (end == NULL)
                break;

              feat = end + 1;
            }
        }
    }
}

void
pango_hb_shape (PangoFont           *font,
                const char          *item_text,
                unsigned int         item_length,
                const PangoAnalysis *analysis,
                PangoGlyphString    *glyphs,
                const char          *paragraph_text,
                unsigned int         paragraph_length)
{
  hb_font_t *hb_font;
  hb_buffer_t *hb_buffer;
  hb_direction_t hb_direction;
  gboolean free_buffer;
  hb_glyph_info_t *hb_glyph;
  hb_glyph_position_t *hb_position;
  int last_cluster;
  guint i, num_glyphs;
  unsigned int item_offset = item_text - paragraph_text;
  hb_feature_t features[32];
  unsigned int num_features = 0;
  PangoGlyphInfo *infos;

  g_return_if_fail (font != NULL);
  g_return_if_fail (analysis != NULL);

  hb_font = pango_font_get_hb_font (font);

  hb_buffer = acquire_buffer (&free_buffer);

  hb_direction = PANGO_GRAVITY_IS_VERTICAL (analysis->gravity) ? HB_DIRECTION_TTB : HB_DIRECTION_LTR;
  if (analysis->level % 2)
    hb_direction = HB_DIRECTION_REVERSE (hb_direction);
  if (PANGO_GRAVITY_IS_IMPROPER (analysis->gravity))
    hb_direction = HB_DIRECTION_REVERSE (hb_direction);

  /* setup buffer */

  hb_buffer_set_direction (hb_buffer, hb_direction);
  hb_buffer_set_script (hb_buffer, hb_glib_script_to_script (analysis->script));
  hb_buffer_set_language (hb_buffer, hb_language_from_string (pango_language_to_string (analysis->language), -1));
#if HB_VERSION_ATLEAST(1,0,3)
  hb_buffer_set_cluster_level (hb_buffer, HB_BUFFER_CLUSTER_LEVEL_MONOTONE_CHARACTERS);
#endif
  hb_buffer_set_flags (hb_buffer, HB_BUFFER_FLAG_BOT | HB_BUFFER_FLAG_EOT);

  hb_buffer_add_utf8 (hb_buffer, paragraph_text, paragraph_length, item_offset, item_length);

  pango_font_get_features (font, features, G_N_ELEMENTS (features), &num_features);
  apply_extra_attributes (analysis->extra_attrs, features, G_N_ELEMENTS (features), &num_features);

  hb_shape (hb_font, hb_buffer, features, num_features);

  if (PANGO_GRAVITY_IS_IMPROPER (analysis->gravity))
    hb_buffer_reverse (hb_buffer);

  /* buffer output */
  num_glyphs = hb_buffer_get_length (hb_buffer);
  hb_glyph = hb_buffer_get_glyph_infos (hb_buffer, NULL);
  pango_glyph_string_set_size (glyphs, num_glyphs);
  infos = glyphs->glyphs;
  last_cluster = -1;
  for (i = 0; i < num_glyphs; i++)
    {
      infos[i].glyph = hb_glyph->codepoint;
      glyphs->log_clusters[i] = hb_glyph->cluster - item_offset;
      infos[i].attr.is_cluster_start = glyphs->log_clusters[i] != last_cluster;
      hb_glyph++;
      last_cluster = glyphs->log_clusters[i];
    }

  hb_position = hb_buffer_get_glyph_positions (hb_buffer, NULL);
  if (PANGO_GRAVITY_IS_VERTICAL (analysis->gravity))
    for (i = 0; i < num_glyphs; i++)
      {
        /* 90 degrees rotation counter-clockwise. */
	infos[i].geometry.width    =  hb_position->y_advance;
	infos[i].geometry.x_offset =  hb_position->y_offset;
	infos[i].geometry.y_offset = -hb_position->x_offset;
	hb_position++;
      }
  else /* horizontal */
    for (i = 0; i < num_glyphs; i++)
      {
	infos[i].geometry.width    = hb_position->x_advance;
	infos[i].geometry.x_offset = hb_position->x_offset;
	infos[i].geometry.y_offset = hb_position->y_offset;
	hb_position++;
      }

  release_buffer (hb_buffer, free_buffer);
}
