/* Pango
 * pangowin32.h:
 *
 * Copyright (C) 1999 Red Hat Software
 * Copyright (C) 2000 Tor Lillqvist
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

#ifndef __PANGOWIN32_H__
#define __PANGOWIN32_H__

#include <glib.h>
#include <pango/pango.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define STRICT
#include <windows.h>

#define PANGO_RENDER_TYPE_WIN32 "PangoRenderWin32"

/* Calls for applications
 */
PangoContext * pango_win32_get_context        (void);

PangoFont *    pango_win32_load_font          (LOGFONT          *lfps,
					       int               n_fonts);
void           pango_win32_render             (HDC               hdc,
					       PangoFont        *font,
					       PangoGlyphString *glyphs,
					       gint              x,
					       gint              y);
void           pango_win32_render_layout_line (HDC               hdc,
					       PangoLayoutLine  *line,
					       int               x,
					       int               y);
void           pango_win32_render_layout      (HDC               hdc,
					       PangoLayout      *layout,
					       int               x, 
					       int               y);



/* API for rendering modules
 */

/* This enum classifies Unicode characters according to the Microsoft
 * Unicode subrange numbering. This is based on the table in "Developing
 * International Software for Windows 95 and Windows NT". This is almost,
 * but not quite, the same as the official Unicode block table in
 * Blocks.txt from ftp.unicode.org.
 */
typedef enum
{
  PANGO_WIN32_U_BASIC_LATIN = 0,
  PANGO_WIN32_U_LATIN_1_SUPPLEMENT = 1,
  PANGO_WIN32_U_LATIN_EXTENDED_A = 2,
  PANGO_WIN32_U_LATIN_EXTENDED_B = 3,
  PANGO_WIN32_U_IPA_EXTENSIONS = 4,
  PANGO_WIN32_U_SPACING_MODIFIER_LETTERS = 5,
  PANGO_WIN32_U_COMBINING_DIACRITICAL_MARKS = 6,
  PANGO_WIN32_U_BASIC_GREEK = 7,
  PANGO_WIN32_U_GREEK_SYMBOLS_AND_COPTIC = 8,
  PANGO_WIN32_U_CYRILLIC = 9,
  PANGO_WIN32_U_ARMENIAN = 10,
  PANGO_WIN32_U_HEBREW_EXTENDED = 12,
  PANGO_WIN32_U_BASIC_HEBREW = 11,
  PANGO_WIN32_U_BASIC_ARABIC = 13,
  PANGO_WIN32_U_ARABIC_EXTENDED = 14,
  PANGO_WIN32_U_DEVANAGARI = 15,
  PANGO_WIN32_U_BENGALI = 16,
  PANGO_WIN32_U_GURMUKHI = 17,
  PANGO_WIN32_U_GUJARATI = 18,
  PANGO_WIN32_U_ORIYA = 19,
  PANGO_WIN32_U_TAMIL = 20,
  PANGO_WIN32_U_TELUGU = 21,
  PANGO_WIN32_U_KANNADA = 22,
  PANGO_WIN32_U_MALAYALAM = 23,
  PANGO_WIN32_U_THAI = 24,
  PANGO_WIN32_U_LAO = 25,
  PANGO_WIN32_U_GEORGIAN_EXTENDED = 27,
  PANGO_WIN32_U_BASIC_GEORGIAN = 26,
  PANGO_WIN32_U_HANGUL_JAMO = 28,
  PANGO_WIN32_U_LATIN_EXTENDED_ADDITIONAL = 29,
  PANGO_WIN32_U_GREEK_EXTENDED = 30,
  PANGO_WIN32_U_GENERAL_PUNCTUATION = 31,
  PANGO_WIN32_U_SUPERSCRIPTS_AND_SUBSCRIPTS = 32,
  PANGO_WIN32_U_CURRENCY_SYMBOLS = 33,
  PANGO_WIN32_U_COMBINING_DIACRITICAL_MARKS_FOR_SYMBOLS = 34,
  PANGO_WIN32_U_LETTERLIKE_SYMBOLS = 35,
  PANGO_WIN32_U_NUMBER_FORMS = 36,
  PANGO_WIN32_U_ARROWS = 37,
  PANGO_WIN32_U_MATHEMATICAL_OPERATORS = 38,
  PANGO_WIN32_U_MISCELLANEOUS_TECHNICAL = 39,
  PANGO_WIN32_U_CONTROL_PICTURES = 40,
  PANGO_WIN32_U_OPTICAL_CHARACTER_RECOGNITION = 41,
  PANGO_WIN32_U_ENCLOSED_ALPHANUMERICS = 42,
  PANGO_WIN32_U_BOX_DRAWING = 43,
  PANGO_WIN32_U_BLOCK_ELEMENTS = 44,
  PANGO_WIN32_U_GEOMETRIC_SHAPES = 45,
  PANGO_WIN32_U_MISCELLANEOUS_SYMBOLS = 46,
  PANGO_WIN32_U_DINGBATS = 47,
  PANGO_WIN32_U_CJK_SYMBOLS_AND_PUNCTUATION = 48,
  PANGO_WIN32_U_HIRAGANA = 49,
  PANGO_WIN32_U_KATAKANA = 50,
  PANGO_WIN32_U_BOPOMOFO = 51,
  PANGO_WIN32_U_HANGUL_COMPATIBILITY_JAMO = 52,
  PANGO_WIN32_U_CJK_MISCELLANEOUS = 53,
  PANGO_WIN32_U_ENCLOSED_CJK = 54,
  PANGO_WIN32_U_CJK_COMPATIBILITY = 55,
  PANGO_WIN32_U_HANGUL = 56,
  PANGO_WIN32_U_HANGUL_SUPPLEMENTARY_A = 57,
  PANGO_WIN32_U_HANGUL_SUPPLEMENTARY_B = 58,
  PANGO_WIN32_U_CJK_UNIFIED_IDEOGRAPHS = 59,
  PANGO_WIN32_U_PRIVATE_USE_AREA = 60,
  PANGO_WIN32_U_CJK_COMPATIBILITY_IDEOGRAPHS = 61,
  PANGO_WIN32_U_ALPHABETIC_PRESENTATION_FORMS = 62,
  PANGO_WIN32_U_ARABIC_PRESENTATION_FORMS_A = 63,
  PANGO_WIN32_U_COMBINING_HALF_MARKS = 64,
  PANGO_WIN32_U_CJK_COMPATIBILITY_FORMS = 65,
  PANGO_WIN32_U_SMALL_FORM_VARIANTS = 66,
  PANGO_WIN32_U_ARABIC_PRESENTATION_FORMS_B = 67,
  PANGO_WIN32_U_SPECIALS = 69,
  PANGO_WIN32_U_HALFWIDTH_AND_FULLWIDTH_FORMS = 68,
  PANGO_WIN32_U_LAST_PLUS_ONE
} PangoWin32UnicodeSubrange;

