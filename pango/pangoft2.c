/* Pango
 * pangoft2.c: Routines for handling FreeType2 fonts
 *
 * Copyright (C) 1999 Red Hat Software
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
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <glib.h>

#include <freetype/freetype.h>
#include <fribidi/fribidi.h>

#include "pangoft2.h"
#include "pangoft2-private.h"

#define PANGO_TYPE_FT2_FONT              (pango_ft2_font_get_type ())
#define PANGO_FT2_FONT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_FT2_FONT, PangoFT2Font))
#define PANGO_FT2_FONT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FT2_FONT, PangoFT2FontClass))
#define PANGO_FT2_IS_FONT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_FT2_FONT))
#define PANGO_FT2_IS_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_FT2_FONT))
#define PANGO_FT2_FONT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FT2_FONT, PangoFT2FontClass))

typedef struct _PangoFT2FontClass   PangoFT2FontClass;
typedef struct _PangoFT2MetricsInfo PangoFT2MetricsInfo;
typedef struct _PangoFT2ContextInfo PangoFT2ContextInfo;

struct _PangoFT2MetricsInfo
{
  const char *lang;
  PangoFontMetrics metrics;
};

struct _PangoFT2FontClass
{
  PangoFontClass parent_class;
};

static PangoFontClass *parent_class;	/* Parent class structure for PangoFT2Font */

static void pango_ft2_font_class_init (PangoFT2FontClass *class);
static void pango_ft2_font_init       (PangoFT2Font      *xfont);
static void pango_ft2_font_shutdown   (GObject         *object);
static void pango_ft2_font_finalize   (GObject         *object);

static PangoFontDescription *pango_ft2_font_describe          (PangoFont            *font);

static PangoCoverage *       pango_ft2_font_get_coverage      (PangoFont            *font,
							       const char           *lang);

static PangoEngineShape *    pango_ft2_font_find_shaper       (PangoFont            *font,
							       const char           *lang,
							       guint32               ch);

static void                  pango_ft2_font_get_glyph_extents (PangoFont            *font,
							       PangoGlyph            glyph,
							       PangoRectangle       *ink_rect,
							       PangoRectangle       *logical_rect);

static void                  pango_ft2_font_get_metrics       (PangoFont            *font,
							       const gchar          *lang,
							       PangoFontMetrics     *metrics);
  
static void                  pango_ft2_get_item_properties    (PangoItem      	    *item,
							       PangoUnderline 	    *uline,
							       PangoAttrColor 	    *fg_color,
							       gboolean       	    *fg_set,
							       PangoAttrColor 	    *bg_color,
							       gboolean       	    *bg_set);

static char *
pango_ft2_open_args_describe (PangoFT2OA *oa)
{
  if (oa->open_args->flags & ft_open_memory)
    return g_strdup_printf ("memory at %p", oa->open_args->memory_base);
  else if (oa->open_args->flags == ft_open_pathname)
    return g_strdup_printf ("file '%s'", oa->open_args->pathname);
  else if (oa->open_args->flags & ft_open_stream)
    return g_strdup_printf ("FT_Stream at %p", oa->open_args->stream);
  else
    return g_strdup_printf ("open_args at %p, face_index %ld", oa->open_args, oa->face_index);
}

static inline FT_Face
pango_ft2_get_face (PangoFont      *font,
		    PangoFT2Subfont subfont_index)
{
  PangoFT2Font *ft2font = (PangoFT2Font *)font;
  PangoFT2FontCache *cache;

  if (subfont_index < 1 || subfont_index > ft2font->n_fonts)
    {
      g_warning ("Invalid subfont %d", subfont_index);
      return NULL;
    }
  
  if (!ft2font->faces[subfont_index-1])
    {
      cache = pango_ft2_font_map_get_font_cache (ft2font->fontmap);
  
      ft2font->faces[subfont_index-1] =
	pango_ft2_font_cache_load (cache,
				   ft2font->oa[subfont_index-1]->open_args,
				   ft2font->oa[subfont_index-1]->face_index);

      if (!ft2font->faces[subfont_index-1])
	g_warning ("Cannot load font for %s",
		   pango_ft2_open_args_describe (ft2font->oa[subfont_index-1]));
    }
  return ft2font->faces[subfont_index-1];
}

/**
 * pango_ft2_get_context:
 * 
 * Retrieves a #PangoContext appropriate for rendering with Pango fonts.
  * 
 * Return value: the new #PangoContext
 **/
PangoContext *
pango_ft2_get_context (void)
{
  PangoContext *result;

  result = pango_context_new ();
  pango_context_add_font_map (result, pango_ft2_font_map_for_display ());

  return result;
}

