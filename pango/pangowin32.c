/* Pango
 * pangowin32.c: Routines for handling Windows fonts
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
#include <glib.h>

#include <fribidi/fribidi.h>

#include "pangowin32.h"
#include "pangowin32-private.h"

#define PANGO_TYPE_WIN32_FONT            (pango_win32_font_get_type ())
#define PANGO_WIN32_FONT(object)         (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_WIN32_FONT, PangoWin32Font))
#define PANGO_WIN32_FONT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_WIN32_FONT, PangoWin32FontClass))
#define PANGO_WIN32_IS_FONT(object)      (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_WIN32_FONT))
#define PANGO_WIN32_IS_FONT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_WIN32_FONT))
#define PANGO_WIN32_FONT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_WIN32_FONT, PangoWin32FontClass))

HDC pango_win32_hdc;

static HBRUSH white_brush;

typedef struct _PangoWin32FontClass   PangoWin32FontClass;
typedef struct _PangoWin32MetricsInfo PangoWin32MetricsInfo;

struct _PangoWin32SubfontInfo
{
  LOGFONT logfont;
  HFONT hfont;

  /* The following fields are used only to check whether a glyph is
   * present in the subfont. On NT, there is the API GetGlyphIndices
   * that can be used to do this very simply. But on Win9x,
   * this isn't available. Bummer. As fas as I can think, the only
   * way to determine if a glyph is present in a HFONT is to
   * actually draw the glyph and compare the result to that from
   * the font's "default character". Well, one other way would be
   * to parse the font's character map via GetFontData, but that does
   * also seem a bit complex, and might not work with new font
   * technologies.
   */
  HBITMAP buf_hbm;
  HDC buf_hdc;
  RECT buf_rect;
  int buf_x, buf_y;
  HBITMAP default_char_hbm;
  char *buf, *default_char_buf;
  int buf_size;
  HFONT oldfont;
  HBITMAP oldbm;
};

struct _PangoWin32MetricsInfo
{
  const char *lang;
  PangoFontMetrics metrics;
};

struct _PangoWin32FontClass
{
  PangoFontClass parent_class;
};

static PangoFontClass *parent_class;	/* Parent class structure for PangoWin32Font */

static void pango_win32_font_class_init (PangoWin32FontClass *class);
static void pango_win32_font_init       (PangoWin32Font      *win32font);
static void pango_win32_font_shutdown   (GObject             *object);
static void pango_win32_font_finalize   (GObject             *object);

static PangoFontDescription *pango_win32_font_describe      (PangoFont        *font);
static PangoCoverage        *pango_win32_font_get_coverage  (PangoFont        *font,
							     const char       *lang);
static PangoEngineShape     *pango_win32_font_find_shaper   (PangoFont        *font,
							     const char       *lang,
							     guint32           ch);
static void pango_win32_font_get_glyph_extents (PangoFont        *font,
						PangoGlyph        glyph,
						PangoRectangle   *ink_rect,
						PangoRectangle   *logical_rect);
static void pango_win32_font_get_metrics       (PangoFont        *font,
						const gchar      *lang,
						PangoFontMetrics *metrics);

static PangoWin32SubfontInfo *pango_win32_find_subfont (PangoFont          *font,
							PangoWin32Subfont   subfont_index);

static gboolean pango_win32_find_glyph (PangoFont              	 *font,
					PangoGlyph             	  glyph,
					PangoWin32SubfontInfo   **subfont_return,
					SIZE                   	 *size_return);
static HFONT pango_win32_get_hfont     (PangoFont              	 *font,
				        PangoWin32SubfontInfo  	 *info);

static void pango_win32_get_item_properties (PangoItem      	 *item,
					     PangoUnderline 	 *uline,
					     PangoAttrColor 	 *fg_color,
					     gboolean       	 *fg_set,
					     PangoAttrColor 	 *bg_color,
					     gboolean       	 *bg_set);

static inline PangoWin32SubfontInfo *
pango_win32_find_subfont (PangoFont        *font,
			  PangoWin32Subfont subfont_index)
{
  PangoWin32Font *win32font = (PangoWin32Font *)font;
  
  if (subfont_index < 1 || subfont_index > win32font->n_subfonts)
    {
      g_warning ("Invalid subfont %d", subfont_index);
      return NULL;
    }
  
  return win32font->subfonts[subfont_index-1];
}

static inline HFONT
pango_win32_get_hfont (PangoFont             *font,
		       PangoWin32SubfontInfo *info)
{
  PangoWin32Font *win32font = (PangoWin32Font *)font;
  PangoWin32FontCache *cache;

  if (!info->hfont)
    {
      cache = pango_win32_font_map_get_font_cache (win32font->fontmap);
  
      info->hfont = pango_win32_font_cache_load (cache, &info->logfont);
      if (!info->hfont)
	g_warning ("Cannot load font '%s\n", info->logfont.lfFaceName);
    }
  
  return info->hfont;
}

/**
 * pango_win32_get_context:
 * 
 * Retrieves a #PangoContext appropriate for rendering with Windows fonts.
  * 
 * Return value: the new #PangoContext
 **/
PangoContext *
pango_win32_get_context (void)
{
  PangoContext *result;

  result = pango_context_new ();
  pango_context_add_font_map (result, pango_win32_font_map_for_display ());

  return result;
}

static GType
pango_win32_font_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (PangoWin32FontClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) pango_win32_font_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (PangoWin32Font),
        0,              /* n_preallocs */
        (GInstanceInitFunc) pango_win32_font_init,
      };
      
      object_type = g_type_register_static (PANGO_TYPE_FONT,
                                            "PangoWin32Font",
                                            &object_info);
    }
  
  return object_type;
}

