/* Pango
 * gscriptx.c: Routines for handling X fonts
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

#include <string.h>
#include <math.h>

#include <X11/Xlib.h>
#include <fribidi/fribidi.h>
#include <unicode.h>
#include "pangox.h"
#include "pangox-private.h"

#include "utils.h"

#include <config.h>

#define PANGO_TYPE_X_FONT              (pango_x_font_get_type ())
#define PANGO_X_FONT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_X_FONT, PangoXFont))
#define PANGO_X_FONT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_X_FONT, PangoXFontClass))
#define PANGO_X_IS_FONT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_X_FONT))
#define PANGO_X_IS_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_X_FONT))
#define PANGO_X_FONT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_X_FONT, PangoXFontClass))

typedef struct _PangoXFontClass   PangoXFontClass;
typedef struct _PangoXMetricsInfo PangoXMetricsInfo;
typedef struct _PangoXContextInfo PangoXContextInfo;

struct _PangoXSubfontInfo
{
  char *xlfd;
  XFontStruct *font_struct;
  gboolean     is_1byte;
  int          range_byte1;
  int          range_byte2;
};

struct _PangoXMetricsInfo
{
  const char *lang;
  PangoFontMetrics metrics;
};

struct _PangoXContextInfo
{
  PangoGetGCFunc  get_gc_func;
  PangoFreeGCFunc free_gc_func;
};

struct _PangoXFontClass
{
  PangoFontClass parent_class;
};

static PangoFontClass *parent_class;	/* Parent class structure for PangoXFont */

static void pango_x_font_class_init (PangoXFontClass *class);
static void pango_x_font_init       (PangoXFont      *xfont);
static void pango_x_font_shutdown   (GObject         *object);
static void pango_x_font_finalize   (GObject         *object);

static PangoFontDescription *pango_x_font_describe          (PangoFont        *font);
static PangoCoverage *       pango_x_font_get_coverage      (PangoFont        *font,
							     const char       *lang);
static PangoEngineShape *    pango_x_font_find_shaper       (PangoFont        *font,
							     const char       *lang,
							     guint32           ch);
static void                  pango_x_font_get_glyph_extents (PangoFont        *font,
							     PangoGlyph        glyph,
							     PangoRectangle   *ink_rect,
							     PangoRectangle   *logical_rect);
static void                  pango_x_font_get_metrics       (PangoFont        *font,
							     const gchar      *lang,
							     PangoFontMetrics *metrics);

static PangoXSubfontInfo * pango_x_find_subfont    (PangoFont          *font,
						    PangoXSubfont       subfont_index);
static XCharStruct *       pango_x_get_per_char    (PangoFont          *font,
						    PangoXSubfontInfo  *subfont,
						    guint16             char_index);
static gboolean            pango_x_find_glyph      (PangoFont          *font,
						    PangoGlyph          glyph,
						    PangoXSubfontInfo **subfont_return,
						    XCharStruct       **charstruct_return);
static XFontStruct *       pango_x_get_font_struct (PangoFont          *font,
						    PangoXSubfontInfo  *info);

static void     pango_x_get_item_properties (PangoItem      *item,
					     PangoUnderline *uline,
					     PangoAttrColor *fg_color,
					     gboolean       *fg_set,
					     PangoAttrColor *bg_color,
					     gboolean       *bg_set);

static inline PangoXSubfontInfo *
pango_x_find_subfont (PangoFont  *font,
		      PangoXSubfont subfont_index)
{
  PangoXFont *xfont = (PangoXFont *)font;
  
  if (subfont_index < 1 || subfont_index > xfont->n_subfonts)
    {
      g_warning ("Invalid subfont %d", subfont_index);
      return NULL;
    }
  
  return xfont->subfonts[subfont_index-1];
}

static void
pango_x_make_font_struct (PangoFont *font, PangoXSubfontInfo *info)
{
  PangoXFont *xfont = (PangoXFont *)font;
  PangoXFontCache *cache;

  cache = pango_x_font_map_get_font_cache (xfont->fontmap);
  
  info->font_struct = pango_x_font_cache_load (cache, info->xlfd);
  if (!info->font_struct)
    g_warning ("Cannot load font for XLFD '%s\n", info->xlfd);
  
  info->is_1byte = (info->font_struct->min_byte1 == 0 && info->font_struct->max_byte1 == 0);
  info->range_byte1 = info->font_struct->max_byte1 - info->font_struct->min_byte1 + 1;
  info->range_byte2 = info->font_struct->max_char_or_byte2 - info->font_struct->min_char_or_byte2 + 1;
}