static GType
pango_ft2_font_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoFT2FontClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_ft2_font_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoFT2Font),
        0,              /* n_preallocs */
        (GInstanceInitFunc) pango_ft2_font_init,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT,
                                            "PangoFT2Font",
                                            &object_info);
    }
  
  return object_type;
}

static void 
pango_ft2_font_init (PangoFT2Font *ft2font)
{
  ft2font->oa = NULL;
  ft2font->faces = NULL;

  ft2font->n_fonts = 0;

  ft2font->metrics_by_lang = NULL;

  ft2font->entry = NULL;
}

static void
pango_ft2_font_class_init (PangoFT2FontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);
  
  object_class->finalize = pango_ft2_font_finalize;
  object_class->shutdown = pango_ft2_font_shutdown;
  
  font_class->describe = pango_ft2_font_describe;
  font_class->get_coverage = pango_ft2_font_get_coverage;
  font_class->find_shaper = pango_ft2_font_find_shaper;
  font_class->get_glyph_extents = pango_ft2_font_get_glyph_extents;
  font_class->get_metrics = pango_ft2_font_get_metrics;
}

/**
 * pango_ft2_load_font:
 *
 * Loads a logical font based on XXX
 *
 * Returns a new #PangoFont
 */
PangoFont *
pango_ft2_load_font (PangoFontMap  *fontmap,
		     FT_Open_Args **open_args,
		     FT_Long       *face_indices,
		     int            n_fonts,
		     int            size)
{
  PangoFT2Font *result;
  int i;

  g_return_val_if_fail (fontmap != NULL, NULL);
  g_return_val_if_fail (open_args != NULL, NULL);
  g_return_val_if_fail (face_indices != NULL, NULL);
  g_return_val_if_fail (n_fonts > 0, NULL);

  result = (PangoFT2Font *)g_type_create_instance (PANGO_TYPE_FT2_FONT);
  
  result->fontmap = fontmap;
  g_object_ref (G_OBJECT (result->fontmap));

  result->oa = g_new (PangoFT2OA *, n_fonts);
  result->faces = g_new (FT_Face, n_fonts);
  result->n_fonts = n_fonts;
  result->size = size;

  for (i = 0; i < n_fonts; i++)
    {
      result->oa[i] = g_new (PangoFT2OA, 1);
      result->oa[i]->open_args = open_args[i];
      result->oa[i]->face_index = face_indices[i];
      result->faces[i] = NULL;
    }

  return &result->font;
}
 
/**
 * pango_ft2_render:
 *
 * @bitmap:  the FreeType2 bitmap onto which draw the string
 * @font:    the font in which to draw the string
 * @glyphs:  the glyph string to draw
 * @x:       the x position of start of string (in pixels)
 * @y:       the y position of baseline (in pixels)
 *
 * Render a PangoGlyphString onto a FreeType2 bitmap
 */