static void 
pango_win32_font_init (PangoWin32Font *win32font)
{
  win32font->subfonts_by_subrange = g_hash_table_new (g_direct_hash, g_direct_equal);
  win32font->subfonts = g_new (PangoWin32SubfontInfo *, 1);

  win32font->n_subfonts = 0;
  win32font->max_subfonts = 1;

  win32font->metrics_by_lang = NULL;

  win32font->size = -1;
  win32font->entry = NULL;
}

static void
pango_win32_font_class_init (PangoWin32FontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);
  
  object_class->finalize = pango_win32_font_finalize;
  object_class->shutdown = pango_win32_font_shutdown;
  
  font_class->describe = pango_win32_font_describe;
  font_class->get_coverage = pango_win32_font_get_coverage;
  font_class->find_shaper = pango_win32_font_find_shaper;
  font_class->get_glyph_extents = pango_win32_font_get_glyph_extents;
  font_class->get_metrics = pango_win32_font_get_metrics;

  if (pango_win32_hdc == NULL)
    {
      pango_win32_hdc = CreateDC ("DISPLAY", NULL, NULL, NULL);
      white_brush = GetStockObject (WHITE_BRUSH);
    }
}

PangoWin32Font *
pango_win32_font_new (PangoFontMap  *fontmap,
		      const LOGFONT *lfp,
		      int            n_logfonts,
		      int	     size)
{
  PangoWin32Font *result;

  g_return_val_if_fail (fontmap != NULL, NULL);
  g_return_val_if_fail (lfp != NULL, NULL);

  result = (PangoWin32Font *)g_type_create_instance (PANGO_TYPE_WIN32_FONT);
  
  result->fontmap = fontmap;
  g_object_ref (G_OBJECT (fontmap));

  result->n_logfonts = n_logfonts;
  result->logfonts = g_new (LOGFONT, n_logfonts);
  memcpy (result->logfonts, lfp, sizeof (LOGFONT) * n_logfonts);
  result->size = size;

  return result;
}

/**
 * pango_win32_load_font:
 * @lfps:    an array of LOGFONTs
 * @n_fonts: the number of LOGFONTS
 *
 * Loads a logical font based on a "fontset" style specification.
 *
 * Returns a new #PangoFont
 */
PangoFont *
pango_win32_load_font (LOGFONT *lfp,
		       int      n_fonts)
{
  PangoWin32Font *result;

  g_return_val_if_fail (lfp != NULL, NULL);
  
  result = pango_win32_font_new (pango_win32_font_map_for_display (),
				 lfp, n_fonts, -1);

  return (PangoFont *)result;
}
 
/**
 * pango_win32_render:
 * @hdc:     the device context
 * @font:    the font in which to draw the string
 * @glyphs:  the glyph string to draw
 * @x:       the x position of start of string (in pixels)
 * @y:       the y position of baseline (in pixels)
 *
 * Render a PangoGlyphString onto a Windows DC
 */
void 
pango_win32_render (HDC                hdc,
		    PangoFont         *font,
		    PangoGlyphString  *glyphs,
		    int                x, 
		    int                y)
{
  /* Slow initial implementation. For speed, it should really
   * collect the characters into runs, and draw multiple
   * characters with each TextOutW.
   */
  HFONT old_hfont = NULL;
  HFONT orig_hfont = NULL;
  HFONT hfont;
  int i;
  int x_off = 0;

  g_return_if_fail (glyphs != NULL);
  
  for (i = 0; i <glyphs->num_glyphs; i++)
    {
      if (glyphs->glyphs[i].glyph)
	{
	  guint16 index = PANGO_WIN32_GLYPH_INDEX (glyphs->glyphs[i].glyph);
	  guint16 subfont_index = PANGO_WIN32_GLYPH_SUBFONT (glyphs->glyphs[i].glyph);
	  PangoWin32SubfontInfo *info;
      
	  info = pango_win32_find_subfont (font, subfont_index);
	  if (info)
	    {
	      hfont = pango_win32_get_hfont (font, info);
	      if (!hfont)
		continue;
	      
	      if (hfont != old_hfont)
		{
		  if (orig_hfont == NULL)
		    orig_hfont = SelectObject (hdc, hfont);
		  else
		    SelectObject (hdc, hfont);
		  old_hfont = hfont;
		}
	      
	      TextOutW (hdc,
			x + (x_off + glyphs->glyphs[i].geometry.x_offset) / PANGO_SCALE,
			y + glyphs->glyphs[i].geometry.y_offset / PANGO_SCALE,
			&index, 1);
	    }
	}

      x_off += glyphs->glyphs[i].geometry.width;
    }
  if (orig_hfont != NULL)
    SelectObject (hdc, orig_hfont);
}

/* This table classifies Unicode characters according to the Microsoft
 * Unicode subset numbering. This is based on the table in "Developing
 * International Software for Windows 95 and Windows NT". This is almost,
 * but not quite, the same as the official Unicode block table in
 * Blocks.txt from ftp.unicode.org. The bit number field is the bitfield
 * number as in the FONTSIGNATURE struct's fsUsb field.
 * There are some grave bugs in the table in the books. For instance
 * it claims there are Hangul at U+3400..U+4DFF while this range in
 * fact contains CJK Unified Ideographs Extension A. Also, the whole
 * block of Hangul Syllables U+AC00..U+D7A3 is missing from the book.
 */