static inline XFontStruct *
pango_x_get_font_struct (PangoFont *font, PangoXSubfontInfo *info)
{
  if (!info->font_struct)
    pango_x_make_font_struct (font, info);
  
  return info->font_struct;
}

/**
 * pango_x_get_context:
 * @display: an X display (As returned by XOpenDisplay().)
 * 
 * Retrieves a #PangoContext appropriate for rendering with X fonts on the given display.
  * 
 * Return value: the new #PangoContext
 **/
PangoContext *
pango_x_get_context (Display *display)
{
  PangoContext *result;
  PangoXContextInfo *info;

  g_return_val_if_fail (display != NULL, NULL);

  result = pango_context_new ();
  
  info = g_new (PangoXContextInfo, 1);
  info->get_gc_func = NULL;
  info->free_gc_func = NULL;
  pango_context_set_data (result, "pango-x-info", info, (GDestroyNotify)g_free);
  
  pango_context_add_font_map (result, pango_x_font_map_for_display (display));

  return result;
}

/**
 * pango_x_context_set_funcs:
 * @context: a #PangoContext.
 * @get_gc_func: function called to create a new GC for a given color.
 * @free_gc_func: function called to free a GC created with @get_gc_func.
 * 
 * Sets the functions that will be used to get GC's in various colors when
 * rendering layouts with this context.
 **/
void
pango_x_context_set_funcs  (PangoContext     *context,
			    PangoGetGCFunc    get_gc_func,
			    PangoFreeGCFunc   free_gc_func)
{
  PangoXContextInfo *info;
  
  g_return_if_fail (context != NULL);

  info = pango_context_get_data (context, "pango-x-info");

  info->get_gc_func = get_gc_func;
  info->free_gc_func = free_gc_func;
}

static GType
pango_x_font_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoXFontClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_x_font_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoXFont),
        0,              /* n_preallocs */
        (GInstanceInitFunc) pango_x_font_init,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT,
                                            "PangoXFont",
                                            &object_info);
    }
  
  return object_type;
}

static void 
pango_x_font_init (PangoXFont *xfont)
{
  xfont->subfonts_by_charset = g_hash_table_new (g_str_hash, g_str_equal);
  xfont->subfonts = g_new (PangoXSubfontInfo *, 1);

  xfont->n_subfonts = 0;
  xfont->max_subfonts = 1;

  xfont->metrics_by_lang = NULL;

  xfont->size = -1;
  xfont->entry = NULL;
}

static void
pango_x_font_class_init (PangoXFontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);
  
  object_class->finalize = pango_x_font_finalize;
  object_class->shutdown = pango_x_font_shutdown;
  
  font_class->describe = pango_x_font_describe;
  font_class->get_coverage = pango_x_font_get_coverage;
  font_class->find_shaper = pango_x_font_find_shaper;
  font_class->get_glyph_extents = pango_x_font_get_glyph_extents;
  font_class->get_metrics = pango_x_font_get_metrics;
}

PangoXFont *
pango_x_font_new (PangoFontMap *fontmap, const char *spec, int size)
{
  PangoXFont *result;

  g_return_val_if_fail (fontmap != NULL, NULL);
  g_return_val_if_fail (spec != NULL, NULL);

  result = (PangoXFont *)g_type_create_instance (PANGO_TYPE_X_FONT);
  
  result->fontmap = fontmap;
  g_object_ref (G_OBJECT (fontmap));
  result->display = pango_x_fontmap_get_display (fontmap);

  result->fonts = g_strsplit(spec, ",", -1);
  for (result->n_fonts = 0; result->fonts[result->n_fonts]; result->n_fonts++)
    ; /* Nothing */

  result->size = size;

  return result;
}

/**
 * pango_x_load_font:
 * @display: the X display
 * @spec:    a comma-separated list of XLFD's
 *
 * Loads up a logical font based on a "fontset" style
 * text specification.
 *
 * Returns a new #PangoFont
 */
PangoFont *
pango_x_load_font (Display *display,
		   char    *spec)
{
  PangoXFont *result;

  g_return_val_if_fail (display != NULL, NULL);
  g_return_val_if_fail (spec != NULL, NULL);
  
  result = pango_x_font_new (pango_x_font_map_for_display (display), spec, -1);

  return (PangoFont *)result;
}
 
