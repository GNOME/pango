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
  int n_els;
  char **xlfds;
  gint **ranges;
  gint *n_ranges;
  Charset **charsets;
  PangoCFont **cfonts;
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
  int i;

  tmp_list = cache->mask_tables;
  while (tmp_list)
    {
      MaskTable *mask_table = tmp_list->data;
      tmp_list = tmp_list->next;

      for (i=0; i<mask_table->n_els; i++)
	{
	  g_free (mask_table->xlfds[i]);
	  g_free (mask_table->ranges[i]);
	  if (mask_table->cfonts[i])
	    pango_cfont_unref (mask_table->cfonts[i]);
	}

      g_free (mask_table->xlfds);
      g_free (mask_table->ranges);
      g_free (mask_table->n_ranges);
      g_free (mask_table->charsets);
      g_free (mask_table->cfonts);
	
      g_free (mask_table);
    }

  g_slist_free (cache->mask_tables);

  g_hash_table_foreach (cache->converters, char_cache_converters_foreach, NULL);
  g_hash_table_destroy (cache->converters); 
  
  g_free (cache);
}

static gboolean
check_character (gint *ranges, gint n_ranges, GUChar4 wc)
{
  int start, end, middle;

  start = 0;
  end = n_ranges - 1;

  if (ranges[2*start] > wc || ranges[2*end + 1] < wc)
    return FALSE;

  while (1)
    {
      middle = (start + end) / 2;
      if (middle == start)
	{
	  if (ranges[2 * middle] > wc || ranges[2 * middle + 1] < wc)
	    return FALSE;
	  else
	    return TRUE;
	}
      else
	{
	  if (ranges[2*middle] <= wc)
	    start = middle;
	  else if (ranges[2*middle + 1] >= wc)
	    end = middle;
	  else
	    return FALSE;
	}
    }
}

/* Compare the tail of a to b */
static gboolean
match_end (char *a, char *b)
{
  size_t len_a = strlen (a);
  size_t len_b = strlen (b);

  if (len_b > len_a)
    return FALSE;
  else
    return (strcmp (a + len_a - len_b, b) == 0);
}

static gboolean
find_char (CharCache *cache, PangoFont *font, GUChar4 wc, char *input,
	   PangoCFont **cfont, PangoGlyphIndex *glyph)
{
  guint mask = find_char_mask (wc);
  GSList *tmp_list;
  MaskTable *mask_table;
  int i, j;

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
      gchar **charset_list;
      gint n_charsets = 0;
      
      mask_table = g_new (MaskTable, 1);

      /* Find the character sets that are included in this mask
       */
      charset_list = g_new (gchar *, MEMBERS(charsets));

      for (i=0; i<MEMBERS(charsets); i++)
	{
	  if (mask & (1 << i))
	    charset_list[n_charsets++] = charsets[i].x_charset;
	}

      /* Find the possible xlfds for the charset list
       */
      mask_table->mask = mask;
      pango_x_list_cfonts (font, charset_list, n_charsets,
			      &mask_table->xlfds,
			      &mask_table->n_els);

      g_free (charset_list);

      mask_table->cfonts = g_new0 (PangoCFont *, mask_table->n_els);

      mask_table->ranges = g_new (gint *, mask_table->n_els);
      mask_table->n_ranges = g_new (gint, mask_table->n_els);
      mask_table->charsets = g_new (Charset *, mask_table->n_els);

      for (i=0; i < mask_table->n_els; i++)
	{
	  pango_x_xlfd_get_ranges (font,
				      mask_table->xlfds[i],
				      &mask_table->ranges[i],
				      &mask_table->n_ranges[i]);

	  mask_table->charsets[i] = NULL;
	  for (j=0; j < MEMBERS(charsets); j++)
	    if (match_end (mask_table->xlfds[i], charsets[j].x_charset))
	      {
		mask_table->charsets[i] = &charsets[j];
		break;
	      }
	}

      cache->mask_tables = g_slist_prepend (cache->mask_tables, mask_table);
    }

  for (i=0; i < mask_table->n_els; i++)
    {
      if (mask_table->charsets[i])
	{
	  PangoGlyphIndex index;

	  index = (*mask_table->charsets[i]->conv_func) (cache, mask_table->charsets[i]->id, input);
	  
	  if (check_character (mask_table->ranges[i], mask_table->n_ranges[i], index))
	    {
	      if (!mask_table->cfonts[i])
		mask_table->cfonts[i] = pango_x_load_xlfd (font, mask_table->xlfds[i]);
	      
	      *cfont = mask_table->cfonts[i];
	      pango_cfont_ref (*cfont);

	      *glyph = index;

	      return TRUE;
	    }
	}
    }

  return FALSE;
}

static void
set_glyph (PangoGlyphString *glyphs, gint i, PangoCFont *cfont, PangoGlyphIndex glyph)
{
  gint width;

  glyphs->glyphs[i].font = cfont;
  glyphs->glyphs[i].glyph = glyph;
  
  glyphs->geometry[i].x_offset = 0;
  glyphs->geometry[i].y_offset = 0;

  glyphs->log_clusters[i] = i;

  pango_x_glyph_extents (&glyphs->glyphs[i],
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
basic_engine_shape (PangoFont     *font,
		    gchar           *text,
		    gint             length,
		    PangoAnalysis *analysis,
		    PangoGlyphString    *glyphs)
{
  int n_chars;
  int i;
  char *p, *next;

  PangoCFont *fallback_font = NULL;
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
      PangoCFont *cfont = NULL;
      PangoGlyphIndex index;

      _pango_utf8_iterate (p, &next, &wc);

      if (analysis->level % 2)
	if (fribidi_get_mirror_char (wc, &mirrored_ch))
	  wc = mirrored_ch;

      if (find_char (cache, font, wc, p, &cfont, &index))
	{
	  set_glyph (glyphs, i, cfont, index);

	  if (i != 0 && glyphs->glyphs[i-1].font == cfont)
	    pango_cfont_unref (cfont);

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
	  if (!fallback_font)
	    fallback_font = pango_x_find_cfont (font, "iso8859-1");
	  
	  if (i == 0 || glyphs->glyphs[i-1].font != fallback_font)
	    pango_cfont_ref (fallback_font);
	  
	  set_glyph (glyphs, i, fallback_font, ' ');
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

  if (fallback_font)
    pango_cfont_unref (fallback_font);
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

