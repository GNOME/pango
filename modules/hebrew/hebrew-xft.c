/* Pango
 * hebrew-xft.h:
 *
 * Copyright (C) 2000 Red Hat Software
 * Author: Owen Taylor <otaylor@redhat.com>
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

#include <string.h>

#include "pangoxft.h"
#include "pango-engine.h"
#include "pango-utils.h"
#include "hebrew-shaper.h"

#define MAX_CLUSTER_CHRS	20

static PangoEngineRange hebrew_ranges[] = {
  /* Language characters */
  { 0x0591, 0x05f4, "*" }, /* Hebrew */
};

static PangoEngineInfo script_engines[] = {
  {
    "HebrewScriptEngineXft",
    PANGO_ENGINE_TYPE_SHAPE,
    PANGO_RENDER_TYPE_XFT,
    hebrew_ranges, G_N_ELEMENTS(hebrew_ranges)
  }
};

static guint
get_glyph_num (FT_Face face, PangoFont *font, gunichar wc)
{
  int index = FT_Get_Char_Index (face, wc);

  if (index && index <= face->num_glyphs)
    return index;
  else
    return 0;
}

static void
get_cluster_glyphs(FT_Face        face,
		   PangoFont      *font,
		   gunichar       cluster[],
		   gint           cluster_size,
		   /* output */
		   gint           glyph_num[],
		   PangoGlyph     glyph[],
		   gint           widths[],
		   PangoRectangle ink_rects[])
{
  int i;
  for (i=0; i<cluster_size; i++)
    {
      PangoRectangle logical_rect;
      glyph_num[i] = get_glyph_num(face, font, cluster[i]);
      glyph[i] = glyph_num[i];

      pango_font_get_glyph_extents (font,
				    glyph[i], &ink_rects[i], &logical_rect);
      
      /* Assign the base char width to the last character in the cluster */
      if (i==0)
	{
	  widths[i] = 0;
	  widths[cluster_size-1] = logical_rect.width;
	}
      else if (i < cluster_size-1)
	widths[i] = 0;
    }
}

static void
add_glyph (PangoGlyphString *glyphs, 
	   gint              cluster_start, 
	   PangoGlyph        glyph,
	   gboolean          is_combining,
	   gint              width,
	   gint              x_offset,
	   gint              y_offset
	   )
{
  gint index = glyphs->num_glyphs;

  pango_glyph_string_set_size (glyphs, index + 1);
  
  glyphs->glyphs[index].glyph = glyph;
  glyphs->glyphs[index].attr.is_cluster_start = is_combining ? 0 : 1;
  
  glyphs->log_clusters[index] = cluster_start;

  glyphs->glyphs[index].geometry.x_offset = x_offset;
  glyphs->glyphs[index].geometry.y_offset = y_offset;
  glyphs->glyphs[index].geometry.width = width;
}

static void
add_cluster(PangoFont        *font,
	    PangoGlyphString *glyphs,
	    int              cluster_size,
	    int              cluster_start,
	    int              glyph_num[],
	    PangoGlyph       glyph[],
	    int              width[],
	    int              x_offset[],
	    int              y_offset[])
{
  int i;

  for (i=0; i<cluster_size; i++)
    {
      add_glyph (glyphs, cluster_start, glyph[i],
		 i == 0 ? FALSE : TRUE, width[i], x_offset[i], y_offset[i]);
    }
}


static void 
hebrew_engine_shape (PangoFont        *font,
		    const char       *text,
		    gint              length,
		    PangoAnalysis    *analysis,
		    PangoGlyphString *glyphs)
{
  const char *p;
  const char *log_cluster;
  gunichar cluster[MAX_CLUSTER_CHRS];
  gint cluster_size;
  gint glyph_num[MAX_CLUSTER_CHRS];
  gint glyph_width[MAX_CLUSTER_CHRS], x_offset[MAX_CLUSTER_CHRS], y_offset[MAX_CLUSTER_CHRS];
  PangoRectangle ink_rects[MAX_CLUSTER_CHRS];
  PangoGlyph glyph[MAX_CLUSTER_CHRS];
  FT_Face face;

  g_return_if_fail (font != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (length >= 0);
  g_return_if_fail (analysis != NULL);

  pango_glyph_string_set_size (glyphs, 0);
  face = pango_xft_font_get_face (font);
  g_assert (face);

  p = text;
  while (p < text + length)
    {
      log_cluster = p;
      p = hebrew_shaper_get_next_cluster (p, text + length - p,
					  /* output */
					  cluster, &cluster_size);
      get_cluster_glyphs(face,
			 font,
			 cluster,
			 cluster_size,
			 /* output */
			 glyph_num,
			 glyph,
			 glyph_width,
			 ink_rects);
      
      /* Kern the glyphs! */
      hebrew_shaper_get_cluster_kerning(cluster,
					cluster_size,
					/* Input and output */
					ink_rects,
					glyph_width,
					/* output */
					x_offset,
					y_offset);
      
      add_cluster(font,
		  glyphs,
		  cluster_size,
		  log_cluster - text,
		  glyph_num,
		  glyph,
		  glyph_width,
		  x_offset,
		  y_offset);
      
    }

  if (analysis->level % 2)
    hebrew_shaper_bidi_reorder(glyphs);
}

static PangoCoverage *
hebrew_engine_get_coverage (PangoFont  *font,
			   PangoLanguage *lang)
{
  return pango_font_get_coverage (font, lang);
}

static PangoEngine *
hebrew_engine_xft_new ()
{
  PangoEngineShape *result;
  
  result = g_new (PangoEngineShape, 1);

  result->engine.id = PANGO_RENDER_TYPE_XFT;
  result->engine.type = PANGO_ENGINE_TYPE_SHAPE;
  result->engine.length = sizeof (result);
  result->script_shape = hebrew_engine_shape;
  result->get_coverage = hebrew_engine_get_coverage;

  return (PangoEngine *)result;
}

/* The following three functions provide the public module API for
 * Pango. If we are compiling it is a module, then we name the
 * entry points script_engine_list, etc. But if we are compiling
 * it for inclusion directly in Pango, then we need them to
 * to have distinct names for this module, so we prepend
 * _pango_hebrew_xft_
 */
#ifdef XFT_MODULE_PREFIX
#define MODULE_ENTRY(func) _pango_hebrew_xft_##func
#else
#define MODULE_ENTRY(func) func
#endif

/* List the engines contained within this module
 */
void 
MODULE_ENTRY(script_engine_list) (PangoEngineInfo **engines, gint *n_engines)
{
  *engines = script_engines;
  *n_engines = G_N_ELEMENTS (script_engines);
}

/* Load a particular engine given the ID for the engine
 */
PangoEngine *
MODULE_ENTRY(script_engine_load) (const char *id)
{
  if (!strcmp (id, "HebrewScriptEngineXft"))
    return hebrew_engine_xft_new ();
  else
    return NULL;
}

void 
MODULE_ENTRY(script_engine_unload) (PangoEngine *engine)
{
}