/**
 * pango_x_render:
 * @display: the X display
 * @d:       the drawable on which to draw string
 * @gc:      the graphics context
 * @font:    the font in which to draw the string
 * @glyphs:  the glyph string to draw
 * @x:       the x position of start of string (in pixels)
 * @y:       the y position of baseline (in pixels)
 *
 * Render a PangoGlyphString onto an X drawable
 */
void 
pango_x_render  (Display           *display,
		 Drawable           d,
		 GC                 gc,
		 PangoFont         *font,
		 PangoGlyphString  *glyphs,
		 int                x, 
		 int                y)
{
  /* Slow initial implementation. For speed, it should really
   * collect the characters into runs, and draw multiple
   * characters with each XDrawString16 call.
   */
  Font old_fid = None;
  XFontStruct *fs;
  int i;
  int x_off = 0;

  g_return_if_fail (display != NULL);
  g_return_if_fail (glyphs != NULL);
  
  for (i=0; i<glyphs->num_glyphs; i++)
    {
      if (glyphs->glyphs[i].glyph)
	{
	  guint16 index = PANGO_X_GLYPH_INDEX (glyphs->glyphs[i].glyph);
	  guint16 subfont_index = PANGO_X_GLYPH_SUBFONT (glyphs->glyphs[i].glyph);
	  PangoXSubfontInfo *subfont;
      
	  XChar2b c;
	  
	  subfont = pango_x_find_subfont (font, subfont_index);
	  if (subfont)
	    {
	      c.byte1 = index / 256;
	      c.byte2 = index % 256;
	      
	      fs = pango_x_get_font_struct (font, subfont);
	      if (!fs)
		continue;
	      
	      if (fs->fid != old_fid)
		{
		  XSetFont (display, gc, fs->fid);
		  old_fid = fs->fid;
		}
	      
	      XDrawString16 (display, d, gc,
			     x + (x_off + glyphs->glyphs[i].geometry.x_offset) / PANGO_SCALE,
			     y + glyphs->glyphs[i].geometry.y_offset / PANGO_SCALE,
			     &c, 1);
	    }
	}

      x_off += glyphs->glyphs[i].geometry.width;
    }
}

