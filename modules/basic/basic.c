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

#define MEMBERS(strct) sizeof(strct) / sizeof(strct[1])

typedef struct _CharRange CharRange;
typedef struct _Charset Charset;
typedef struct _CharCache CharCache;
typedef struct _MaskTable MaskTable;

typedef PangoGlyphIndex (*ConvFunc) (CharCache *cache,
				     gchar     *id,
				     gchar     *input);

struct _Charset
{
  gchar *id;
  gchar *x_charset;
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
  PangoXCharset *charset_ids;
  Charset **charsets;

  int n_charsets;
};

struct _CharCache 
{
  GSList *mask_tables;
  GHashTable *converters;
};

static PangoGlyphIndex conv_8bit (CharCache *cache,
				  gchar  *id,
				  char   *input);
static PangoGlyphIndex conv_euc (CharCache *cache,
				 gchar     *id,
				 char      *input);
static PangoGlyphIndex conv_ucs4 (CharCache *cache,
				  gchar     *id,
				  char      *input);

#include "tables-big.i"

static PangoEngineInfo script_engines[] = {
  {
    "BasicScriptEngineLang",
    PANGO_ENGINE_TYPE_LANG,
    PANGO_RENDER_TYPE_NONE,
    basic_ranges, MEMBERS(basic_ranges)
  },
  {
    "BasicScriptEngineX",
    PANGO_ENGINE_TYPE_SHAPE,
    PANGO_RENDER_TYPE_X,
    basic_ranges, MEMBERS(basic_ranges)
  }
};

static gint n_script_engines = MEMBERS (script_engines);

/*
 * Language script engine
 */

static void 
basic_engine_break (gchar            *text,
		    gint             len,
		    PangoAnalysis *analysis,
		    PangoLogAttr  *attrs)
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
  end = MEMBERS(ranges) - 1;

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

      g_free (mask_table->charset_ids);
      g_free (mask_table->charsets);
      
      g_free (mask_table);
    }

  g_slist_free (cache->mask_tables);

  g_hash_table_foreach (cache->converters, char_cache_converters_foreach, NULL);
  g_hash_table_destroy (cache->converters); 
  
  g_free (cache);
}

PangoGlyphIndex 
find_char (CharCache *cache, PangoFont *font, GUChar4 wc, char *input)
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
      PangoXCharset tmp_charset_ids[MEMBERS(charsets)];
      Charset *tmp_charsets[MEMBERS(charsets)];

      mask_table = g_new (MaskTable, 1);

      mask_table->mask = mask;
      mask_table->n_charsets = 0;

      /* Find the character sets that are included in this mask
       */

      for (i=0; i<MEMBERS(charsets); i++)
	{
	  if (mask & (1 << i))
	    {
	      PangoXCharset charset_id = pango_x_find_charset (font, charsets[i].x_charset);
	      if (charset_id)
		{
		  tmp_charset_ids[mask_table->n_charsets] = charset_id;
		  tmp_charsets[mask_table->n_charsets] = &charsets[i];
		  mask_table->n_charsets++;
		}
	    }
	}
      
      mask_table->charset_ids = g_new (PangoXCharset, mask_table->n_charsets);
      mask_table->charsets = g_new (Charset *, mask_table->n_charsets);
      
      memcpy (mask_table->charset_ids, tmp_charset_ids, sizeof(PangoXCharset) * mask_table->n_charsets);
      memcpy (mask_table->charsets, tmp_charsets, sizeof(Charset *) * mask_table->n_charsets);

      cache->mask_tables = g_slist_prepend (cache->mask_tables, mask_table);
    }

  for (i=0; i < mask_table->n_charsets; i++)
    {
      PangoGlyphIndex index;
      PangoGlyphIndex glyph;

      index = (*mask_table->charsets[i]->conv_func) (cache, mask_table->charsets[i]->id, input);

      glyph = PANGO_X_MAKE_GLYPH (mask_table->charset_ids[i], index);

      if (pango_x_has_glyph (font, glyph))
	return glyph;	  
    }

  return 0;
}

static void
set_glyph (PangoFont *font, PangoGlyphString *glyphs, gint i, PangoGlyphIndex glyph)
{
  gint width;

  glyphs->glyphs[i].glyph = glyph;
  
  glyphs->geometry[i].x_offset = 0;
  glyphs->geometry[i].y_offset = 0;

  glyphs->log_clusters[i] = i;

  pango_x_glyph_extents (font, glyphs->glyphs[i].glyph,
			 NULL, NULL, &width, NULL, NULL, NULL, NULL);
  glyphs->geometry[i].width = width * 72;
}

