/* Pango
 * basic.c:
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

#include <iconv.h>

#include <glib.h>
#include "pango.h"
#include "pangox.h"
#include "utils.h"
#include <unicode.h>
#include <fribidi/fribidi.h>

typedef struct _CharRange CharRange;
typedef struct _Charset Charset;
typedef struct _CharCache CharCache;
typedef struct _MaskTable MaskTable;

typedef PangoGlyph (*ConvFunc) (CharCache   *cache,
				const char  *id,
				const gchar *input);

struct _Charset
{
  char *id;
  char *x_charset;
  ConvFunc conv_func;
};

struct _CharRange
{
  guint16 start;
  guint16 end;
  guint16 charsets;
};

struct _MaskTable 
{
  guint mask;

  int n_subfonts;

  PangoXSubfont *subfonts;
  Charset **charsets;
};

struct _CharCache 
{
  GSList *mask_tables;
  GHashTable *converters;
};

static PangoGlyph conv_8bit (CharCache *cache,
			     const char *id,
			     const char *input);
static PangoGlyph conv_euc (CharCache  *cache,
			    const char *id,
			    const char *input);
static PangoGlyph conv_ucs4 (CharCache  *cache,
			     const char *id,
			     const char *input);

#include "tables-big.i"

static PangoEngineInfo script_engines[] = {
  {
    "BasicScriptEngineLang",
    PANGO_ENGINE_TYPE_LANG,
    PANGO_RENDER_TYPE_NONE,
    basic_ranges, G_N_ELEMENTS(basic_ranges)
  },
  {
    "BasicScriptEngineX",
    PANGO_ENGINE_TYPE_SHAPE,
    PANGO_RENDER_TYPE_X,
    basic_ranges, G_N_ELEMENTS(basic_ranges)
  }
};

static gint n_script_engines = G_N_ELEMENTS (script_engines);

/*
 * Language script engine
 */

static void 
basic_engine_break (const char     *text,
		    gint            len,
		    PangoAnalysis  *analysis,
		    PangoLogAttr   *attrs)
{
}

static PangoEngine *
basic_engine_lang_new ()
{
  PangoEngineLang *result;
  
  result = g_new (PangoEngineLang, 1);

  result->engine.id = "BasicScriptEngine";
  result->engine.type = PANGO_ENGINE_TYPE_LANG;
  result->engine.length = sizeof (result);
  result->script_break = basic_engine_break;

  return (PangoEngine *)result;
}

/*
 * X window system script engine portion
 */

static guint
find_char_mask (GUChar4 wc)
{
  int start, end, middle;

  start = 0;
  end = G_N_ELEMENTS(ranges) - 1;

  if (ranges[start].start > wc || ranges[end].end < wc)
    return 0;

  while (1)
    {
      middle = (start + end) / 2;
      if (middle == start)
	{
	  if (ranges[middle].start > wc || ranges[middle].end < wc)
	    return ENC_ISO_10646;
	  else
	    return ranges[middle].charsets | ENC_ISO_10646;
	}
      else
	{
	  if (ranges[middle].start <= wc)
	    start = middle;
	  else if (ranges[middle].end >= wc)
	    end = middle;
	  else
	    return ENC_ISO_10646;
	}
    }
}

static CharCache *
char_cache_new (void)
{
  CharCache *result;

  result = g_new (CharCache, 1);
  result->mask_tables = NULL;
  result->converters = g_hash_table_new (g_str_hash, g_str_equal);

  return result;
}

static void
char_cache_converters_foreach (gpointer key, gpointer value, gpointer data)
{
  g_free (key);
  iconv_close ((iconv_t)value);
}

static void
char_cache_free (CharCache *cache)
{
  GSList *tmp_list;

  tmp_list = cache->mask_tables;
  while (tmp_list)
    {
      MaskTable *mask_table = tmp_list->data;
      tmp_list = tmp_list->next;

      g_free (mask_table->subfonts);
      g_free (mask_table->charsets);
      
      g_free (mask_table);
    }

  g_slist_free (cache->mask_tables);

  g_hash_table_foreach (cache->converters, char_cache_converters_foreach, NULL);
  g_hash_table_destroy (cache->converters); 
  
  g_free (cache);
}