static void
pango_x_font_get_glyph_extents  (PangoFont      *font,
				 PangoGlyph      glyph,
				 PangoRectangle *ink_rect,
				 PangoRectangle *logical_rect)
{
  XCharStruct *cs;
  PangoXSubfontInfo *subfont;

  if (glyph && pango_x_find_glyph (font, glyph, &subfont, &cs))
    {
      if (ink_rect)
	{
	  ink_rect->x = PANGO_SCALE * cs->lbearing;
	  ink_rect->width = PANGO_SCALE * (cs->rbearing - cs->lbearing);
	  ink_rect->y = PANGO_SCALE * -cs->ascent;
	  ink_rect->height = PANGO_SCALE * (cs->ascent + cs->descent);
	}
      if (logical_rect)
	{
	  logical_rect->x = 0;
	  logical_rect->width = PANGO_SCALE * cs->width;
	  logical_rect->y = - PANGO_SCALE * subfont->font_struct->ascent;
	  logical_rect->height = PANGO_SCALE * (subfont->font_struct->ascent + subfont->font_struct->descent);
	}
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
      PangoXSubfontInfo *subfont = pango_x_find_subfont (font, GPOINTER_TO_UINT (tmp_list->data));
      
      if (subfont)
	{
	  XFontStruct *fs = pango_x_get_font_struct (font, subfont);
	  if (fs)
	    {
	      if (first)
		{
		  metrics->ascent = fs->ascent * PANGO_SCALE;
		  metrics->descent = fs->descent * PANGO_SCALE;
		  first = FALSE;
		}
	      else
		{
		  metrics->ascent = MAX (fs->ascent * PANGO_SCALE, metrics->ascent);
		  metrics->descent = MAX (fs->descent * PANGO_SCALE, metrics->descent);
		}
	    }
	}
      else
	g_warning ("Invalid subfont %d in get_font_metrics_from_subfonts", GPOINTER_TO_UINT (tmp_list->data));
	  
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
  GUChar4 *text_ucs4;
  int n_chars, i;
  guint8 *embedding_levels;
  FriBidiCharType base_dir = PANGO_DIRECTION_LTR;
  GSList *subfonts = NULL;
  
  n_chars = unicode_strlen (str, -1);

  text_ucs4 = _pango_utf8_to_ucs4 (str, strlen (str));
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
      unicode_char_t wc;
      p = unicode_get_utf8 (p, &wc);

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
	      PangoXSubfont subfont = PANGO_X_GLYPH_SUBFONT (glyph_str->glyphs[j].glyph);
	      if (!g_slist_find (subfonts, GUINT_TO_POINTER ((guint)subfont)))
		subfonts = g_slist_prepend (subfonts, GUINT_TO_POINTER ((guint)subfont));
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
lang_info_compare (const void *key, const void *val)
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
  { "ar", "Arabic  السلام عليكم" },
  { "cs", "Czech (česky)  Dobrý den" },
  { "da", "Danish (Dansk)  Hej, Goddag" },
  { "el", "Greek (Ελληνικά) Γειά σας" },
  { "en", "English Hello" },
  { "eo", "Esperanto Saluton" },
  { "es", "Spanish (Español) ¡Hola!" },
  { "et", "Estonian  Tere, Tervist" },
  { "fi", "Finnish (Suomi)  Hei" },
  { "fr", "French (Français)" },
  { "de", "German Grüß Gott" },
  { "iw", "Hebrew   שלום" },
  { "il", "Italiano  Ciao, Buon giorno" },
  { "ja", "Japanese (日本語) こんにちは, ｺﾝﾆﾁﾊ" },
  { "ko", "Korean (한글)   안녕하세요, 안녕하십니까" },
  { "mt", "Maltese   Ċaw, Saħħa" },
  { "nl", "Nederlands, Vlaams Hallo, Dag" },
  { "no", "Norwegian (Norsk) Hei, God dag" },
  { "pl", "Polish   Dzień dobry, Hej" },
  { "ru", "Russian (Русский)" },
  { "sk", "Slovak   Dobrý deň" },
  { "sv", "Swedish (Svenska) Hej, Goddag" },
  { "tr", "Turkish (Türkçe) Merhaba" },
  { "zh", "Chinese (中文,普通话,汉语)" }
};

static void
pango_x_font_get_metrics (PangoFont        *font,
			  const gchar      *lang,
			  PangoFontMetrics *metrics)
{
  PangoXMetricsInfo *info;
  PangoXFont *xfont = (PangoXFont *)font;
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
	  str = "French (Français)";     /* Assume iso-8859-1 */
	}
    }
  else
    {
      lookup_lang = "NONE";

      /* Complete junk
       */
      str = "السلام عليكم česky Ελληνικά Français 日本語 한글 Русский 中文,普通话,汉语 Türkçe";
    }
  
  tmp_list = xfont->metrics_by_lang;
  while (tmp_list)
    {
      info = tmp_list->data;
      
      if (info->lang == lookup_lang)        /* We _don't_ need strcmp */
	break;

      tmp_list = tmp_list->next;
    }

  if (!tmp_list)
    {
      info = g_new (PangoXMetricsInfo, 1);
      info->lang = lookup_lang;

      xfont->metrics_by_lang = g_slist_prepend (xfont->metrics_by_lang, info);
      
      get_font_metrics_from_string (font, lang, str, &info->metrics);
    }
      
  *metrics = info->metrics;
  return;
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

/* Substitute in a charset into an XLFD. Return the
 * (g_malloc'd) new name, or NULL if the XLFD cannot
 * match the charset
 */
static char *
name_for_charset (char *xlfd, char *charset)
{
  char *p;
  char *dash_charset = g_strconcat ("-", charset, NULL);
  char *result = NULL;
  int ndashes = 0;

  for (p = xlfd; *p; p++)
    if (*p == '-')
      ndashes++;
  
  if (ndashes == 14) /* Complete XLFD */
    {
      if (match_end (xlfd, "-*-*"))
	{
	  result = g_malloc (strlen (xlfd) - 4 + strlen (dash_charset) + 1);
	  strncpy (result, xlfd, strlen (xlfd) - 4);
	  strcpy (result + strlen (xlfd) - 4, dash_charset);
	}
      if (match_end (xlfd, dash_charset))
	result = g_strdup (xlfd);
    }
  else if (ndashes == 13)
    {
      if (match_end (xlfd, "-*"))
	{
	  result = g_malloc (strlen (xlfd) - 2 + strlen (dash_charset) + 1);
	  strncpy (result, xlfd, strlen (xlfd) - 2);
	  strcpy (result + strlen (xlfd) - 2, dash_charset);
	}
      if (match_end (xlfd, dash_charset))
	result = g_strdup (xlfd);
    }
  else
    {
      if (match_end (xlfd, "*"))
	{
	  result = g_malloc (strlen (xlfd) + strlen (dash_charset) + 1);
	  strcpy (result, xlfd);
	  strcpy (result + strlen (xlfd), dash_charset);
	}
      if (match_end (xlfd, dash_charset))
	result = g_strdup (xlfd);
    }

  g_free (dash_charset);
  return result;
}

