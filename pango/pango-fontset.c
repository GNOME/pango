/* Pango
 * pango-fontset.c:
 *
 * Copyright (C) 2001 Red Hat Software
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

/*
 * PangoFontset
 */

#include "pango-types.h"
#include "pango-font.h"
#include "pango-fontset.h"
#include "pango-utils.h"

static void              pango_fontset_class_init       (PangoFontsetClass *class);
static PangoFontMetrics *pango_fontset_real_get_metrics (PangoFontset      *fontset);

GType
pango_fontset_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoFontsetClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_fontset_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoFontset),
        0,              /* n_preallocs */
	NULL            /* init */
      };
      
      object_type = g_type_register_static (G_TYPE_OBJECT,
                                            "PangoFontset",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void
pango_fontset_class_init (PangoFontsetClass *class)
{
  class->get_metrics = pango_fontset_real_get_metrics;
}

/**
 * pango_fontset_get_font:
 * @fontset: a #PangoFontset
 * @wc: a unicode character
 *
 * Returns the font in the fontset that contains the best glyph for the
 * unicode character wc.
 *
 * Returns: a #PangoFont. The caller must call g_object_unref when finished
 *          with the font.
 **/
PangoFont *
pango_fontset_get_font (PangoFontset  *fontset,
			guint          wc)
{
  
  g_return_val_if_fail (fontset != NULL, NULL);

  return PANGO_FONTSET_GET_CLASS (fontset)->get_font (fontset, wc);
}

/**
 * pango_fontset_get_metrics:
 * @fontset: a #PangoFontset
 * 
 * Get overall metric information for the fonts in the fontset.
 *
 * Returns: a #PangoMetrics object. The caller must call pango_font_metrics_unref()
 *   when finished using the object.
 **/
PangoFontMetrics *
pango_fontset_get_metrics (PangoFontset  *fontset)
{
  g_return_val_if_fail (fontset != NULL, NULL);

  return PANGO_FONTSET_GET_CLASS (fontset)->get_metrics (fontset);
}


static PangoFontMetrics *
pango_fontset_real_get_metrics (PangoFontset  *fontset)
{
  PangoFontMetrics *metrics, *raw_metrics;
  const char *sample_str;
  const char *p;
  int count;
  GHashTable *fonts_seen;
  PangoFont *font;
  PangoLanguage *language;

  language = PANGO_FONTSET_GET_CLASS (fontset)->get_language (fontset);
  sample_str = pango_language_get_sample_string (language);

  count = 0;
  metrics = pango_font_metrics_new ();
  fonts_seen = g_hash_table_new_full (NULL, NULL, g_object_unref, NULL);

  p = sample_str;
  while (*p)
    {
      gunichar wc = g_utf8_get_char (p);
      font = pango_fontset_get_font (fontset, wc);
      if (font)
	{
	  if (g_hash_table_lookup (fonts_seen, font) == NULL)
	    {
	      raw_metrics = pango_font_get_metrics (font, language);
	      g_hash_table_insert (fonts_seen, font, font);
	      
	      if (count == 0)
		{
		  metrics->ascent = raw_metrics->ascent;
		  metrics->descent = raw_metrics->descent;
		  metrics->approximate_char_width = raw_metrics->approximate_char_width;
		  metrics->approximate_digit_width = raw_metrics->approximate_digit_width;
		}
	      else
		{ 
		  metrics->ascent = MAX (metrics->ascent, raw_metrics->ascent);
		  metrics->descent = MAX (metrics->descent, raw_metrics->descent);
		  metrics->approximate_char_width += raw_metrics->approximate_char_width;
		  metrics->approximate_digit_width += raw_metrics->approximate_digit_width;
		}
	      count++;
	      pango_font_metrics_unref (raw_metrics);
	    }
	  else
	    g_object_unref (font);
	}
	  
      p = g_utf8_next_char (p);
    }

  g_hash_table_destroy (fonts_seen);
  
  metrics->approximate_char_width /= count;
  metrics->approximate_digit_width /= count;

  return metrics;
}


/*
 * PangoFontsetSimple
 */

#define PANGO_FONTSET_SIMPLE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FONTSET_SIMPLE, PangoFontsetSimpleClass))
#define PANGO_IS_FONTSET_SIMPLE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_FONTSET_SIMPLE))
#define PANGO_FONTSET_SIMPLE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FONTSET_SIMPLE, PangoFontsetSimpleClass))

static void              pango_fontset_simple_class_init   (PangoFontsetSimpleClass *class);
static void              pango_fontset_simple_finalize     (GObject                 *object);
static void              pango_fontset_simple_init         (PangoFontsetSimple      *fontset);
static PangoFontMetrics *pango_fontset_simple_get_metrics  (PangoFontset            *fontset);
static PangoLanguage *   pango_fontset_simple_get_language (PangoFontset            *fontset);
static  PangoFont *      pango_fontset_simple_get_font     (PangoFontset            *fontset,
							    guint                    wc);

struct _PangoFontsetSimple
{
  PangoFontset parent_instance;

  GPtrArray *fonts;
  GPtrArray *coverages;
  PangoLanguage *language;
};

struct _PangoFontsetSimpleClass
{
  PangoFontsetClass parent_class;
};

static PangoFontsetClass *simple_parent_class;	/* Parent class structure for PangoFontsetSimple */

/**
 * pango_fontset_simple_new:
 * @language: a #PangoLanguage tag
 * 
 * Creates a new #PangoFontsetSimple for the given language.
 *
 * Returns: a newly-allocated #PangoFontsetSimple.
 **/
PangoFontsetSimple *
pango_fontset_simple_new (PangoLanguage *language)
{
  PangoFontsetSimple *fontset;
  
  fontset = g_object_new (PANGO_TYPE_FONTSET_SIMPLE, NULL);
  fontset->language = language;
  
  return fontset;
}

GType
pango_fontset_simple_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoFontsetSimpleClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_fontset_simple_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoFontsetSimple),
        0,              /* n_preallocs */
        (GInstanceInitFunc) pango_fontset_simple_init,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONTSET,
                                            "PangoFontsetSimple",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void