PangoGlyph 
find_char (CharCache *cache, PangoFont *font, GUChar4 wc, const char *input)
{
  guint mask = find_char_mask (wc);
  GSList *tmp_list;
  MaskTable *mask_table;
  int i;

  tmp_list = cache->mask_tables;
  while (tmp_list)
    {
      mask_table = tmp_list->data;

      if (mask_table->mask == mask)
	break;

      tmp_list = tmp_list->next;
    }

  if (!tmp_list)
    {
      char *charset_names[G_N_ELEMENTS(charsets)];
      Charset *charsets_map[G_N_ELEMENTS(charsets)];

      int n_charsets = 0;

      int *subfont_charsets;

      mask_table = g_new (MaskTable, 1);

      mask_table->mask = mask;

      /* Find the character sets that are included in this mask
       */

      for (i=0; i<G_N_ELEMENTS(charsets); i++)
	{
	  if (mask & (1 << i))
	    {
	      charset_names[n_charsets] = charsets[i].x_charset;
	      charsets_map[n_charsets] = &charsets[i];
	      
	      n_charsets++;
	    }
	}
      
      mask_table->n_subfonts = pango_x_list_subfonts (font, charset_names, n_charsets, &mask_table->subfonts, &subfont_charsets);

      mask_table->charsets = g_new (Charset *, mask_table->n_subfonts);
      for (i=0; i<mask_table->n_subfonts; i++)
	mask_table->charsets[i] = charsets_map[subfont_charsets[i]];

      g_free (subfont_charsets);

      cache->mask_tables = g_slist_prepend (cache->mask_tables, mask_table);
    }

  for (i=0; i < mask_table->n_subfonts; i++)
    {
      PangoGlyph index;
      PangoGlyph glyph;

      index = (*mask_table->charsets[i]->conv_func) (cache, mask_table->charsets[i]->id, input);

      glyph = PANGO_X_MAKE_GLYPH (mask_table->subfonts[i], index);

      if (pango_x_has_glyph (font, glyph))
	return glyph;	  
    }

  return 0;
}

static void
set_glyph (PangoFont *font, PangoGlyphString *glyphs, gint i, PangoGlyph glyph)
{
  gint width;

  glyphs->glyphs[i].glyph = glyph;
  
  glyphs->glyphs[i].geometry.x_offset = 0;
  glyphs->glyphs[i].geometry.y_offset = 0;

  glyphs->log_clusters[i] = i;

  pango_x_glyph_extents (font, glyphs->glyphs[i].glyph,
			 NULL, NULL, &width, NULL, NULL, NULL, NULL);
  glyphs->glyphs[i].geometry.width = width * 72;
}

static iconv_t
find_converter (CharCache *cache, const char *id)
{
  iconv_t cd = g_hash_table_lookup (cache->converters, id);
  if (!cd)
    {
      cd = iconv_open  (id, "utf8");
      g_hash_table_insert (cache->converters, g_strdup(id), (gpointer)cd);
    }

  return cd;
}

static PangoGlyph
conv_8bit (CharCache  *cache,
	   const char *id,
	   const char *input)
{
  iconv_t cd;
  char outbuf;
  const char *p;
  
  const char *inptr = input;
  int inbytesleft;
  char *outptr = &outbuf;
  int outbytesleft = 1;

  _pango_utf8_iterate (input, &p, NULL);
  inbytesleft = p - input;
  
  cd = find_converter (cache, id);
  g_assert (cd != (iconv_t)-1);

  iconv (cd, (const char **)&inptr, &inbytesleft, &outptr, &outbytesleft);

  return (guchar)outbuf;
}

static PangoGlyph
conv_euc (CharCache  *cache,
	  const char *id,
	  const char *input)
{
  iconv_t cd;
  char outbuf[2];
  const char *p;

  const char *inptr = input;
  int inbytesleft;
  char *outptr = outbuf;
  int outbytesleft = 2;

  _pango_utf8_iterate (input, &p, NULL);
  inbytesleft = p - input;
  
  cd = find_converter (cache, id);
  g_assert (cd != (iconv_t)-1);

  iconv (cd, &inptr, &inbytesleft, &outptr, &outbytesleft);

  if ((guchar)outbuf[0] < 128)
    return outbuf[0];
  else
    return ((guchar)outbuf[0] & 0x7f) * 256 + ((guchar)outbuf[1] & 0x7f);
}

static PangoGlyph
conv_ucs4 (CharCache  *cache,
	   const char *id,
	   const char *input)
{
  GUChar4 wc;
  
  _pango_utf8_iterate (input, NULL, &wc);
  return wc;
}

static void
swap_range (PangoGlyphString *glyphs, int start, int end)
{
  int i, j;
  
  for (i = start, j = end - 1; i < j; i++, j--)
    {
      PangoGlyphInfo glyph_info;
      gint log_cluster;
      
      glyph_info = glyphs->glyphs[i];
      glyphs->glyphs[i] = glyphs->glyphs[j];
      glyphs->glyphs[j] = glyph_info;
      
      log_cluster = glyphs->log_clusters[i];
      glyphs->log_clusters[i] = glyphs->log_clusters[j];
      glyphs->log_clusters[j] = log_cluster;
    }
}

static CharCache *
get_char_cache (PangoFont *font)
{
  CharCache *cache = pango_font_get_data (font, "basic-char-cache");
  if (!cache)
    {
      cache = char_cache_new ();
      pango_font_set_data (font, "basic-char-cache",
			   cache, (GDestroyNotify)char_cache_free);
    }

  return cache;
}