PangoWin32UnicodeSubrange pango_win32_unicode_classify (wchar_t wc);

typedef guint16 PangoWin32Subfont;

#define PANGO_WIN32_MAKE_GLYPH(subfont,index) ((subfont)<<16 | (index))
#define PANGO_WIN32_GLYPH_SUBFONT(glyph) ((glyph)>>16)
#define PANGO_WIN32_GLYPH_INDEX(glyph) ((glyph) & 0xffff)

int        pango_win32_list_subfonts 	(PangoFont                 *font,
				     	 PangoWin32UnicodeSubrange  subrange,
				     	 PangoWin32Subfont        **subfont_ids);
gboolean   pango_win32_has_glyph     	(PangoFont                 *font,
				     	 PangoGlyph                 glyph);
PangoGlyph pango_win32_get_unknown_glyph (PangoFont                *font);

/* API for libraries that want to use PangoWin32 mixed with classic
 * Win32 fonts.
 */
typedef struct _PangoWin32FontCache PangoWin32FontCache;

PangoWin32FontCache *pango_win32_font_cache_new          (void);
void                 pango_win32_font_cache_free         (PangoWin32FontCache *cache);
 
HFONT                pango_win32_font_cache_load         (PangoWin32FontCache *cache,
						          const LOGFONT       *logfont);
void                 pango_win32_font_cache_unload       (PangoWin32FontCache *cache,
						     	  HFONT                hfont);

PangoFontMap        *pango_win32_font_map_for_display    (void);
void                 pango_win32_shutdown_display        (void);
PangoWin32FontCache *pango_win32_font_map_get_font_cache (PangoFontMap       *font_map);

LOGFONT             *pango_win32_font_subfont_logfont    (PangoFont          *font,
							  PangoWin32Subfont   subfont_id);

/* Debugging.
 */
void                 pango_win32_fontmap_dump            (int                 indent,
							  PangoFontMap       *fontmap);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __PANGOWIN32_H__ */