void 
pango_ft2_render (FT_Bitmap        *bitmap,
		  PangoFont        *font,
		  PangoGlyphString *glyphs,
		  int               x, 
		  int               y)
{
  FT_Face face;
  FT_Face prev_face = NULL;
  FT_UInt glyph_index, prev_index;
  int i;
  int x_position = 0;
  int ix, iy, ixoff, iyoff, y_start, y_limit, x_start, x_limit;
  guint16 char_index;
  PangoFT2Subfont subfont_index;
  PangoGlyphInfo *gi;
  guchar *p, *q;

  g_return_if_fail (bitmap != NULL);
  g_return_if_fail (glyphs != NULL);

  PING (("bitmap: %dx%d@+%d+%d\n", bitmap->width, bitmap->rows, x, y));

  gi = glyphs->glyphs;
  for (i = 0; i < glyphs->num_glyphs; i++, gi++)
    {
      if (gi->glyph)
	{
	  char_index = PANGO_FT2_GLYPH_INDEX (gi->glyph);
	  subfont_index = PANGO_FT2_GLYPH_SUBFONT (gi->glyph);
	  face = pango_ft2_get_face (font, subfont_index);

	  if (face)
	    {
	      /* Draw glyph */
	      glyph_index = FT_Get_Char_Index (face, char_index);
	      /* FIXME hint or not? */ 
	      FT_Load_Glyph (face, glyph_index, FT_LOAD_DEFAULT);
	      FT_Render_Glyph (face->glyph, ft_render_mode_normal);

	      ixoff = x + PANGO_PIXELS (x_position + gi->geometry.x_offset);
	      iyoff = y + PANGO_PIXELS (gi->geometry.y_offset);

	      x_start = MAX (0, -face->glyph->bitmap_left - ixoff);
	      x_limit = MIN (face->glyph->bitmap.width, face->glyph->bitmap_left - ixoff + bitmap->width);

	      y_start = MAX (0, face->glyph->bitmap_top - iyoff);
	      y_limit = MIN (face->glyph->bitmap.rows, face->glyph->bitmap_top - iyoff + bitmap->rows);


	      PING (("glyph %d:%d: bitmap: %dx%d, left:%d top:%d\n",
		     i, glyph_index,
		     face->glyph->bitmap.width, face->glyph->bitmap.rows,
		     face->glyph->bitmap_left, face->glyph->bitmap_top));
	      PING (("xstart:%d xlim:%d ystart:%d ylim:%d\n",
		     x_start, x_limit, y_start, y_limit));

	      if (face->glyph->bitmap.pixel_mode == ft_pixel_mode_grays)
		for (iy = y_start; iy < y_limit; iy++)
		  {
		    p = bitmap->buffer +
		      (iyoff - face->glyph->bitmap_top + iy) * bitmap->pitch +
		      ixoff +
		      face->glyph->bitmap_left + x_start;
		    
		    q = face->glyph->bitmap.buffer + iy*face->glyph->bitmap.pitch;
		    for (ix = x_start; ix < x_limit; ix++)
		      {
			*p = MIN (*p, 0xFF - *q);
			q++;
			p++;
		      }
		  }
	      else if (face->glyph->bitmap.pixel_mode == ft_pixel_mode_mono)
		for (iy = y_start; iy < y_limit; iy++)
		  {
		    p = bitmap->buffer +
		      (iyoff - face->glyph->bitmap_top + iy) * bitmap->pitch +
		      ixoff +
		      face->glyph->bitmap_left + x_start;
		    
		    q = face->glyph->bitmap.buffer + iy*face->glyph->bitmap.pitch;
		    for (ix = x_start; ix < x_limit; ix++)
		      {
			if ((*q) & (1 << (7 - (ix%8))))
			  *p = 0;
			else
			  *p = MIN (*p, 0xFF);
			if ((ix%8) == 7)
			  q++;
			p++;
		      }
		  }
	      else
		g_warning ("pango_ft2_render: Unrecognized glyph bitmap pixel mode %d\n",
			   face->glyph->bitmap.pixel_mode);

	      prev_face = face;
	      prev_index = glyph_index;
	    }
	}

      x_position += glyphs->glyphs[i].geometry.width;
    }
}

static FT_Glyph_Metrics *
pango_ft2_get_per_char (PangoFont       *font,
			PangoFT2Subfont  subfont_index,
			guint16          char_index)
{
  PangoFT2Font *ft2font = (PangoFT2Font *)font;
  FT_Face face;
  FT_UInt glyph_index;
  FT_Error error;

  if (!(face = pango_ft2_get_face (font, subfont_index)))
    return NULL;

  glyph_index = FT_Get_Char_Index (face, char_index);
  if (!glyph_index)
    return NULL;

  error = FT_Set_Pixel_Sizes (face, PANGO_PIXELS (ft2font->size), 0);
  if (error)
    g_warning ("Error in FT_Set_Pixel_Sizes: %d", error);

  FT_Load_Glyph (face, glyph_index, FT_LOAD_DEFAULT);
  return &face->glyph->metrics;
}

static void
pango_ft2_font_get_glyph_extents (PangoFont      *font,
				  PangoGlyph      glyph,
				  PangoRectangle *ink_rect,
				  PangoRectangle *logical_rect)
{
  guint16 char_index = PANGO_FT2_GLYPH_INDEX (glyph);
  PangoFT2Subfont subfont_index = PANGO_FT2_GLYPH_SUBFONT (glyph);
  FT_Glyph_Metrics *gm;

  if (glyph && (gm = pango_ft2_get_per_char (font, subfont_index, char_index)))
    {
      if (ink_rect)
	{
	  ink_rect->x = PANGO_UNITS_26_6 (gm->horiBearingX);
	  ink_rect->width = PANGO_UNITS_26_6 (gm->width);
	  ink_rect->y = -PANGO_UNITS_26_6 (gm->horiBearingY);
	  ink_rect->height = PANGO_UNITS_26_6 (gm->height);
	}
      if (logical_rect)
	{
	  FT_Face face = pango_ft2_get_face (font, subfont_index);

	  logical_rect->x = 0;
	  logical_rect->width = PANGO_UNITS_26_6 (gm->horiAdvance);
	  logical_rect->y = -PANGO_UNITS_26_6 (face->size->metrics.ascender + 64);
	  /* Some fonts report negative descender, some positive ! (?) */
	  logical_rect->height = PANGO_UNITS_26_6 (face->size->metrics.ascender + ABS (face->size->metrics.descender) + 128);
	}
      PING (("glyph:%d logical: %dx%d@%d+%d\n",
	     char_index, logical_rect->width, logical_rect->height,
	     logical_rect->x, logical_rect->y));
    }
  else
    {
      if (ink_rect)
	{
	  ink_rect->x = 0;
	  ink_rect->width = 0;
	  ink_rect->y = 0;
	  ink_rect->height = 0;
	}
      if (logical_rect)
	{
	  logical_rect->x = 0;
	  logical_rect->width = 0;
	  logical_rect->y = 0;
	  logical_rect->height = 0;
	}
    }
}