static struct {
  wchar_t low, high;
  PangoWin32UnicodeSubrange bit; 
  gchar *name;
} utab[] =
{
  { 0x0000, 0x007E,
    PANGO_WIN32_U_BASIC_LATIN, "Basic Latin" },
  { 0x00A0, 0x00FF,
    PANGO_WIN32_U_LATIN_1_SUPPLEMENT, "Latin-1 Supplement" },
  { 0x0100, 0x017F,
    PANGO_WIN32_U_LATIN_EXTENDED_A, "Latin Extended-A" },
  { 0x0180, 0x024F,
    PANGO_WIN32_U_LATIN_EXTENDED_B, "Latin Extended-B" },
  { 0x0250, 0x02AF,
    PANGO_WIN32_U_IPA_EXTENSIONS, "IPA Extensions" },
  { 0x02B0, 0x02FF,
    PANGO_WIN32_U_SPACING_MODIFIER_LETTERS, "Spacing Modifier Letters" },
  { 0x0300, 0x036F,
    PANGO_WIN32_U_COMBINING_DIACRITICAL_MARKS, "Combining Diacritical Marks" },
  { 0x0370, 0x03CF,
    PANGO_WIN32_U_BASIC_GREEK, "Basic Greek" },
  { 0x03D0, 0x03FF,
    PANGO_WIN32_U_GREEK_SYMBOLS_AND_COPTIC, "Greek Symbols and Coptic" },
  { 0x0400, 0x04FF,
    PANGO_WIN32_U_CYRILLIC, "Cyrillic" },
  { 0x0530, 0x058F,
    PANGO_WIN32_U_ARMENIAN, "Armenian" },
  { 0x0590, 0x05CF,
    PANGO_WIN32_U_HEBREW_EXTENDED, "Hebrew Extended" },
  { 0x05D0, 0x05FF,
    PANGO_WIN32_U_BASIC_HEBREW, "Basic Hebrew" },
  { 0x0600, 0x0652,
    PANGO_WIN32_U_BASIC_ARABIC, "Basic Arabic" },
  { 0x0653, 0x06FF,
    PANGO_WIN32_U_ARABIC_EXTENDED, "Arabic Extended" },
  { 0x0900, 0x097F,
    PANGO_WIN32_U_DEVANAGARI, "Devanagari" },
  { 0x0980, 0x09FF,
    PANGO_WIN32_U_BENGALI, "Bengali" },
  { 0x0A00, 0x0A7F,
    PANGO_WIN32_U_GURMUKHI, "Gurmukhi" },
  { 0x0A80, 0x0AFF,
    PANGO_WIN32_U_GUJARATI, "Gujarati" },
  { 0x0B00, 0x0B7F,
    PANGO_WIN32_U_ORIYA, "Oriya" },
  { 0x0B80, 0x0BFF,
    PANGO_WIN32_U_TAMIL, "Tamil" },
  { 0x0C00, 0x0C7F,
    PANGO_WIN32_U_TELUGU, "Telugu" },
  { 0x0C80, 0x0CFF,
    PANGO_WIN32_U_KANNADA, "Kannada" },
  { 0x0D00, 0x0D7F,
    PANGO_WIN32_U_MALAYALAM, "Malayalam" },
  { 0x0E00, 0x0E7F,
    PANGO_WIN32_U_THAI, "Thai" },
  { 0x0E80, 0x0EFF,
    PANGO_WIN32_U_LAO, "Lao" },
  { 0x10A0, 0x10CF,
    PANGO_WIN32_U_GEORGIAN_EXTENDED, "Georgian Extended" },
  { 0x10D0, 0x10FF,
    PANGO_WIN32_U_BASIC_GEORGIAN, "Basic Georgian" },
  { 0x1100, 0x11FF,
    PANGO_WIN32_U_HANGUL_JAMO, "Hangul Jamo" },
  { 0x1E00, 0x1EFF,
    PANGO_WIN32_U_LATIN_EXTENDED_ADDITIONAL, "Latin Extended Additional" },
  { 0x1F00, 0x1FFF,
    PANGO_WIN32_U_GREEK_EXTENDED, "Greek Extended" },
  { 0x2000, 0x206F,
    PANGO_WIN32_U_GENERAL_PUNCTUATION, "General Punctuation" },
  { 0x2070, 0x209F,
    PANGO_WIN32_U_SUPERSCRIPTS_AND_SUBSCRIPTS, "Superscripts and Subscripts" },
  { 0x20A0, 0x20CF,
    PANGO_WIN32_U_CURRENCY_SYMBOLS, "Currency Symbols" },
  { 0x20D0, 0x20FF,
    PANGO_WIN32_U_COMBINING_DIACRITICAL_MARKS_FOR_SYMBOLS, "Combining Diacritical Marks for Symbols" },
  { 0x2100, 0x214F,
    PANGO_WIN32_U_LETTERLIKE_SYMBOLS, "Letterlike Symbols" },
  { 0x2150, 0x218F,
    PANGO_WIN32_U_NUMBER_FORMS, "Number Forms" },
  { 0x2190, 0x21FF,
    PANGO_WIN32_U_ARROWS, "Arrows" },
  { 0x2200, 0x22FF,
    PANGO_WIN32_U_MATHEMATICAL_OPERATORS, "Mathematical Operators" },
  { 0x2300, 0x23FF,
    PANGO_WIN32_U_MISCELLANEOUS_TECHNICAL, "Miscellaneous Technical" },
  { 0x2400, 0x243F,
    PANGO_WIN32_U_CONTROL_PICTURES, "Control Pictures" },
  { 0x2440, 0x245F,
    PANGO_WIN32_U_OPTICAL_CHARACTER_RECOGNITION, "Optical Character Recognition" },
  { 0x2460, 0x24FF,
    PANGO_WIN32_U_ENCLOSED_ALPHANUMERICS, "Enclosed Alphanumerics" },
  { 0x2500, 0x257F,
    PANGO_WIN32_U_BOX_DRAWING, "Box Drawing" },
  { 0x2580, 0x259F,
    PANGO_WIN32_U_BLOCK_ELEMENTS, "Block Elements" },
  { 0x25A0, 0x25FF,
    PANGO_WIN32_U_GEOMETRIC_SHAPES, "Geometric Shapes" },
  { 0x2600, 0x26FF,
    PANGO_WIN32_U_MISCELLANEOUS_SYMBOLS, "Miscellaneous Symbols" },
  { 0x2700, 0x27BF,
    PANGO_WIN32_U_DINGBATS, "Dingbats" },
  { 0x3000, 0x303F,
    PANGO_WIN32_U_CJK_SYMBOLS_AND_PUNCTUATION, "CJK Symbols and Punctuation" },
  { 0x3040, 0x309F,
    PANGO_WIN32_U_HIRAGANA, "Hiragana" },
  { 0x30A0, 0x30FF,
    PANGO_WIN32_U_KATAKANA, "Katakana" },
  { 0x3100, 0x312F,
    PANGO_WIN32_U_BOPOMOFO, "Bopomofo" },
  { 0x3130, 0x318F,
    PANGO_WIN32_U_HANGUL_COMPATIBILITY_JAMO, "Hangul Compatibility Jamo" },
  { 0x3190, 0x319F,
    PANGO_WIN32_U_CJK_MISCELLANEOUS, "CJK Miscellaneous" },
  { 0x3200, 0x32FF,
    PANGO_WIN32_U_ENCLOSED_CJK, "Enclosed CJK" },
  { 0x3300, 0x33FF,
    PANGO_WIN32_U_CJK_COMPATIBILITY, "CJK Compatibility" },
  /* The book claims:
   * U+3400..U+3D2D Hangul
   * U+3D2E..U+44B7 Hangul Supplementary A
   * U+44B8..U+4DFF Hangul Supplementary B
   * but actually in Unicode
   * U+3400..U+4DB5 is CJK Unified Ideographs Extension A
   */
  { 0x3400, 0x4DB5,
    PANGO_WIN32_U_CJK_UNIFIED_IDEOGRAPHS, "CJK Unified Ideographs Extension A" },
  { 0x4E00, 0x9FFF,
    PANGO_WIN32_U_CJK_UNIFIED_IDEOGRAPHS, "CJK Unified Ideographs" },
  /* This was missing completely from the book's table. */
  { 0xAC00, 0xD7A3,
    PANGO_WIN32_U_HANGUL, "Hangul Syllables" },
  { 0xE000, 0xF8FF,
    PANGO_WIN32_U_PRIVATE_USE_AREA, "Private Use Area" },
  { 0xF900, 0xFAFF,
    PANGO_WIN32_U_CJK_COMPATIBILITY_IDEOGRAPHS, "CJK Compatibility Ideographs" },
  { 0xFB00, 0xFB4F,
    PANGO_WIN32_U_ALPHABETIC_PRESENTATION_FORMS, "Alphabetic Presentation Forms" },
  { 0xFB50, 0xFDFF,
    PANGO_WIN32_U_ARABIC_PRESENTATION_FORMS_A, "Arabic Presentation Forms-A" },
  { 0xFE20, 0xFE2F,
    PANGO_WIN32_U_COMBINING_HALF_MARKS, "Combining Half Marks" },
  { 0xFE30, 0xFE4F,
    PANGO_WIN32_U_CJK_COMPATIBILITY_FORMS, "CJK Compatibility Forms" },
  { 0xFE50, 0xFE6F,
    PANGO_WIN32_U_SMALL_FORM_VARIANTS, "Small Form Variants" },
  { 0xFE70, 0xFEFE,
    PANGO_WIN32_U_ARABIC_PRESENTATION_FORMS_B, "Arabic Presentation Forms-B" },
  { 0xFEFF, 0xFEFF,
    PANGO_WIN32_U_SPECIALS, "Specials" },
  { 0xFF00, 0xFFEF,
    PANGO_WIN32_U_HALFWIDTH_AND_FULLWIDTH_FORMS, "Halfwidth and Fullwidth Forms" },
  { 0xFFF0, 0xFFFD,
    PANGO_WIN32_U_SPECIALS, "Specials" }
};

