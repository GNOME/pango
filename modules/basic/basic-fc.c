/* Pango
 * basic-fc.c: Basic shaper for FreeType-based backends
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

#define PANGO_ENABLE_BACKEND 1 /* XXX */
#include <glib/gprintf.h>

#include "pango-engine.h"
#include "pango-utils.h"
#include "pangofc-font.h"
#include <hb-ft.h>
#include <hb-glib.h>

#define PANGO_UNITS_26_6(d)	((d) << 4)


/* No extra fields needed */
typedef PangoEngineShape      BasicEngineFc;
typedef PangoEngineShapeClass BasicEngineFcClass;

#define SCRIPT_ENGINE_NAME "BasicScriptEngineFc"
#define RENDER_TYPE PANGO_RENDER_TYPE_FC

static PangoEngineScriptInfo basic_scripts[] = {
  { PANGO_SCRIPT_COMMON,   "" }
};

static PangoEngineInfo script_engines[] = {
  {
    SCRIPT_ENGINE_NAME,
    PANGO_ENGINE_TYPE_SHAPE,
    RENDER_TYPE,
    basic_scripts, G_N_ELEMENTS(basic_scripts)
  }
};


/* cache a single hb_buffer_t */
static hb_buffer_t *cached_buffer = NULL;
G_LOCK_DEFINE_STATIC (cached_buffer);

static hb_buffer_t *
create_buffer (void)
{
  hb_buffer_t *buffer;

  buffer = hb_buffer_create (32);
  hb_buffer_set_unicode_funcs (buffer, hb_glib_get_unicode_funcs ());

  return buffer;
}

static hb_buffer_t *
acquire_buffer (gboolean *free_buffer)
{
  hb_buffer_t *buffer;

  if (G_LIKELY (G_TRYLOCK (cached_buffer)))
    {
      if (G_UNLIKELY (!cached_buffer))
	cached_buffer = create_buffer ();

      buffer = cached_buffer;
      *free_buffer = FALSE;
    }
  else
    {
      buffer = create_buffer ();
      *free_buffer = TRUE;
    }

  return buffer;
}

static void
release_buffer (hb_buffer_t *buffer, gboolean free_buffer)
{
  if (G_LIKELY (!free_buffer) && hb_buffer_get_reference_count (buffer) == 1)
    {
      hb_buffer_clear (buffer);
      G_UNLOCK (cached_buffer);
    }
  else
    hb_buffer_destroy (buffer);
}

typedef struct _PangoFcHbContext {
  FT_Face ft_face;
  PangoFcFont *fc_font;
} PangoFcHbContext;

static hb_codepoint_t
pango_fc_hb_font_get_glyph (hb_font_t *font, hb_face_t *face, const void *user_data,
			    hb_codepoint_t unicode, hb_codepoint_t variation_selector)
{
  PangoFcHbContext *context = (PangoFcHbContext *) user_data;
  PangoFcFont *fc_font = context->fc_font;
  PangoGlyph glyph;

  glyph = pango_fc_font_get_glyph (fc_font, unicode);
  if (G_UNLIKELY (!glyph))
    glyph = PANGO_GET_UNKNOWN_GLYPH (unicode);

  return glyph;
}

static hb_bool_t
pango_fc_hb_font_get_contour_point (hb_font_t *font, hb_face_t *face, const void *user_data,
				    unsigned int point_index,
				    hb_codepoint_t glyph, hb_position_t *x, hb_position_t *y)
{
  return FALSE;
#if 0
  FT_Face ft_face = (FT_Face) user_data;
  int load_flags = FT_LOAD_DEFAULT;

  /* TODO: load_flags, embolden, etc */

  if (HB_UNLIKELY (FT_Load_Glyph (ft_face, glyph, load_flags)))
      return FALSE;

  if (HB_UNLIKELY (ft_face->glyph->format != FT_GLYPH_FORMAT_OUTLINE))
      return FALSE;

  if (HB_UNLIKELY (point_index >= (unsigned int) ft_face->glyph->outline.n_points))
      return FALSE;

  *x = ft_face->glyph->outline.points[point_index].x;
  *y = ft_face->glyph->outline.points[point_index].y;

  return TRUE;
#endif
}

static void
pango_fc_hb_font_get_glyph_metrics (hb_font_t *font, hb_face_t *face, const void *user_data,
				    hb_codepoint_t glyph, hb_glyph_metrics_t *metrics)
{
  PangoFcHbContext *context = (PangoFcHbContext *) user_data;
  PangoFcFont *fc_font = context->fc_font;
  PangoRectangle ink, logical;

  pango_font_get_glyph_extents ((PangoFont *) fc_font, glyph, &ink, &logical);

  metrics->x_advance = logical.width;
  metrics->y_advance = 0;
  metrics->x_offset  =  ink.x;
  metrics->y_offset  = -ink.y;
  metrics->width     = ink.width;
  metrics->height    = ink.height;
}

static hb_position_t
pango_fc_hb_font_get_kerning (hb_font_t *font, hb_face_t *face, const void *user_data,
			      hb_codepoint_t first_glyph, hb_codepoint_t second_glyph)
{
  PangoFcHbContext *context = (PangoFcHbContext *) user_data;
  FT_Face ft_face = context->ft_face;
  FT_Vector kerning;

  /* TODO: Kern type? */
  if (FT_Get_Kerning (ft_face, first_glyph, second_glyph, FT_KERNING_DEFAULT, &kerning))
      return 0;

  return PANGO_UNITS_26_6 (kerning.x);
}