static PangoXSubfont
pango_x_insert_subfont (PangoFont *font, const char *xlfd)
{
  PangoXFont *xfont = (PangoXFont *)font;
  PangoXSubfontInfo *info;
  
  info = g_new (PangoXSubfontInfo, 1);
  
  info->xlfd = g_strdup (xlfd);
  info->font_struct = NULL;

  xfont->n_subfonts++;
  
  if (xfont->n_subfonts > xfont->max_subfonts)
    {
      xfont->max_subfonts *= 2;
      xfont->subfonts = g_renew (PangoXSubfontInfo *, xfont->subfonts, xfont->max_subfonts);
    }
  
  xfont->subfonts[xfont->n_subfonts - 1] = info;
  
  return xfont->n_subfonts;
}

/**
 * pango_x_list_subfonts:
 * @font: a PangoFont
 * @charsets: the charsets to list subfonts for.
 * @n_charsets: the number of charsets in @charsets
 * @subfont_ids: location to store a pointer to an array of subfont IDs for each found subfont
 *               the result must be freed using g_free()
 * @subfont_charsets: location to store a pointer to an array of subfont IDs for each found subfont
 *               the result must be freed using g_free()
 * 
 * Returns number of charsets found
 **/
int
pango_x_list_subfonts (PangoFont        *font,
		       char            **charsets,
		       int               n_charsets,
		       PangoXSubfont   **subfont_ids,
		       int             **subfont_charsets)
{
  PangoXFont *xfont = (PangoXFont *)font;
  PangoXSubfont **subfont_lists;
  PangoFontMap *fontmap;
  int i, j;
  int n_subfonts = 0;

  g_return_val_if_fail (font != NULL, 0);
  g_return_val_if_fail (n_charsets == 0 || charsets != NULL, 0);

  fontmap = pango_x_font_map_for_display (xfont->display);
  
  subfont_lists = g_new (PangoXSubfont *, n_charsets);

  for (j=0; j<n_charsets; j++)
    {
      subfont_lists[j] = g_hash_table_lookup (xfont->subfonts_by_charset, charsets[j]);
      if (!subfont_lists[j])
	{
	  subfont_lists[j] = g_new (PangoXSubfont, xfont->n_fonts);
	  
	  for (i = 0; i < xfont->n_fonts; i++)
	    {
	      PangoXSubfont subfont = 0;
	      char *xlfd;

	      if (xfont->size == -1)
		{
		  xlfd = name_for_charset (xfont->fonts[i], charsets[j]);
	      
		  if (xlfd)
		    {
		      int count;
		      char **names = XListFonts (xfont->display, xlfd, 1, &count);
		      if (count > 0)
			subfont = pango_x_insert_subfont (font, names[0]);
		      
		      XFreeFontNames (names);
		      g_free (xlfd);
		    }
		}
	      else
		{
		  xlfd = pango_x_make_matching_xlfd (fontmap, xfont->fonts[i], charsets[j], xfont->size);
		  if (xlfd)
		    {
		      subfont = pango_x_insert_subfont (font, xlfd);
		      g_free (xlfd);
		    }
		}
		  
	      subfont_lists[j][i] = subfont;
	    }

	  g_hash_table_insert (xfont->subfonts_by_charset, g_strdup (charsets[j]), subfont_lists[j]);
	}

      for (i = 0; i < xfont->n_fonts; i++)
	if (subfont_lists[j][i])
	  n_subfonts++;
    }

  *subfont_ids = g_new (PangoXSubfont, n_subfonts);
  *subfont_charsets = g_new (int, n_subfonts);

  n_subfonts = 0;

  for (i=0; i<xfont->n_fonts; i++)
    for (j=0; j<n_charsets; j++)
      if (subfont_lists[j][i])
	{
	  (*subfont_ids)[n_subfonts] = subfont_lists[j][i];
	  (*subfont_charsets)[n_subfonts] = j;
	  n_subfonts++;
	}

  g_free (subfont_lists);

  return n_subfonts;
}

