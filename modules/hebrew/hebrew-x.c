/* Pango
 * hebrew-x.c:
 *
 * Copyright (C) 1999 Red Hat Software
 * Author: Owen Taylor <otaylor@redhat.com>
 *
 * Copyright (c) 1996-2000 by Sun Microsystems, Inc.
 * Author: Chookij Vanatham <Chookij.Vanatham@Eng.Sun.COM>
 *
 * Hebrew points positioning improvements 2001
 * Author: Dov Grobgeld <dov@imagic.weizmann.ac.il>
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

#include <glib.h>
#include <string.h>
#include "pangox.h"
#include "pango-engine.h"
#include "hebrew-shaper.h"

#define ucs2iso8859_8(wc)		(unsigned int)((unsigned int)(wc) - 0x0590 + 0x10)
#define iso8859_8_2uni(c)		((gunichar)(c) - 0x10 + 0x0590)

#define SCRIPT_ENGINE_NAME "HebrewScriptEngineX"
#define MAX_CLUSTER_CHRS	20

static const gint iso_8859_8_shape_table[128] = {
  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,

  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,

  0xE0, 0xE1, 0xE2, 0xE3,  0xE4, 0xE5, 0xE6, 0xE7,
  0xE8, 0xE9, 0xEA, 0xEB,  0xEC, 0xED, 0xEE, 0xEF,
  0xF0, 0xF1, 0xF2, 0xF3,  0xF4, 0xF5, 0xF6, 0xF7,
  0xF8, 0xF9, 0xFA, 0x00,  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
};

/* Unicode Hebrew Font Layout
 */
static const gint Unicode_shape_table[128] = {
  /* 00 */    0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,
              0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x00,

  /* cantillation marks followed by accents */
  /* 10 */    0x0000, 0x0591, 0x0592, 0x0593, 0x0594, 0x0595, 0x0596, 0x0597,
              0x0598, 0x0599, 0x059A, 0x059B, 0x059C, 0x059D, 0x059E, 0x059F,
  /* 20 */    0x05A0, 0x05A1, 0x0000, 0x05A3, 0x05A4, 0x05A5, 0x05A6, 0x05A7,
              0x05A8, 0x05A9, 0x05AA, 0x05AB, 0x05AC, 0x05AD, 0x05AE, 0x05AF,
  /* 30 */    0x05B0, 0x05B1, 0x05B2, 0x05B3, 0x05B4, 0x05B5, 0x05B6, 0x05B7,
              0x05B8, 0x05B9, 0x0000, 0x05BB, 0x05BC, 0x05BD, 0x05BE, 0x05BF,
  /* 40 */    0x05C0, 0x05C1, 0x05C2, 0x05C3, 0x05C4, 0x0000, 0x0000, 0x0000,
              0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,

  /* Aleph-Tav, Yiddish ligatures, and punctuation */
  /* 50 */    0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7,
              0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF,
  /* 60 */    0x05E0, 0x05E1, 0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7,
              0x05E8, 0x05E9, 0x05EA, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  /* 70 */    0x05F0, 0x05F1, 0x05F2, 0x05F3, 0x05F4, 0x0000, 0x0000, 0x0000,
              0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
};

/* We handle the range U+0591 to U+05f4 exactly
 */
static PangoEngineRange hebrew_ranges[] = {
  { 0x0591, 0x05f4, "*" },  /* Hebrew */
};

static PangoEngineInfo script_engines[] = {
  {
    SCRIPT_ENGINE_NAME,
    PANGO_ENGINE_TYPE_SHAPE,
    PANGO_RENDER_TYPE_X,
    hebrew_ranges, G_N_ELEMENTS(hebrew_ranges)
  }
};

/*
 * X window system script engine portion
 */

typedef struct _HebrewFontInfo HebrewFontInfo;

/* The type of encoding that we will use
 */
typedef enum {
  HEBREW_FONT_NONE,
  HEBREW_FONT_ISO8859_8,
  HEBREW_FONT_ISO10646
} HebrewFontType;

struct _HebrewFontInfo
{
  PangoFont   *font;
  HebrewFontType type;
  PangoXSubfont subfont;
};

/* Returns a structure with information we will use to rendering given the
 * #PangoFont. This is computed once per font and cached for later retrieval.
 */
static HebrewFontInfo *
get_font_info (PangoFont *font)
{
  static const char *charsets[] = {
    "iso8859-8",
    "iso10646-1",
  };

  static const int charset_types[] = {
    HEBREW_FONT_ISO8859_8,
    HEBREW_FONT_ISO10646,
  };
  
  HebrewFontInfo *font_info;
  GQuark info_id = g_quark_from_string ("hebrew-font-info");
  
  font_info = g_object_get_qdata (G_OBJECT (font), info_id);

  if (!font_info)
    {
      /* No cached information not found, so we need to compute it
       * from scratch
       */
      PangoXSubfont *subfont_ids;
      gint *subfont_charsets;
      gint n_subfonts, i;

      font_info = g_new (HebrewFontInfo, 1);
      font_info->font = font;
      font_info->type = HEBREW_FONT_NONE;
      
      g_object_set_qdata_full (G_OBJECT (font), info_id, font_info, (GDestroyNotify)g_free);
      
      n_subfonts = pango_x_list_subfonts (font, (char **)charsets, G_N_ELEMENTS (charsets),
					  &subfont_ids, &subfont_charsets);

      for (i=0; i < n_subfonts; i++)
	{
	  HebrewFontType font_type = charset_types[subfont_charsets[i]];
	  
	  if (font_type == HEBREW_FONT_ISO10646 &&
	      pango_x_has_glyph (font, PANGO_X_MAKE_GLYPH (subfont_ids[i], 0x05D0)))
	    {
	      font_info->type = font_type;
	      font_info->subfont = subfont_ids[i];
	      
	      break;
	    }
	  else if (font_type == HEBREW_FONT_ISO8859_8 &&
		   pango_x_has_glyph (font, PANGO_X_MAKE_GLYPH (subfont_ids[i], 0xE0)))
	    {
	      font_info->type = font_type;
	      font_info->subfont = subfont_ids[i];
	      
	      break;
	    }
	}
      g_free (subfont_ids);
      g_free (subfont_charsets);
    }

  return font_info;
}