int
pango_ft2_font_get_kerning (PangoFont *font,
			    PangoGlyph left,
			    PangoGlyph right)
{
  PangoFT2Subfont subfont_index;
  guint16 left_char_index;
  guint16 right_char_index;
  FT_Face face;
  FT_UInt left_glyph_index, right_glyph_index;
  FT_Error error;
  FT_Vector kerning;

  subfont_index = PANGO_FT2_GLYPH_SUBFONT (left);
  if (PANGO_FT2_GLYPH_SUBFONT (right) != subfont_index)
    return 0;
  
  face = pango_ft2_get_face (font, subfont_index);
  if (!face)
    return 0;

  if (!FT_HAS_KERNING (face))
    return 0;

  left_char_index = PANGO_FT2_GLYPH_INDEX (left);
  right_char_index = PANGO_FT2_GLYPH_INDEX (right);

  left_glyph_index = FT_Get_Char_Index (face, left_char_index);
  right_glyph_index = FT_Get_Char_Index (face, right_char_index);
  if (!left_glyph_index || !right_char_index)
    return 0;

  error = FT_Get_Kerning (face, left_glyph_index, right_glyph_index,
			  ft_kerning_default, &kerning);
  if (error != FT_Err_Ok)
    g_warning ("FT_Get_Kerning returns error: %s",
	       pango_ft2_ft_strerror (error));

  return PANGO_UNITS_26_6 (kerning.x);
}

/* Get composite font metrics for all subfonts in list
 */
static void
get_font_metrics_from_subfonts (PangoFont        *font,
				GSList           *subfonts,
				PangoFontMetrics *metrics)
{
  GSList *tmp_list = subfonts;
  gboolean first = TRUE;
  
  metrics->ascent = 0;
  metrics->descent = 0;
  
  while (tmp_list)
    {
      FT_Face face = pango_ft2_get_face (font, GPOINTER_TO_UINT (tmp_list->data));
      
      g_assert (face != NULL);

      if (first)
	{
	  metrics->ascent = PANGO_UNITS_26_6 (face->ascender);
	  metrics->descent = PANGO_UNITS_26_6 (face->descender);
	  first = FALSE;
	}
      else
	{
	  metrics->ascent = MAX (PANGO_UNITS_26_6 (face->ascender), metrics->ascent);
	  metrics->descent = MAX (PANGO_UNITS_26_6 (face->descender), metrics->descent);
	}

      tmp_list = tmp_list->next;
    }
}

/* Get composite font metrics for all subfonts resulting from shaping
 * string str with the given font
 *
 * This duplicates quite a bit of code from pango_itemize. This function
 * should die and we should simply add the ability to specify particular
 * fonts when itemizing.
 */
static void
get_font_metrics_from_string (PangoFont        *font,
			      const char       *lang,
			      const char       *str,
			      PangoFontMetrics *metrics)
{
  const char *start, *p;
  PangoGlyphString *glyph_str = pango_glyph_string_new ();
  PangoEngineShape *shaper, *last_shaper;
  int last_level;
  gunichar *text_ucs4;
  int n_chars, i;
  guint8 *embedding_levels;
  FriBidiCharType base_dir = PANGO_DIRECTION_LTR;
  GSList *subfonts = NULL;
  
  n_chars = g_utf8_strlen (str, -1);

  text_ucs4 = g_utf8_to_ucs4 (str, strlen (str));
  if (!text_ucs4)
    return;

  embedding_levels = g_new (guint8, n_chars);
  fribidi_log2vis_get_embedding_levels (text_ucs4, n_chars, &base_dir,
					embedding_levels);
  g_free (text_ucs4);

  last_shaper = NULL;
  last_level = 0;
  
  i = 0;
  p = start = str;
  while (*p)
    {
      gunichar wc = g_utf8_get_char (p);
      p = g_utf8_next_char (p);
	  
      shaper = pango_font_find_shaper (font, lang, wc);
      if (p > start &&
	  (shaper != last_shaper || last_level != embedding_levels[i]))
	{
	  PangoAnalysis analysis;
	  int j;

	  analysis.shape_engine = shaper;
	  analysis.lang_engine = NULL;
	  analysis.font = font;
	  analysis.level = last_level;
	  
	  pango_shape (start, p - start, &analysis, glyph_str);

	  for (j = 0; j < glyph_str->num_glyphs; j++)
	    {
	      PangoFT2Subfont subfont_index = PANGO_FT2_GLYPH_SUBFONT (glyph_str->glyphs[j].glyph);
	      if (!g_slist_find (subfonts, GUINT_TO_POINTER ((guint)subfont_index)))
		subfonts = g_slist_prepend (subfonts, GUINT_TO_POINTER ((guint)subfont_index));
	    }
	  
	  start = p;
	}

      last_shaper = shaper;
      last_level = embedding_levels[i];
      i++;
    }

  get_font_metrics_from_subfonts (font, subfonts, metrics);
  g_slist_free (subfonts);
  
  pango_glyph_string_free (glyph_str);
  g_free (embedding_levels);
}