/**
 * pango_x_has_glyph:
 * @font: a #PangoFont which must be from the X backend.
 * @glyph: the index of a glyph in the font. (Formed
 *         using the PANGO_X_MAKE_GLYPH macro)
 * 
 * Check if the given glyph is present in a X font.
 * 
 * Return value: %TRUE if the glyph is present.
 **/
gboolean
pango_x_has_glyph (PangoFont  *font,
		   PangoGlyph  glyph)
{
  PangoXSubfontInfo *subfont;
  XCharStruct *cs;

  guint16 char_index = PANGO_X_GLYPH_INDEX (glyph);
  guint16 subfont_index = PANGO_X_GLYPH_SUBFONT (glyph);

  subfont = pango_x_find_subfont (font, subfont_index);
  if (!subfont)
    return FALSE;
  
  cs = pango_x_get_per_char (font, subfont, char_index);

  if (cs && (cs->lbearing != cs->rbearing || cs->width != 0))
    return TRUE;
  else
    return FALSE;
}

/**
 * pango_x_font_subfont_xlfd:
 * @font: a #PangoFont which must be from the X backend
 * @subfont_id: the id of a subfont within the font.
 * 
 * Determine the X Logical Font Description for the specified
 * subfont.
 * 
 * Return value: A newly allocated string containing the XLFD for the
 * subfont. This string must be freed with g_free().
 **/
char *
pango_x_font_subfont_xlfd (PangoFont     *font,
			   PangoXSubfont  subfont_id)
{
  PangoXSubfontInfo *subfont;

  g_return_val_if_fail (font != NULL, NULL);
  g_return_val_if_fail (PANGO_X_IS_FONT (font), NULL);

  subfont = pango_x_find_subfont (font, subfont_id);
  if (!subfont)
    {
      g_warning ("pango_x_font_subfont_xlfd: Invalid subfont_id specified");
      return NULL;
    }

  return g_strdup (subfont->xlfd);
}

static void
pango_x_font_shutdown (GObject *object)
{
  PangoXFont *xfont = PANGO_X_FONT (object);

  /* If the font is not already in the freed-fonts cache, add it,
   * if it is already there, do nothing and the font will be
   * freed.
   */
  if (!xfont->in_cache && xfont->fontmap)
    pango_x_fontmap_cache_add (xfont->fontmap, xfont);

  G_OBJECT_CLASS (parent_class)->shutdown (object);
}


static void
subfonts_foreach (gpointer key, gpointer value, gpointer data)
{
  g_free (key);
  g_free (value);
}