static void
add_glyph (HebrewFontInfo   *font_info, 
	   PangoGlyphString *glyphs, 
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

#if 0
  {
    PangoRectangle ink_rect, logical_rect;
    
    pango_font_get_glyph_extents (font_info->font,
				  glyphs->glyphs[index].glyph, &ink_rect, &logical_rect);

    printf("width logical_rect.width x_offset y_offset = %d %d %d %d\n", width, logical_rect.width, x_offset, y_offset);
    glyphs->glyphs[index].geometry.x_offset = 0;
    glyphs->glyphs[index].geometry.y_offset = 0;
    glyphs->glyphs[index].geometry.width = width;
  }
#endif
}

static PangoGlyph
get_glyph(HebrewFontInfo *font_info,
	  int glyph_num)
{
  return PANGO_X_MAKE_GLYPH (font_info->subfont, glyph_num);
}

static void
add_cluster(HebrewFontInfo   *font_info,
	    PangoFont        *font,
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
      add_glyph (font_info, glyphs, cluster_start, glyph[i],
		 i == 0 ? FALSE : TRUE, width[i], x_offset[i], y_offset[i]);
    }
}

gint get_glyph_num(HebrewFontInfo *font_info,
		   PangoFont *font,
		   gunichar  uch)
{
  if (font_info->type == HEBREW_FONT_ISO8859_8)
    {
      return iso_8859_8_shape_table[ucs2iso8859_8(uch)];
    }
  else
    {
      return Unicode_shape_table[ucs2iso8859_8(uch)];
    }
}

static void
get_cluster_glyphs(HebrewFontInfo *font_info,
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
      glyph_num[i] = get_glyph_num(font_info, font, cluster[i]);
      glyph[i] = get_glyph(font_info, glyph_num[i]);

      pango_font_get_glyph_extents (font_info->font,
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
hebrew_engine_shape (PangoFont        *font,
                     const char       *text,
                     gint              length,
                     PangoAnalysis    *analysis,
                     PangoGlyphString *glyphs)
{
  HebrewFontInfo *font_info;
  const char *p;
  const char *log_cluster;
  gunichar cluster[MAX_CLUSTER_CHRS];
  gint cluster_size;
  gint glyph_num[MAX_CLUSTER_CHRS];
  gint glyph_width[MAX_CLUSTER_CHRS], x_offset[MAX_CLUSTER_CHRS], y_offset[MAX_CLUSTER_CHRS];
  PangoRectangle ink_rects[MAX_CLUSTER_CHRS];
  PangoGlyph glyph[MAX_CLUSTER_CHRS];

  pango_glyph_string_set_size (glyphs, 0);

  font_info = get_font_info (font);

  p = text;
  while (p < text + length)
    {
	log_cluster = p;
	p = hebrew_shaper_get_next_cluster (p, text + length - p,
					    /* output */
					    cluster, &cluster_size);
	get_cluster_glyphs(font_info,
			   font,
			   cluster,
			   cluster_size,
			   /* output */
			   glyph_num,
			   glyph,
			   glyph_width,
			   ink_rects);
#if 0
	{
	  int i;
	  for (i=0; i< cluster_size; i++)
	    {
	      printf("cluster %d: U+%04x %d\n", i, glyph_num[i], glyph_width[i]);
	    }
	}
#endif
	
	/* Kern the glyphs! */
	hebrew_shaper_get_cluster_kerning(cluster,
					  cluster_size,
					  /* Input and output */
					  ink_rects,
					  glyph_width,
					  /* output */
					  x_offset,
					  y_offset);


#if 0
	old_add_cluster (font_info, glyphs, log_cluster - text, cluster, cluster_size);
#endif
	add_cluster(font_info,
		    font,
		    glyphs,
		    cluster_size,
		    log_cluster - text,
		    glyph_num,
		    glyph,
		    glyph_width,
		    x_offset,
		    y_offset);

    }

  /* Simple bidi support */
  if (analysis->level % 2)
    hebrew_shaper_bidi_reorder(glyphs);
}

static PangoCoverage *
hebrew_engine_get_coverage (PangoFont     *font,
                            PangoLanguage *lang)
{
  PangoCoverage *result = pango_coverage_new ();
  
  HebrewFontInfo *font_info = get_font_info (font);
  
  if (font_info->type != HEBREW_FONT_NONE)
    {
      gunichar wc;
      
      for (wc = 0x590; wc <= 0x5f4; wc++)
	pango_coverage_set (result, wc, PANGO_COVERAGE_EXACT);
    }

  return result;
}

static PangoEngine *
hebrew_engine_x_new ()
{
  PangoEngineShape *result;
  
  result = g_new (PangoEngineShape, 1);

  result->engine.id = SCRIPT_ENGINE_NAME;
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
 * _pango_hebrew_x_
 */
#ifdef X_MODULE_PREFIX
#define MODULE_ENTRY(func) _pango_hebrew_x_##func
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
  if (!strcmp (id, SCRIPT_ENGINE_NAME))
    return hebrew_engine_x_new ();
  else
    return NULL;
}

void 
MODULE_ENTRY(script_engine_unload) (PangoEngine *engine)
{
}