pango_fontset_simple_class_init (PangoFontsetSimpleClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontsetClass *fontset_class = PANGO_FONTSET_CLASS (class);

  simple_parent_class = g_type_class_peek_parent (class);
  
  object_class->finalize = pango_fontset_simple_finalize;
  fontset_class->get_font = pango_fontset_simple_get_font;
  fontset_class->get_metrics = pango_fontset_simple_get_metrics;
  fontset_class->get_language = pango_fontset_simple_get_language;
}

static void 
pango_fontset_simple_init (PangoFontsetSimple *fontset)
{
  fontset->fonts = g_ptr_array_new ();
  fontset->coverages = g_ptr_array_new ();
  fontset->language = NULL;
}

static void
pango_fontset_simple_finalize (GObject *object)
{
  PangoFontsetSimple *fontset = PANGO_FONTSET_SIMPLE (object);
  PangoCoverage *coverage;
  int i;

  for (i = 0; i < fontset->fonts->len; i++)
    g_object_unref (g_ptr_array_index(fontset->fonts, i));

  g_ptr_array_free (fontset->fonts, TRUE);

  for (i = 0; i < fontset->coverages->len; i++)
    {
      coverage = g_ptr_array_index (fontset->coverages, i);
      if (coverage)
	pango_coverage_unref (coverage);
    }
  
  g_ptr_array_free (fontset->coverages, TRUE);

  G_OBJECT_CLASS (simple_parent_class)->finalize (object);
}

/**
 * pango_fontset_simple_append:
 * @fontset: a #PangoFontsetSimple.
 * @font: a #PangoFont.
 *
 * Adds a font to the fontset.
 **/
void
pango_fontset_simple_append (PangoFontsetSimple *fontset,
			     PangoFont          *font)
{
  g_ptr_array_add (fontset->fonts, font);
  g_ptr_array_add (fontset->coverages, NULL);
}

/**
 * pango_fontset_simple_size:
 * @fontset: a #PangoFontsetSimple.
 *
 * Returns the number of fonts in the fontset. 
 *
 * Returns: the size of @fontset.
 **/
int
pango_fontset_simple_size (PangoFontsetSimple *fontset)
{
  return fontset->fonts->len;
}

static PangoLanguage *
pango_fontset_simple_get_language (PangoFontset  *fontset)
{
  PangoFontsetSimple *simple = PANGO_FONTSET_SIMPLE (fontset);

  return simple->language;
}

static PangoFontMetrics *
pango_fontset_simple_get_metrics (PangoFontset  *fontset)
{
  PangoFontsetSimple *simple = PANGO_FONTSET_SIMPLE (fontset);
  
  if (simple->fonts->len == 1)
    return pango_font_get_metrics (PANGO_FONT (g_ptr_array_index(simple->fonts, 0)),
				   simple->language);
  
  return PANGO_FONTSET_CLASS (simple_parent_class)->get_metrics (fontset);
}

static PangoFont *
pango_fontset_simple_get_font (PangoFontset  *fontset,
			       guint          wc)
{
  PangoFontsetSimple *simple = PANGO_FONTSET_SIMPLE (fontset);
  PangoCoverageLevel best_level = PANGO_COVERAGE_NONE;
  PangoCoverageLevel level;
  PangoFont *font;
  PangoCoverage *coverage;
  int result = -1;
  int i;
  
  for (i = 0; i < simple->fonts->len; i++)
    {
      coverage = g_ptr_array_index (simple->coverages, i);
      
      if (coverage == NULL)
	{
	  font = g_ptr_array_index (simple->fonts, i);
	  
	  coverage = pango_font_get_coverage (font, simple->language);
	  g_ptr_array_index (simple->coverages, i) = coverage;
	}
      
      level = pango_coverage_get (coverage, wc);
      
      if (result == -1 || level > best_level)
	{
	  result = i;
	  best_level = level;
	  if (level == PANGO_COVERAGE_EXACT)
	    break;
	}
    }

  font = g_ptr_array_index(simple->fonts, result);
  return g_object_ref (font);
}