/* Return the Unicode subrange number for a Unicode character */
PangoWin32UnicodeSubrange
pango_win32_unicode_classify (wchar_t wc)
{
  int min = 0;
  int max = G_N_ELEMENTS (utab) - 1;
  int mid;

  while (max >= min)
    {
      mid = (min + max) / 2;
      if (utab[mid].high < wc)
	min = mid + 1;
      else if (wc < utab[mid].low)
	max = mid - 1;
      else if (utab[mid].low <= wc && wc <= utab[mid].high)
	return utab[mid].bit;
      else
	return -1;
    }
}

static void
pango_win32_font_get_glyph_extents (PangoFont      *font,
				    PangoGlyph      glyph,
				    PangoRectangle *ink_rect,
				    PangoRectangle *logical_rect)
{
  PangoWin32SubfontInfo *info;
  SIZE size;

  if (glyph && pango_win32_find_glyph (font, glyph, &info, &size))
    {
      /* This is totally bogus */
      if (ink_rect)
	{
	  ink_rect->x = PANGO_SCALE * size.cx;
	  ink_rect->width = ink_rect->x;
	  ink_rect->y = PANGO_SCALE * -size.cy;
	  ink_rect->height = PANGO_SCALE * size.cy;
	}
      if (logical_rect)
	{
	  logical_rect->x = 0;
	  logical_rect->width = PANGO_SCALE * size.cx;
	  logical_rect->y = - PANGO_SCALE * size.cy;
	  logical_rect->height = PANGO_SCALE * size.cy;
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
  HFONT hfont;
  HGDIOBJ oldfont;
  TEXTMETRIC tm;
  GSList *tmp_list = subfonts;
  gboolean first = TRUE;
  
  metrics->ascent = 0;
  metrics->descent = 0;
  
  while (tmp_list)
    {
      PangoWin32SubfontInfo *info = pango_win32_find_subfont (font, GPOINTER_TO_UINT (tmp_list->data));
      
      if (info)
	{
	  hfont = pango_win32_get_hfont (font, info);
	  if (hfont != NULL)
	    {
	      oldfont = SelectObject (pango_win32_hdc, hfont);
	      GetTextMetrics (pango_win32_hdc, &tm);
	      SelectObject (pango_win32_hdc, oldfont);
	      if (first)
		{
		  metrics->ascent = tm.tmAscent * PANGO_SCALE;
		  metrics->descent = tm.tmDescent * PANGO_SCALE;
		  first = FALSE;
		}
	      else
		{
		  metrics->ascent = MAX (tm.tmAscent * PANGO_SCALE, metrics->ascent);
		  metrics->descent = MAX (tm.tmDescent * PANGO_SCALE, metrics->descent);
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
	      PangoWin32Subfont subfont = PANGO_WIN32_GLYPH_SUBFONT (glyph_str->glyphs[j].glyph);
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
  { "fi", "Finnish (Suomi)  Hei, Hyvää päivää" },
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
  { "sv", "Swedish (Svenska) Hej på dej" },
  { "tr", "Turkish (Türkçe) Merhaba" },
  { "zh", "Chinese (中文,普通话,汉语)" }
};

static void
pango_win32_font_get_metrics (PangoFont        *font,
			      const gchar      *lang,
			      PangoFontMetrics *metrics)
{
  PangoWin32MetricsInfo *info;
  PangoWin32Font *win32font = (PangoWin32Font *)font;
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
  
  tmp_list = win32font->metrics_by_lang;
  while (tmp_list)
    {
      info = tmp_list->data;
      
      if (info->lang == lookup_lang)        /* We _don't_ need strcmp */
	break;

      tmp_list = tmp_list->next;
    }

  if (!tmp_list)
    {
      info = g_new (PangoWin32MetricsInfo, 1);
      info->lang = lookup_lang;

      win32font->metrics_by_lang = g_slist_prepend (win32font->metrics_by_lang, info);
      
      get_font_metrics_from_string (font, lang, str, &info->metrics);
    }
      
  *metrics = info->metrics;
  return;
}

static PangoWin32Subfont
pango_win32_insert_subfont (PangoFont     *font,
			    const LOGFONT *lfp)
{
  PangoWin32Font *win32font = (PangoWin32Font *)font;
  PangoWin32SubfontInfo *info;
  
  info = g_new (PangoWin32SubfontInfo, 1);
  
  info->logfont = *lfp;
  info->hfont = NULL;
  info->buf_hbm = NULL;

  win32font->n_subfonts++;
  
  if (win32font->n_subfonts > win32font->max_subfonts)
    {
      win32font->max_subfonts *= 2;
      win32font->subfonts = g_renew (PangoWin32SubfontInfo *, win32font->subfonts, win32font->max_subfonts);
    }
  
  win32font->subfonts[win32font->n_subfonts - 1] = info;
  
  return win32font->n_subfonts;
}

/**
 * pango_win32_list_subfonts:
 * @font: a PangoFont
 * @subrange: the Unicode subrange to list subfonts for
 * @subfont_ids: location to store a pointer to an array of subfont IDs for each found subfont
 *               the result must be freed using g_free()
 * 
 * Returns number of subfonts found
 **/
int
pango_win32_list_subfonts (PangoFont                 *font,
			   PangoWin32UnicodeSubrange  subrange,
			   PangoWin32Subfont 	    **subfont_ids)
{
  LOGFONT *lfp;
  PangoWin32Font *win32font = (PangoWin32Font *)font;
  PangoWin32Subfont *subfont_list;
  int i, j;
  int n_subfonts;

  g_return_val_if_fail (font != NULL, 0);

  subfont_list = g_hash_table_lookup (win32font->subfonts_by_subrange, (gpointer) subrange);
  if (!subfont_list)
    {
      subfont_list = g_new (PangoWin32Subfont, win32font->n_logfonts);
      
      for (i = 0; i < win32font->n_logfonts; i++)
	{
	  /* Does this font cover the subrange? */
	  PangoWin32Subfont subfont = 0;
	  
	  if (pango_win32_logfont_has_subrange (win32font->fontmap, win32font->logfonts+i, subrange))
	  {
	    lfp = pango_win32_make_matching_logfont (win32font->fontmap, win32font->logfonts+i, win32font->size);
	    if (lfp)
	      {
		subfont = pango_win32_insert_subfont (font, lfp);
		g_free (lfp);
	      }
	  }
	  
	  subfont_list[i] = subfont;
	}
      
      g_hash_table_insert (win32font->subfonts_by_subrange, (gpointer) subrange, subfont_list);
    }
  
  n_subfonts = 0;
  for (i = 0; i < win32font->n_logfonts; i++)
    if (subfont_list[i])
      n_subfonts++;

  *subfont_ids = g_new (PangoWin32Subfont, n_subfonts);

  n_subfonts = 0;
  for (i = 0; i < win32font->n_logfonts; i++)
    if (subfont_list[i])
      {
	(*subfont_ids)[n_subfonts] = subfont_list[i];
	n_subfonts++;
      }

  return n_subfonts;
}

static HBITMAP
create_bitmap_dibsection (HDC    hdc,
			  char **bits,
			  int   *size,
			  int    width,
			  int    height)
{
  struct {
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD bmiColors[2];
  } bmi;
  DIBSECTION ds;
  HBITMAP result;

  bmi.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = width;
  bmi.bmiHeader.biHeight = -height;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 1;
  bmi.bmiHeader.biCompression = BI_RGB;
  bmi.bmiHeader.biSizeImage = 0;
  bmi.bmiHeader.biXPelsPerMeter =
    bmi.bmiHeader.biYPelsPerMeter = 0;
  bmi.bmiHeader.biClrUsed = 0;
  bmi.bmiHeader.biClrImportant = 0;
  
  bmi.bmiColors[0].rgbBlue =
    bmi.bmiColors[0].rgbGreen =
    bmi.bmiColors[0].rgbRed = 0x00;
  bmi.bmiColors[0].rgbReserved = 0x00;
  
  bmi.bmiColors[1].rgbBlue =
    bmi.bmiColors[1].rgbGreen =
    bmi.bmiColors[1].rgbRed = 0xFF;
  bmi.bmiColors[1].rgbReserved = 0x00;
      
  result = CreateDIBSection (hdc, (BITMAPINFO *)&bmi, DIB_RGB_COLORS,
			     (PVOID *)bits, NULL, 0);
  if (size != NULL)
    {
      GetObject (result, sizeof (ds), &ds);
      *size = ds.dsBmih.biSizeImage;
    }
  return result;
}

static gboolean
subfont_has_glyph (PangoFont             *font,
		   PangoWin32SubfontInfo *info,
		   wchar_t                wc)

{
  TEXTMETRIC tm;
  PangoWin32Font *win32font = (PangoWin32Font *) font;
  wchar_t default_wc;
#ifdef HEAVY_DEBUGGING
  static int dispx = 0, dispy = 0;
#endif

  if (!pango_win32_logfont_has_subrange (win32font->fontmap, &info->logfont,
					 pango_win32_unicode_classify (wc)))
    return FALSE;

  if (info->buf_hbm == NULL)
    {
      info->buf_hdc = CreateCompatibleDC (pango_win32_hdc);
      info->oldfont = SelectObject (info->buf_hdc, info->hfont);
      SetTextAlign (info->buf_hdc, TA_LEFT|TA_BASELINE|TA_NOUPDATECP);
      GetTextMetrics (info->buf_hdc, &tm);
      PING(("wt:%d,ht:%d",tm.tmMaxCharWidth,tm.tmHeight));

      info->default_char_hbm =
	create_bitmap_dibsection (info->buf_hdc, &info->default_char_buf,
				  &info->buf_size,
				  tm.tmMaxCharWidth, tm.tmHeight);
      
      info->oldbm = SelectObject (info->buf_hdc, info->default_char_hbm);
      info->buf_rect.left = 0;
      info->buf_rect.top = 0;
      info->buf_rect.right = tm.tmMaxCharWidth;
      info->buf_rect.bottom = tm.tmHeight;
      info->buf_x = 0;
      info->buf_y = tm.tmHeight - tm.tmDescent;
      FillRect (info->buf_hdc, &info->buf_rect, white_brush);

      default_wc = tm.tmDefaultChar;
      TextOutW (info->buf_hdc, info->buf_x, info->buf_y, &default_wc, 1);
#ifdef HEAVY_DEBUGGING
      if (wc < 256)
	{
	  BitBlt (pango_win32_hdc,dispx,dispy,tm.tmMaxCharWidth,tm.tmHeight,info->buf_hdc,0,0,SRCCOPY);
	  dispx += tm.tmMaxCharWidth + 5;
	  if (dispx > 1000)
	    {
	      dispx = 0;
	      dispy += tm.tmHeight + 5;
	    }
	}
#endif

      info->buf_hbm =
	create_bitmap_dibsection (info->buf_hdc, &info->buf,
				  NULL,
				  tm.tmMaxCharWidth, tm.tmHeight);
      SelectObject (info->buf_hdc, info->buf_hbm);
    }

  /* Draw character into our bitmap; compare with the bitmap for
   * the default character. If they are identical, this character
   * does not exist in the font.
   */
  FillRect (info->buf_hdc, &info->buf_rect, white_brush);
  TextOutW (info->buf_hdc, info->buf_x, info->buf_y, &wc, 1);
#ifdef HEAVY_DEBUGGING
  if (wc < 256)
    {
      BitBlt (pango_win32_hdc,dispx,dispy,info->buf_rect.right,info->buf_rect.bottom,info->buf_hdc,0,0,SRCCOPY);
      dispx += info->buf_rect.right + 5;
      if (dispx > 1000)
	{
	  dispx = 0;
	  dispy += info->buf_rect.bottom + 5;
	}
    }
#endif

  return (memcmp (info->buf, info->default_char_buf, info->buf_size) != 0);
}

/**
 * pango_win32_has_glyph:
 * @font: a #PangoFont which must be from the Win32 backend.
 * @glyph: the index of a glyph in the font. (Formed
 *         using the PANGO_WIN32_MAKE_GLYPH macro)
 * 
 * Check if the given glyph is present in a Win32 font.
 * 
 * Return value: %TRUE if the glyph is present.
 **/
gboolean
pango_win32_has_glyph (PangoFont  *font,
		       PangoGlyph  glyph)
{
  HFONT hfont;
  PangoWin32SubfontInfo *info;
  guint16 char_index = PANGO_WIN32_GLYPH_INDEX (glyph);
  guint16 subfont_index = PANGO_WIN32_GLYPH_SUBFONT (glyph);

  info = pango_win32_find_subfont (font, subfont_index);
  if (!info)
    return FALSE;
  
  hfont = pango_win32_get_hfont (font, info);
  if (hfont == NULL)
    return FALSE;

  return subfont_has_glyph (font, info, char_index);
}

/**
 * pango_win32_font_subfont_logfont:
 * @font: a #PangoFont which must be from the Win32 backend
 * @subfont_id: the id of a subfont within the font.
 * 
 * Determine the LOGFONT struct for the specified subfont.
 * 
 * Return value: A newly allocated LOGFONT struct. It must be
 * freed with g_free().
 **/
LOGFONT *
pango_win32_font_subfont_logfont (PangoFont         *font,
				  PangoWin32Subfont  subfont_id)
{
  PangoWin32SubfontInfo *info;
  LOGFONT *lfp;

  g_return_val_if_fail (font != NULL, NULL);
  g_return_val_if_fail (PANGO_WIN32_IS_FONT (font), NULL);

  info = pango_win32_find_subfont (font, subfont_id);
  if (!info)
    {
      g_warning ("pango_win32_font_subfont_logfont: Invalid subfont_id specified");
      return NULL;
    }

  lfp = g_new (LOGFONT, 1);
  *lfp = info->logfont;

  return lfp;
}

static void
pango_win32_font_shutdown (GObject *object)
{
  PangoWin32Font *win32font = PANGO_WIN32_FONT (object);

  /* If the font is not already in the freed-fonts cache, add it,
   * if it is already there, do nothing and the font will be
   * freed.
   */
  if (!win32font->in_cache && win32font->fontmap)
    pango_win32_fontmap_cache_add (win32font->fontmap, win32font);

  G_OBJECT_CLASS (parent_class)->shutdown (object);
}

static void
subfonts_foreach (gpointer key,
		  gpointer value,
		  gpointer data)
{
  g_free (value);
}

static void
pango_win32_font_finalize (GObject *object)
{
  PangoWin32Font *win32font = (PangoWin32Font *)object;
  PangoWin32FontCache *cache = pango_win32_font_map_get_font_cache (win32font->fontmap);
  int i;

  for (i = 0; i < win32font->n_subfonts; i++)
    {
      PangoWin32SubfontInfo *info = win32font->subfonts[i];

      if (info->hfont != NULL)
	pango_win32_font_cache_unload (cache, info->hfont);

      if (info->buf_hbm != NULL)
	{
	  SelectObject (info->buf_hdc, info->oldfont);
	  SelectObject (info->buf_hdc, info->oldbm);
	  DeleteObject (info->buf_hbm);
	  DeleteObject (info->default_char_hbm);
	  DeleteDC (info->buf_hdc);
	}
      g_free (info);
    }

  g_free (win32font->subfonts);

  g_hash_table_foreach (win32font->subfonts_by_subrange, subfonts_foreach, NULL);
  g_hash_table_destroy (win32font->subfonts_by_subrange);

  g_slist_foreach (win32font->metrics_by_lang, (GFunc)g_free, NULL);
  g_slist_free (win32font->metrics_by_lang);
  
  if (win32font->entry)
    pango_win32_font_entry_remove (win32font->entry, (PangoFont *)win32font);

  g_object_unref (G_OBJECT (win32font->fontmap));

  g_free (win32font->logfonts);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static PangoFontDescription *
pango_win32_font_describe (PangoFont *font)
{
  /* FIXME: implement */
  return NULL;
}

PangoMap *
pango_win32_get_shaper_map (const char *lang)
{
  static guint engine_type_id = 0;
  static guint render_type_id = 0;
  
  if (engine_type_id == 0)
    {
      engine_type_id = g_quark_from_static_string (PANGO_ENGINE_TYPE_SHAPE);
      render_type_id = g_quark_from_static_string (PANGO_RENDER_TYPE_WIN32);
    }
  
  return pango_find_map (lang, engine_type_id, render_type_id);
}

static PangoCoverage *
pango_win32_font_get_coverage (PangoFont  *font,
			       const char *lang)
{
  PangoWin32Font *win32font = (PangoWin32Font *)font;

  return pango_win32_font_entry_get_coverage (win32font->entry, font, lang);
}

static PangoEngineShape *
pango_win32_font_find_shaper (PangoFont   *font,
			      const gchar *lang,
			      guint32      ch)
{
  PangoMap *shape_map = NULL;

  shape_map = pango_win32_get_shaper_map (lang);
  return (PangoEngineShape *)pango_map_get_engine (shape_map, ch);
}

/* Utility functions */

static gboolean
pango_win32_find_glyph (PangoFont              *font,
			PangoGlyph              glyph,
			PangoWin32SubfontInfo **subfont_return,
			SIZE                   *size_return)
{
  SIZE size;
  PangoWin32SubfontInfo *info;
  guint16 char_index = PANGO_WIN32_GLYPH_INDEX (glyph);
  guint16 subfont_index = PANGO_WIN32_GLYPH_SUBFONT (glyph);

  info = pango_win32_find_subfont (font, subfont_index);
  if (!info)
    return FALSE;
  
  if (!subfont_has_glyph (font, info, char_index))
    return FALSE;

  GetTextExtentPoint32W (info->buf_hdc, &char_index, 1, &size);

  if (subfont_return)
    *subfont_return = info;

  if (size_return)
    *size_return = size;
      
  return TRUE;
}

/**
 * pango_win32_get_unknown_glyph:
 * @font: a #PangoFont
 * 
 * Return the index of a glyph suitable for drawing unknown characters.
 * 
 * Return value: a glyph index into @font
 **/
PangoGlyph
pango_win32_get_unknown_glyph (PangoFont *font)
{
  return PANGO_WIN32_MAKE_GLYPH (1, 0);	/* XXX */
}

/**
 * pango_win32_render_layout_line:
 * @hdc:       HDC to use for uncolored drawing
 * @line:      a #PangoLayoutLine
 * @x:         the x position of start of string (in pixels)
 * @y:         the y position of baseline (in pixels)
 *
 * Render a #PangoLayoutLine onto a device context
 */
void 
pango_win32_render_layout_line (HDC               hdc,
				PangoLayoutLine  *line,
				int               x, 
				int               y)
{
  GSList *tmp_list = line->runs;
  PangoRectangle overall_rect;
  PangoRectangle logical_rect;
  PangoRectangle ink_rect;
  PangoContext *context = pango_layout_get_context (line->layout);
  
  int x_off = 0;

  pango_layout_line_get_extents (line,NULL, &overall_rect);
  
  while (tmp_list)
    {
      HBRUSH oldfg;
      HBRUSH brush;
      POINT points[2];
      PangoUnderline uline = PANGO_UNDERLINE_NONE;
      PangoLayoutRun *run = tmp_list->data;
      PangoAttrColor fg_color, bg_color;
      gboolean fg_set, bg_set;
      
      tmp_list = tmp_list->next;

      pango_win32_get_item_properties (run->item, &uline, &fg_color, &fg_set, &bg_color, &bg_set);

      if (uline == PANGO_UNDERLINE_NONE)
	pango_glyph_string_extents (run->glyphs, run->item->analysis.font,
				    NULL, &logical_rect);
      else
	pango_glyph_string_extents (run->glyphs, run->item->analysis.font,
				    &ink_rect, &logical_rect);

      if (bg_set)
	{
	  HBRUSH oldbrush;

	  brush = CreateSolidBrush (RGB ((bg_color.red + 128) >> 8,
					 (bg_color.green + 128) >> 8,
					 (bg_color.blue + 128) >> 8));
	  oldbrush = SelectObject (hdc, brush);
	  Rectangle (hdc, x + (x_off + logical_rect.x) / PANGO_SCALE,
			  y + overall_rect.y / PANGO_SCALE,
			  logical_rect.width / PANGO_SCALE,
			  overall_rect.height / PANGO_SCALE);
	  SelectObject (hdc, oldbrush);
	  DeleteObject (brush);
	}

      if (fg_set)
	{
	  brush = CreateSolidBrush (RGB ((fg_color.red + 128) >> 8,
					 (fg_color.green + 128) >> 8,
					 (fg_color.blue + 128) >> 8));
	  oldfg = SelectObject (hdc, brush);
	}

      pango_win32_render (hdc, run->item->analysis.font, run->glyphs,
			  x + x_off / PANGO_SCALE, y);

      switch (uline)
	{
	case PANGO_UNDERLINE_NONE:
	  break;
	case PANGO_UNDERLINE_DOUBLE:
	  points[0].x = x + (x_off + ink_rect.x) / PANGO_SCALE - 1;
	  points[0].y = points[1].y = y + 4;
	  points[1].x = x + (x_off + ink_rect.x + ink_rect.width) / PANGO_SCALE;
	  Polyline (hdc, points, 2);
	  /* Fall through */
	case PANGO_UNDERLINE_SINGLE:
	  points[0].y = points[1].y = y + 2;
	  Polyline (hdc, points, 2);
	  break;
	case PANGO_UNDERLINE_LOW:
	  points[0].x = x + (x_off + ink_rect.x) / PANGO_SCALE - 1;
	  points[0].y = points[1].y = y + (ink_rect.y + ink_rect.height) / PANGO_SCALE + 2;
	  points[1].x = x + (x_off + ink_rect.x + ink_rect.width) / PANGO_SCALE;
	  Polyline (hdc, points, 2);
	  break;
	}

      if (fg_set)
	{
	  SelectObject (hdc, oldfg);
	  DeleteObject (brush);
	}
      
      x_off += logical_rect.width;
    }
}

/**
 * pango_win32_render_layout:
 * @hdc:       HDC to use for uncolored drawing
 * @layout:    a #PangoLayout
 * @x:         the X position of the left of the layout (in pixels)
 * @y:         the Y position of the top of the layout (in pixels)
 *
 * Render a #PangoLayoutLine onto an X drawable
 */
void 
pango_win32_render_layout (HDC              hdc,
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
	  
      pango_win32_render_layout_line (hdc, line,
				      x + x_offset / PANGO_SCALE,
				      y + (y_offset - logical_rect.y) / PANGO_SCALE);

      y_offset += logical_rect.height;
      tmp_list = tmp_list->next;
    }
}

/* This utility function is duplicated here and in pango-layout.c; should it be
 * public? Trouble is - what is the appropriate set of properties?
 */
static void
pango_win32_get_item_properties (PangoItem      *item,
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