typedef struct {
  const char *lang;
  const char *str;
} LangInfo;

int
lang_info_compare (const void *key,
		   const void *val)
{
  const LangInfo *lang_info = val;

  return strncmp (key, lang_info->lang, 2);
}

/* The following array is supposed to contain enough text to tickle all necessary fonts for each
 * of the languages in the following. Yes, it's pretty lame. Not all of the languages
 * in the following have sufficient text to excercise all the accents for the language, and
 * there are obviously many more languages to include as well.
 */
LangInfo lang_texts[] = {
  { "ar", "Arabic  Ø§Ù,DX³ÙX§Ù… Ø¹ÙY(JY,CY…(B" },
  { "cs", "Czech (Äesky)  DobrÃ½ den" },
  { "da", "Danish (Dansk)  Hej, Goddag" },
  { "el", "Greek (Î$(GN;Î»Î·Î½Î¹ÎºÎ¬(B) Î$(CN5Î¹Î¬ Ï,CN±Ï‚(B" },
  { "en", "English Hello" },
  { "eo", "Esperanto Saluton" },
  { "es", "Spanish (EspaÃ±ol) Â¡Hola!" },
  { "et", "Estonian  Tere, Tervist" },
  { "fi", "Finnish (Suomi)  Hei, HyvÃ¤Ã¤ pÃ¤ivÃ¤Ã¤" },
  { "fr", "French (FranÃ§ais)" },
  { "de", "German GrÃ¼ÃŸ Gott" },
  { "iw", "Hebrew   ×©×œ×•×" },
  { "il", "Italiano  Ciao, Buon giorno" },
  { "ja", "Japanese (æ—¥æ¬èªž) ã“ã‚“ã«ã¡ã¯, ï½ºï¾ï¾,Fo¾ï¾Š(B" },
  { "ko", "Korean (í•œê¸€)   ì•,Hk…•í•˜ì,D8ìš”(B, ì•,Hk…•í•˜ì‹­ë‹j¹Œ(B" },
  { "mt", "Maltese   ÄŠaw, SaÄ§Ä§a" },
  { "nl", "Nederlands, Vlaams Hallo, Dag" },
  { "no", "Norwegian (Norsk) Hei, God dag" },
  { "pl", "Polish   DzieÅ„ dobry, Hej" },
  { "ru", "Russian (Ð Ñ,CQÑÐºÐ¸Ð¹(B)" },
  { "sk", "Slovak   DobrÃ½ deÅˆ" },
  { "sv", "Swedish (Svenska) Hej pÃ¥ dej, Goddag" },
  { "tr", "Turkish (TÃ¼rkÃ§e) Merhaba" },
  { "zh", "Chinese (ä¸­æ–‡,æ$(1.i€è¯(B,æ±(Ih¯­(B)" }
};

static void
pango_ft2_font_get_metrics (PangoFont        *font,
			    const gchar      *lang,
			    PangoFontMetrics *metrics)
{
  PangoFT2MetricsInfo *info;
  PangoFT2Font *ft2font = (PangoFT2Font *)font;
  GSList *tmp_list;
      
  const char *lookup_lang;
  const char *str;

  if (lang)
    {
      LangInfo *lang_info = bsearch (lang, lang_texts,
				     G_N_ELEMENTS (lang_texts), sizeof (LangInfo),
				     lang_info_compare);

      if (lang_info)
	{
	  lookup_lang = lang_info->lang;
	  str = lang_info->str;
	}
      else
	{
	  lookup_lang = "UNKNOWN";
	  str = "French (FranÃ§ais)";     /* Assume iso-8859-1 */
	}
    }
  else
    {
      lookup_lang = "NONE";

      /* Complete junk
       */
      str = "Ø§Ù,DX³ÙX§Ù… Ø¹ÙY(JY,CY… Ä(Besky Î$(GN;Î»Î·Î½Î¹ÎºÎ¬ (BFranÃ§ais æ—¥æ¬èªž í•œê¸€ Ð Ñ,CQÑÐºÐ¸Ð¹ ä¸­æ–‡(B,æ$(1.i€è¯(B,æ±(Ih¯­ (BTÃ¼rkÃ§e";
    }
  
  tmp_list = ft2font->metrics_by_lang;
  while (tmp_list)
    {
      info = tmp_list->data;
      
      if (info->lang == lookup_lang)        /* We _don't_ need strcmp */
	break;

      tmp_list = tmp_list->next;
    }

  if (!tmp_list)
    {
      info = g_new (PangoFT2MetricsInfo, 1);
      info->lang = lookup_lang;

      ft2font->metrics_by_lang = g_slist_prepend (ft2font->metrics_by_lang, info);
      
      get_font_metrics_from_string (font, lang, str, &info->metrics);
    }
      
  *metrics = info->metrics;
}