static void 
basic_engine_shape (PangoFont        *font,
		    const char       *text,
		    gint              length,
		    PangoAnalysis    *analysis,
		    PangoGlyphString *glyphs)
{
  int n_chars;
  int i;
  const char *p;
  const char *next;

  PangoXSubfont fallback_subfont = 0;
  CharCache *cache;

  g_return_if_fail (font != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (length >= 0);
  g_return_if_fail (analysis != NULL);

  cache = get_char_cache (font);

  n_chars = unicode_strlen (text, length);
  pango_glyph_string_set_size (glyphs, n_chars);

  p = text;
  for (i=0; i < n_chars; i++)
    {
      GUChar4 wc;
      FriBidiChar mirrored_ch;
      PangoGlyph index;

      _pango_utf8_iterate (p, &next, &wc);

      if (analysis->level % 2)
	if (fribidi_get_mirror_char (wc, &mirrored_ch))
	  wc = mirrored_ch;

      index = find_char (cache, font, wc, p);
      if (index)
	{
	  set_glyph (font, glyphs, i, index);

	  if (unicode_type (wc) == UNICODE_NON_SPACING_MARK)
	    {
	      if (i > 0)
		{
		  glyphs->glyphs[i].geometry.width = MAX (glyphs->glyphs[i-1].geometry.width,
							  glyphs->glyphs[i].geometry.width);
		  glyphs->glyphs[i-1].geometry.width = 0;
		  glyphs->log_clusters[i] = glyphs->log_clusters[i-1];
		}
	    }
	}
      else
	{
	  if (!fallback_subfont)
	    {
	      static char *charset_names[] = { "iso8859-1" };
	      PangoXSubfont *subfonts;
	      int *subfont_charsets;
	      int n_subfonts;
	      
	      n_subfonts = pango_x_list_subfonts (font, charset_names, 1, &subfonts, &subfont_charsets);

	      if (n_subfonts == 0)
		{
		  g_warning ("No fallback character found\n");
		  continue;
		}

	      fallback_subfont = subfonts[0];

	      g_free (subfont_charsets);
	      g_free (subfonts);
	    }
	  
	  set_glyph (font, glyphs, i, PANGO_X_MAKE_GLYPH (fallback_subfont, ' '));
	}
      
      p = next;
    }

  /* Simple bidi support... may have separate modules later */

  if (analysis->level % 2)
    {
      int start, end;

      /* Swap all glyphs */
      swap_range (glyphs, 0, n_chars);
      
      /* Now reorder glyphs within each cluster back to LTR */
      for (start=0; start<n_chars;)
	{
	  end = start;
	  while (end < n_chars &&
		 glyphs->log_clusters[end] == glyphs->log_clusters[start])
	    end++;
	  
	  swap_range (glyphs, start, end);
	  start = end;
	}
    }
}

static PangoCoverage *
basic_engine_get_coverage (PangoFont  *font,
			   const char *lang)
{
  CharCache *cache = get_char_cache (font);
  PangoCoverage *result = pango_coverage_new ();
  GUChar4 wc;

  iconv_t utf8_conv = (iconv_t)-1;

  if (utf8_conv == (iconv_t)-1)
    {
      utf8_conv = iconv_open ("utf-8", "ucs-4");
      if (utf8_conv == (iconv_t)-1)
	g_error ("Could not open coverter from ucs-4 to utf-8!");
    }

  for (wc = 0; wc < 65536; wc++)
    {
      char buf[4];
      const char *inbuf = (const char *)&wc;
      size_t inbytes_left = 4;
      char *outbuf = buf;
      size_t outbytes_left = 4;
      
      iconv (utf8_conv, &inbuf, &inbytes_left, &outbuf, &outbytes_left);

      if (find_char (cache, font, wc, buf))
	pango_coverage_set (result, wc, PANGO_COVERAGE_EXACT);

    }

  return result;
}

static PangoEngine *
basic_engine_x_new ()
{
  PangoEngineShape *result;
  
  result = g_new (PangoEngineShape, 1);

  result->engine.id = "BasicScriptEngine";
  result->engine.type = PANGO_ENGINE_TYPE_LANG;
  result->engine.length = sizeof (result);
  result->script_shape = basic_engine_shape;
  result->get_coverage = basic_engine_get_coverage;

  return (PangoEngine *)result;
}

/* The following three functions provide the public module API for
 * Pango
 */
void 
script_engine_list (PangoEngineInfo **engines, gint *n_engines)
{
  *engines = script_engines;
  *n_engines = n_script_engines;
}

PangoEngine *
script_engine_load (const char *id)
{
  if (!strcmp (id, "BasicScriptEngineLang"))
    return basic_engine_lang_new ();
  else if (!strcmp (id, "BasicScriptEngineX"))
    return basic_engine_x_new ();
  else
    return NULL;
}

void 
script_engine_unload (PangoEngine *engine)
{
}