static iconv_t
find_converter (CharCache *cache, gchar *id)
{
  iconv_t cd = g_hash_table_lookup (cache->converters, id);
  if (!cd)
    {
      cd = iconv_open  (id, "utf8");
      g_hash_table_insert (cache->converters, g_strdup(id), (gpointer)cd);
    }

  return cd;
}

static PangoGlyphIndex
conv_8bit (CharCache *cache,
	   gchar     *id,
	   char      *input)
{
  iconv_t cd;
  char outbuf;
  char *p;
  
  char *inptr = input;
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

static PangoGlyphIndex
conv_euc (CharCache *cache,
	  gchar     *id,
	  char      *input)
{
  iconv_t cd;
  char outbuf[2];
  char *p;

  char *inptr = input;
  int inbytesleft;
  char *outptr = outbuf;
  int outbytesleft = 2;

  _pango_utf8_iterate (input, &p, NULL);
  inbytesleft = p - input;
  
  cd = find_converter (cache, id);
  g_assert (cd != (iconv_t)-1);

  iconv (cd, (const char **)&inptr, &inbytesleft, &outptr, &outbytesleft);

  if ((guchar)outbuf[0] < 128)
    return outbuf[0];
  else
    return ((guchar)outbuf[0] & 0x7f) * 256 + ((guchar)outbuf[1] & 0x7f);
}

static PangoGlyphIndex
conv_ucs4 (CharCache *cache,
	   gchar     *id,
	   char      *input)
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
      PangoGlyph glyph;
      PangoGlyphGeometry geometry;
      PangoGlyphVisAttr attr;
      gint log_cluster;
      
      glyph = glyphs->glyphs[i];
      glyphs->glyphs[i] = glyphs->glyphs[j];
      glyphs->glyphs[j] = glyph;
      
      geometry = glyphs->geometry[i];
      glyphs->geometry[i] = glyphs->geometry[j];
      glyphs->geometry[j] = geometry;
      
      attr = glyphs->attrs[i];
      glyphs->attrs[i] = glyphs->attrs[j];
      glyphs->attrs[j] = attr;
      
      log_cluster = glyphs->log_clusters[i];
      glyphs->log_clusters[i] = glyphs->log_clusters[j];
      glyphs->log_clusters[j] = log_cluster;
    }
}

static void 
basic_engine_shape (PangoFont        *font,
		    gchar            *text,
		    gint              length,
		    PangoAnalysis    *analysis,
		    PangoGlyphString *glyphs)
{
  int n_chars;
  int i;
  char *p, *next;

  PangoXCharset fallback_charset = 0;
  CharCache *cache;

  g_return_if_fail (font != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (length >= 0);
  g_return_if_fail (analysis != NULL);

  cache = pango_font_get_data (font, "basic-char-cache");
  if (!cache)
    {
      cache = char_cache_new ();
      pango_font_set_data (font, "basic-char-cache",
			   cache, (GDestroyNotify)char_cache_free);
    }
  
  n_chars = unicode_strlen (text, length);
  pango_glyph_string_set_size (glyphs, n_chars);

  p = text;
  for (i=0; i < n_chars; i++)
    {
      GUChar4 wc;
      FriBidiChar mirrored_ch;
      PangoGlyphIndex index;

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
		  glyphs->geometry[i].width = MAX (glyphs->geometry[i-1].width,
						   glyphs->geometry[i].width);
		  glyphs->geometry[i-1].width = 0;
		  glyphs->log_clusters[i] = glyphs->log_clusters[i-1];
		}
	    }
	}
      else
	{
	  if (!fallback_charset)
	    fallback_charset = pango_x_find_charset (font, "iso8859-1");
	  
	  set_glyph (font, glyphs, i, PANGO_X_MAKE_GLYPH (fallback_charset, ' '));
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

static PangoEngine *
basic_engine_x_new ()
{
  PangoEngineShape *result;
  
  result = g_new (PangoEngineShape, 1);

  result->engine.id = "BasicScriptEngine";
  result->engine.type = PANGO_ENGINE_TYPE_LANG;
  result->engine.length = sizeof (result);
  result->script_shape = basic_engine_shape;

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