/**
 * pango_ft2_n_subfonts:
 * @font: a PangoFont
 * Returns number of subfonts in a PangoFT2Font.
 **/
int
pango_ft2_n_subfonts (PangoFont *font)
{
  PangoFT2Font *ft2font = (PangoFT2Font *)font;

  g_return_val_if_fail (font != NULL, 0);

  return ft2font->n_fonts;
}

/**
 * pango_ft2_has_glyph:
 * @font: a #PangoFont which must be from the FreeType2 backend.
 * @glyph: the index of a glyph in the font. (Formed
 *         using the PANGO_FT2_MAKE_GLYPH macro)
 * 
 * Check if the given glyph is present in a FT2 font.
 * 
 * Return value: %TRUE if the glyph is present.
 **/
gboolean
pango_ft2_has_glyph (PangoFont  *font,
		     PangoGlyph  glyph)
{
  guint16 char_index = PANGO_FT2_GLYPH_INDEX (glyph);
  PangoFT2Subfont subfont_index = PANGO_FT2_GLYPH_SUBFONT (glyph);
  FT_Face face = pango_ft2_get_face (font, subfont_index);

  if (!face)
    return FALSE;
  
  if (FT_Get_Char_Index (face, char_index) == 0)
    return FALSE;
  else
    return TRUE;
}

/**
 * pango_ft2_font_subfont_open_args:
 * @font: a #PangoFont which must be from the FT2 backend
 * @open_args: pointer where to store the #FT_Open_Args for this subfont
 * @face_index: pointer where to store the face index for this subfont
 * 
 * Determine the FT_Open_Args and face index for the specified subfont.
 **/
void
pango_ft2_font_subfont_open_args (PangoFont        *font,
				  PangoFT2Subfont   subfont_id,
				  FT_Open_Args    **open_args,
				  FT_Long          *face_index)
{
  PangoFT2Font *ft2font = (PangoFT2Font *)font;
  *open_args = NULL;
  *face_index = 0;

  g_return_if_fail (font != NULL);
  g_return_if_fail (PANGO_FT2_IS_FONT (font));

  if (subfont_id < 1 || subfont_id > ft2font->n_fonts)
    g_warning ("pango_ft2_font_subfont_open_args: Invalid subfont_id specified");
  else
    {
      *open_args = ft2font->oa[subfont_id-1]->open_args;
      *face_index = ft2font->oa[subfont_id-1]->face_index;
    }
}

static void
pango_ft2_font_shutdown (GObject *object)
{
  PangoFT2Font *ft2font = PANGO_FT2_FONT (object);

  /* If the font is not already in the freed-fonts cache, add it,
   * if it is already there, do nothing and the font will be
   * freed.
   */
  if (!ft2font->in_cache && ft2font->fontmap)
    pango_ft2_fontmap_cache_add (ft2font->fontmap, ft2font);

  G_OBJECT_CLASS (parent_class)->shutdown (object);
}