static hb_font_funcs_t *
pango_fc_get_hb_font_funcs (void)
{
  static hb_font_funcs_t *funcs;

  if (G_UNLIKELY (!funcs)) {
    funcs = hb_font_funcs_create ();
    hb_font_funcs_set_glyph_func (funcs, pango_fc_hb_font_get_glyph);
    hb_font_funcs_set_contour_point_func (funcs, pango_fc_hb_font_get_contour_point);
    hb_font_funcs_set_glyph_metrics_func (funcs, pango_fc_hb_font_get_glyph_metrics);
    hb_font_funcs_set_kerning_func (funcs, pango_fc_hb_font_get_kerning);
  }

  return funcs;
}




static void
basic_engine_shape (PangoEngineShape *engine G_GNUC_UNUSED,
		    PangoFont        *font,
		    const char       *text,
		    gint              length,
		    const PangoAnalysis *analysis,
		    PangoGlyphString *glyphs)
{
  PangoFcHbContext context;
  PangoFcFont *fc_font;
  FT_Face ft_face;
  hb_face_t *hb_face;
  hb_font_t *hb_font;
  hb_buffer_t *hb_buffer;
  gboolean free_buffer;
  gboolean is_hinted;
  hb_glyph_info_t *hb_glyph;
  hb_glyph_position_t *hb_position;
  int last_cluster;
  guint i, num_glyphs;

  g_return_if_fail (font != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (length >= 0);
  g_return_if_fail (analysis != NULL);

  fc_font = PANGO_FC_FONT (font);
  ft_face = pango_fc_font_lock_face (fc_font);
  if (!ft_face)
    return;
  hb_face = hb_ft_face_create_cached (ft_face);

  /* TODO: Cache hb_font? */
  context.ft_face = ft_face;
  context.fc_font = fc_font;
  hb_font = hb_font_create ();
  hb_font_set_funcs (hb_font,
		     pango_fc_get_hb_font_funcs (),
		     NULL,
		     &context);
  hb_font_set_scale (hb_font,
		     /* XXX CTM */
		     ((guint64) ft_face->size->metrics.x_scale * ft_face->units_per_EM) >> 12,
		     ((guint64) ft_face->size->metrics.y_scale * ft_face->units_per_EM) >> 12);
  is_hinted = fc_font->is_hinted;
  hb_font_set_ppem (hb_font,
		    is_hinted ? ft_face->size->metrics.x_ppem : 0,
		    is_hinted ? ft_face->size->metrics.y_ppem : 0);

  hb_buffer = acquire_buffer (&free_buffer);
  /* setup buffer */
  hb_buffer_set_direction (hb_buffer, analysis->level % 2 != 0 ? HB_DIRECTION_RTL : HB_DIRECTION_LTR);
  hb_buffer_set_script (hb_buffer, analysis->script);
  hb_buffer_set_language (hb_buffer, hb_language_from_string (pango_language_to_string (analysis->language)));
  hb_buffer_add_utf8 (hb_buffer, text, length, 0, length);

  hb_shape (hb_font, hb_face, hb_buffer, NULL, 0);

  /* buffer output */
  num_glyphs = hb_buffer_get_length (hb_buffer);
  hb_glyph = hb_buffer_get_glyph_infos (hb_buffer);
  hb_position = hb_buffer_get_glyph_positions (hb_buffer);
  pango_glyph_string_set_size (glyphs, num_glyphs);
  last_cluster = -1;
  for (i = 0; i < num_glyphs; i++)
    {
      glyphs->glyphs[i].glyph = hb_glyph->codepoint;
      glyphs->log_clusters[i] = hb_glyph->cluster;
      glyphs->glyphs[i].attr.is_cluster_start = glyphs->log_clusters[i] != last_cluster;
      last_cluster = glyphs->log_clusters[i];

      glyphs->glyphs[i].geometry.width = hb_position->x_advance;
      glyphs->glyphs[i].geometry.x_offset =  hb_position->x_offset;
      glyphs->glyphs[i].geometry.y_offset = -hb_position->y_offset;

      hb_glyph++;
      hb_position++;
    }

  release_buffer (hb_buffer, free_buffer);
  hb_font_destroy (hb_font);
  hb_face_destroy (hb_face);
  pango_fc_font_unlock_face (fc_font);
}

static void
basic_engine_fc_class_init (PangoEngineShapeClass *class)
{
  class->script_shape = basic_engine_shape;
}

PANGO_ENGINE_SHAPE_DEFINE_TYPE (BasicEngineFc, basic_engine_fc,
				basic_engine_fc_class_init, NULL)

void
PANGO_MODULE_ENTRY(init) (GTypeModule *module)
{
  basic_engine_fc_register_type (module);
}

void
PANGO_MODULE_ENTRY(exit) (void)
{
}

void
PANGO_MODULE_ENTRY(list) (PangoEngineInfo **engines,
			  int              *n_engines)
{
  *engines = script_engines;
  *n_engines = G_N_ELEMENTS (script_engines);
}

PangoEngine *
PANGO_MODULE_ENTRY(create) (const char *id)
{
  if (!strcmp (id, SCRIPT_ENGINE_NAME))
    return g_object_new (basic_engine_fc_type, NULL);
  else
    return NULL;
}