static void
pango_x_font_finalize (GObject *object)
{
  PangoXFont *xfont = (PangoXFont *)object;
  PangoXFontCache *cache = pango_x_font_map_get_font_cache (xfont->fontmap);

  int i;

  for (i=0; i<xfont->n_subfonts; i++)
    {
      PangoXSubfontInfo *info = xfont->subfonts[i];

      g_free (info->xlfd);

      if (info->font_struct)
	pango_x_font_cache_unload (cache, info->font_struct);

      g_free (info);
    }

  g_free (xfont->subfonts);

  g_hash_table_foreach (xfont->subfonts_by_charset, subfonts_foreach, NULL);
  g_hash_table_destroy (xfont->subfonts_by_charset);

  g_slist_foreach (xfont->metrics_by_lang, (GFunc)g_free, NULL);
  g_slist_free (xfont->metrics_by_lang);
  
  if (xfont->entry)
    pango_x_font_entry_remove (xfont->entry, (PangoFont *)xfont);

  g_object_unref (G_OBJECT (xfont->fontmap));

  g_strfreev (xfont->fonts);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static PangoFontDescription *
pango_x_font_describe (PangoFont *font)
{
  /* FIXME: implement */
  return NULL;
}

PangoMap *
pango_x_get_shaper_map (const char *lang)
{
  static guint engine_type_id = 0;
  static guint render_type_id = 0;
  
  if (engine_type_id == 0)
    {
      engine_type_id = g_quark_from_static_string (PANGO_ENGINE_TYPE_SHAPE);
      render_type_id = g_quark_from_static_string (PANGO_RENDER_TYPE_X);
    }
  
  return _pango_find_map (lang, engine_type_id, render_type_id);
}

static PangoCoverage *
pango_x_font_get_coverage (PangoFont  *font,
			   const char *lang)
{
  PangoXFont *xfont = (PangoXFont *)font;

  return pango_x_font_entry_get_coverage (xfont->entry, font, lang);
}

static PangoEngineShape *
pango_x_font_find_shaper (PangoFont   *font,
			  const gchar *lang,
			  guint32      ch)
{
  PangoMap *shape_map = NULL;

  shape_map = pango_x_get_shaper_map (lang);
  return (PangoEngineShape *)_pango_map_get_engine (shape_map, ch);
}

/* Utility functions */

static XCharStruct *
pango_x_get_per_char (PangoFont         *font,
		      PangoXSubfontInfo *subfont,
		      guint16            char_index)
{
  XFontStruct *fs;

  int index;
  int byte1;
  int byte2;

  fs = pango_x_get_font_struct (font, subfont);
  if (!fs)
    return NULL;

  if (subfont->is_1byte)
    {
      index = (int)char_index - fs->min_char_or_byte2;
      if (index < 0 || index >= subfont->range_byte2)
	return NULL;
    }
  else
    {
      byte1 = (int)(char_index / 256) - fs->min_byte1;
      if (byte1 < 0 || byte1 >= subfont->range_byte1)
	return NULL;
	  
      byte2 = (int)(char_index % 256) - fs->min_char_or_byte2;
      if (byte2 < 0 || byte2 >= subfont->range_byte2)
	return NULL;

      index = byte1 * subfont->range_byte2 + byte2;
    }
  
  if (fs->per_char)
    return &fs->per_char[index];
  else
    return &fs->min_bounds;
}

static gboolean
pango_x_find_glyph (PangoFont *font,
		    PangoGlyph glyph,
		    PangoXSubfontInfo **subfont_return,
		    XCharStruct **charstruct_return)
{
  PangoXSubfontInfo *subfont;
  XCharStruct *cs;

  guint16 char_index = PANGO_X_GLYPH_INDEX (glyph);
  guint16 subfont_index = PANGO_X_GLYPH_SUBFONT (glyph);

  subfont = pango_x_find_subfont (font, subfont_index);
  if (!subfont)
    return FALSE;
  
  cs = pango_x_get_per_char (font, subfont, char_index);

  if (cs && (cs->lbearing != cs->rbearing || cs->width != 0))
    {
      if (subfont_return)
	*subfont_return = subfont;

      if (charstruct_return)
	*charstruct_return = cs;
      
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * pango_x_get_unknown_glyph:
 * @font: a #PangoFont
 * 
 * Return the index of a glyph suitable for drawing unknown characters.
 * 
 * Return value: a glyph index into @font
 **/
PangoGlyph
pango_x_get_unknown_glyph (PangoFont *font)
{
  PangoXFont *xfont = (PangoXFont *)font;
  
  /* The strategy here is to find _a_ X font, any X font in the fontset, and
   * then get the unknown glyph for that font.
   */

  g_return_val_if_fail (font != 0, 0);
  g_return_val_if_fail (xfont->n_fonts != 0, 0);

  if (xfont->n_subfonts == 0)
    {
      int count;
      char **names = XListFonts (xfont->display, xfont->fonts[0], 1, &count);

      if (count > 0)
	pango_x_insert_subfont (font, names[0]);

      XFreeFontNames (names);
    }

  if (xfont->n_subfonts > 0)
    {
      XFontStruct *font_struct = pango_x_get_font_struct (font, xfont->subfonts[0]);

      if (font_struct)
	return PANGO_X_MAKE_GLYPH (1,font_struct->default_char);
    }

  return 0;
}

/**
 * pango_x_render_layout_line:
 * @display:   the X display
 * @drawable:  the drawable on which to draw string
 * @gc:        GC to use for uncolored drawing
 * @line:      a #PangoLayoutLine
 * @x:         the x position of start of string (in pixels)
 * @y:         the y position of baseline (in pixels)
 *
 * Render a #PangoLayoutLine onto an X drawable
 */
void 
pango_x_render_layout_line (Display          *display,
			    Drawable          drawable,
			    GC                gc,
			    PangoLayoutLine  *line,
			    int               x, 
			    int               y)
{
  GSList *tmp_list = line->runs;
  PangoRectangle overall_rect;
  PangoRectangle logical_rect;
  PangoRectangle ink_rect;
  PangoContext *context = pango_layout_get_context (line->layout);
  PangoXContextInfo *info = pango_context_get_data (context, "pango-x-info");
  
  int x_off = 0;

  pango_layout_line_get_extents (line,NULL, &overall_rect);
  
  while (tmp_list)
    {
      PangoUnderline uline = PANGO_UNDERLINE_NONE;
      PangoLayoutRun *run = tmp_list->data;
      PangoAttrColor fg_color, bg_color;
      gboolean fg_set, bg_set;
      GC fg_gc;
      
      tmp_list = tmp_list->next;

      pango_x_get_item_properties (run->item, &uline, &fg_color, &fg_set, &bg_color, &bg_set);

      if (fg_set && info->get_gc_func)
	fg_gc = info->get_gc_func (context, &fg_color, gc);
      else
	fg_gc = gc;

      if (uline == PANGO_UNDERLINE_NONE)
	pango_glyph_string_extents (run->glyphs, run->item->analysis.font,
				    NULL, &logical_rect);
      else
	pango_glyph_string_extents (run->glyphs, run->item->analysis.font,
				    &ink_rect, &logical_rect);

      if (bg_set && info->get_gc_func)
	{
	  GC bg_gc = info->get_gc_func (context, &bg_color, gc);

	  XFillRectangle (display, drawable, bg_gc,
			  x + (x_off + logical_rect.x) / PANGO_SCALE,
			  y + overall_rect.y / PANGO_SCALE,
			  logical_rect.width / PANGO_SCALE,
			  overall_rect.height / PANGO_SCALE);

	  if (info->free_gc_func)
	    info->free_gc_func (context, bg_gc);
	}

      pango_x_render (display, drawable, fg_gc, run->item->analysis.font, run->glyphs,
		      x + x_off / PANGO_SCALE, y);

      switch (uline)
	{
	case PANGO_UNDERLINE_NONE:
	  break;
	case PANGO_UNDERLINE_DOUBLE:
	  XDrawLine (display, drawable, fg_gc,
		     x + (x_off + ink_rect.x) / PANGO_SCALE - 1, y + 4,
		     x + (x_off + ink_rect.x + ink_rect.width) / PANGO_SCALE, y + 4);
	  /* Fall through */
	case PANGO_UNDERLINE_SINGLE:
	  XDrawLine (display, drawable, fg_gc,
		     x + (x_off + ink_rect.x) / PANGO_SCALE - 1, y + 2,
		     x + (x_off + ink_rect.x + ink_rect.width) / PANGO_SCALE, y + 2);
	  break;
	case PANGO_UNDERLINE_LOW:
	  XDrawLine (display, drawable, fg_gc,
		     x + (x_off + ink_rect.x) / PANGO_SCALE - 1, y + (ink_rect.y + ink_rect.height) / PANGO_SCALE + 2,
		     x + (x_off + ink_rect.x + ink_rect.width) / PANGO_SCALE, y + (ink_rect.y + ink_rect.height) / PANGO_SCALE + 2);
	  break;
	}

      if (fg_set && info->get_gc_func && info->free_gc_func)
	info->free_gc_func (context, fg_gc);
      
      x_off += logical_rect.width;
    }
}

/**
 * pango_x_render_layout:
 * @display:   the X display
 * @drawable:  the drawable on which to draw string
 * @gc:        GC to use for uncolored drawing
 * @layout:    a #PangoLayout
 * @x:         the X position of the left of the layout (in pixels)
 * @y:         the Y position of the top of the layout (in pixels)
 *
 * Render a #PangoLayoutLine onto an X drawable
 */
void 
pango_x_render_layout (Display         *display,
		       Drawable         drawable,
		       GC               gc,
		       PangoLayout     *layout,
		       int              x, 
		       int              y)
{
  PangoRectangle logical_rect;
  GSList *tmp_list;
  PangoAlignment align;
  int indent;
  int width;
  int y_offset = 0;

  gboolean first = FALSE;
  
  g_return_if_fail (display != NULL);
  g_return_if_fail (layout != NULL);

  indent = pango_layout_get_indent (layout);
  width = pango_layout_get_width (layout);
  align = pango_layout_get_alignment (layout);

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
	  
      pango_x_render_layout_line (display, drawable, gc,
				  line, x + x_offset / PANGO_SCALE, y + (y_offset - logical_rect.y) / PANGO_SCALE);

      y_offset += logical_rect.height;
      tmp_list = tmp_list->next;
    }
}

/* This utility function is duplicated here and in pango-layout.c; should it be
 * public? Trouble is - what is the appropriate set of properties?
 */
static void
pango_x_get_item_properties (PangoItem      *item,
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