static void
pango_ft2_font_finalize (GObject *object)
{
  PangoFT2Font *ft2font = (PangoFT2Font *)object;
  PangoFT2FontCache *cache = pango_ft2_font_map_get_font_cache (ft2font->fontmap);
  int i;

  PING (("\n"));

  for (i = 0; i < ft2font->n_fonts; i++)
    {
      if (ft2font->faces[i])
	pango_ft2_font_cache_unload (cache, ft2font->faces[i]);
    }

  g_free (ft2font->oa);
  g_free (ft2font->faces);

  g_slist_foreach (ft2font->metrics_by_lang, (GFunc)g_free, NULL);
  g_slist_free (ft2font->metrics_by_lang);
  
  if (ft2font->entry)
    pango_ft2_font_entry_remove (ft2font->entry, (PangoFont *)ft2font);

  g_object_unref (G_OBJECT (ft2font->fontmap));

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static PangoFontDescription *
pango_ft2_font_describe (PangoFont *font)
{
  /* FIXME: implement */
  return NULL;
}

PangoMap *
pango_ft2_get_shaper_map (const char *lang)
{
  static guint engine_type_id = 0;
  static guint render_type_id = 0;
  
  if (engine_type_id == 0)
    {
      engine_type_id = g_quark_from_static_string (PANGO_ENGINE_TYPE_SHAPE);
      render_type_id = g_quark_from_static_string (PANGO_RENDER_TYPE_FT2);
    }
  
  return pango_find_map (lang, engine_type_id, render_type_id);
}

static PangoCoverage *
pango_ft2_font_get_coverage (PangoFont  *font,
			     const char *lang)
{
  PangoFT2Font *ft2font = (PangoFT2Font *)font;

  return pango_ft2_font_entry_get_coverage (ft2font->entry, font, lang);
}

static PangoEngineShape *
pango_ft2_font_find_shaper (PangoFont   *font,
			    const gchar *lang,
			    guint32      ch)
{
  PangoMap *shape_map = NULL;

  shape_map = pango_ft2_get_shaper_map (lang);
  return (PangoEngineShape *)pango_map_get_engine (shape_map, ch);
}

/* Utility functions */

/**
 * pango_ft2_get_unknown_glyph:
 * @font: a #PangoFont
 * 
 * Return the index of a glyph suitable for drawing unknown characters.
 * 
 * Return value: a glyph index into @font
 **/
PangoGlyph
pango_ft2_get_unknown_glyph (PangoFont *font)
{
  return PANGO_FT2_MAKE_GLYPH (1, 0);
}

/**
 * pango_ft2_render_layout_line:
 * @line:      a #PangoLayoutLine
 * @x:         the x position of start of string (in pixels)
 * @y:         the y position of baseline (in pixels)
 *
 * Render a #PangoLayoutLine onto a FreeType2 bitmap
 */
void 
pango_ft2_render_layout_line (FT_Bitmap       *bitmap,
			      PangoLayoutLine *line,
			      int              x, 
			      int              y)
{
  GSList *tmp_list = line->runs;
  PangoRectangle overall_rect;
  PangoRectangle logical_rect;
  PangoRectangle ink_rect;
  unsigned char *p;
  int ix;
  int x_off = 0;
  int x_limit;

  pango_layout_line_get_extents (line,NULL, &overall_rect);
  
  while (tmp_list)
    {
      PangoUnderline uline = PANGO_UNDERLINE_NONE;
      PangoLayoutRun *run = tmp_list->data;
      PangoAttrColor fg_color, bg_color;
      gboolean fg_set, bg_set;
      
      tmp_list = tmp_list->next;

      pango_ft2_get_item_properties (run->item, &uline, &fg_color, &fg_set, &bg_color, &bg_set);

      if (uline == PANGO_UNDERLINE_NONE)
	pango_glyph_string_extents (run->glyphs, run->item->analysis.font,
				    NULL, &logical_rect);
      else
	pango_glyph_string_extents (run->glyphs, run->item->analysis.font,
				    &ink_rect, &logical_rect);

      pango_ft2_render (bitmap, run->item->analysis.font, run->glyphs,
			x + PANGO_PIXELS (x_off), y);

      x_limit = PANGO_PIXELS (ink_rect.width);
      switch (uline)
	{
	case PANGO_UNDERLINE_NONE:
	  break;
	case PANGO_UNDERLINE_DOUBLE:
	  p = bitmap->buffer +
	    (y + 4) * bitmap->pitch +
	    x + PANGO_PIXELS (x_off + ink_rect.x) - 1;

	  /* Don't drawn the underline through descenders */
	  for (ix = 0; ix < x_limit; ix++)
	    if (*p == 0xFF &&
		(ix == x_limit - 1 || p[1] == 0xFF))
	      *p++ = 0;
	  /* Fall through */
	case PANGO_UNDERLINE_SINGLE:
	  p = bitmap->buffer +
	    (y + 2) * bitmap->pitch +
	    x + PANGO_PIXELS (x_off + ink_rect.x) - 1;
	  for (ix = 0; ix < x_limit; ix++)
	    if (*p == 0xFF &&
		(ix == x_limit - 1 || p[1] == 0xFF))
	      *p++ = 0;
	  break;
	case PANGO_UNDERLINE_LOW:
	  p = bitmap->buffer +
	    (y + PANGO_PIXELS (ink_rect.y + ink_rect.height)) * bitmap->pitch +
	    x + PANGO_PIXELS (x_off + ink_rect.x) - 1;
	  for (ix = 0; ix < PANGO_PIXELS (ink_rect.width); ix++)
	    *p++ = 0;
	  break;
	}

      x_off += logical_rect.width;
    }
}

/**
 * pango_ft2_render_layout:
 * @layout:    a #PangoLayout
 * @x:         the X position of the left of the layout (in pixels)
 * @y:         the Y position of the top of the layout (in pixels)
 *
 * Render a #PangoLayoutLine onto a FreeType2 bitmap
 */
void 
pango_ft2_render_layout (FT_Bitmap   *bitmap,
			 PangoLayout *layout,
			 int         x, 
			 int         y)
{
  PangoRectangle logical_rect;
  GSList *tmp_list;
  PangoAlignment align;
  int indent;
  int width;
  int y_offset = 0;

  gboolean first = FALSE;
  
  g_return_if_fail (bitmap != NULL);
  g_return_if_fail (layout != NULL);

  indent = pango_layout_get_indent (layout);
  width = pango_layout_get_width (layout);
  align = pango_layout_get_alignment (layout);

  PING (("x:%d y:%d indent:%d width:%d\n", x, y, indent, width));

  if (width == -1 && align != PANGO_ALIGN_LEFT)
    {
      pango_layout_get_extents (layout, NULL, &logical_rect);
      width = logical_rect.width;
    }
  
  tmp_list = pango_layout_get_lines (layout);
  while (tmp_list)
    {
      PangoLayoutLine *line = tmp_list->data;
      int x_offset;
      
      pango_layout_line_get_extents (line, NULL, &logical_rect);

      if (width != 1 && align == PANGO_ALIGN_RIGHT)
	x_offset = width - logical_rect.width;
      else if (width != 1 && align == PANGO_ALIGN_CENTER)
	x_offset = (width - logical_rect.width) / 2;
      else
	x_offset = 0;

      if (first)
	{
	  if (indent > 0)
	    {
	      if (align == PANGO_ALIGN_LEFT)
		x_offset += indent;
	      else
		x_offset -= indent;
	    }

	  first = FALSE;
	}
      else
	{
	  if (indent < 0)
	    {
	      if (align == PANGO_ALIGN_LEFT)
		x_offset -= indent;
	      else
		x_offset += indent;
	    }
	}

      PING (("x_offset:%d y_offset:%d logical_rect.y:%d logical_rect.height:%d\n", x_offset, y_offset, logical_rect.y, logical_rect.height));

      pango_ft2_render_layout_line (bitmap, line,
				    x + PANGO_PIXELS (x_offset),
				    y + PANGO_PIXELS (y_offset - logical_rect.y));

      y_offset += logical_rect.height;
      tmp_list = tmp_list->next;
    }
}

/* This utility function is duplicated here and in pango-layout.c; should it be
 * public? Trouble is - what is the appropriate set of properties?
 */
static void
pango_ft2_get_item_properties (PangoItem      *item,
			       PangoUnderline *uline,
			       PangoAttrColor *fg_color,
			       gboolean       *fg_set,
			       PangoAttrColor *bg_color,
			       gboolean       *bg_set)
{
  GSList *tmp_list = item->extra_attrs;

  if (fg_set)
    *fg_set = FALSE;
  
  if (bg_set)
    *bg_set = FALSE;
  
  while (tmp_list)
    {
      PangoAttribute *attr = tmp_list->data;

      switch (attr->klass->type)
	{
	case PANGO_ATTR_UNDERLINE:
	  if (uline)
	    *uline = ((PangoAttrInt *)attr)->value;
	  break;
	  
	case PANGO_ATTR_FOREGROUND:
	  if (fg_color)
	    *fg_color = *((PangoAttrColor *)attr);
	  if (fg_set)
	    *fg_set = TRUE;
	  
	  break;
	  
	case PANGO_ATTR_BACKGROUND:
	  if (bg_color)
	    *bg_color = *((PangoAttrColor *)attr);
	  if (bg_set)
	    *bg_set = TRUE;
	  
	  break;
	  
	default:
	  break;
	}
      tmp_list = tmp_list->next;
    }
}

typedef struct
{
  FT_Error     code;
  const char*  msg;
} ft_error_description;

static int
ft_error_compare (const void *pkey,
		  const void *pbase)
{
  return ((ft_error_description *) pkey)->code - ((ft_error_description *) pbase)->code;
}

const char *
pango_ft2_ft_strerror (FT_Error error)
{
#undef FTERRORS_H
#define FT_ERRORDEF( e, v, s )  { e, s },
#define FT_ERROR_START_LIST  {
#define FT_ERROR_END_LIST    { 0, 0 } }

  const ft_error_description ft_errors[] =
#include <freetype/fterrors.h>
  ;

#undef FT_ERRORDEF
#undef FT_ERROR_START_LIST
#undef FT_ERROR_END_LIST

  ft_error_description *found =
    bsearch (&error, ft_errors, G_N_ELEMENTS (ft_errors) - 1,
	     sizeof (ft_errors[0]), ft_error_compare);
  if (found != NULL)
    return found->msg;
  else
    {
      static char default_msg[100];

      sprintf (default_msg, "Unknown FreeType2 error %#x", error);
      return default_msg;
    }
}
