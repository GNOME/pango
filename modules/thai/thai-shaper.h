#ifndef __THAI_SHAPER_H__
#define __THAI_SHAPER_H__

#define isthai(wc)      (wc >= 0xE00 && wc < 0xE80)
#define ucs2tis(wc)     (unsigned int)((unsigned int)(wc) - 0x0E00 + 0xA0)
#define tis2uni(c)      ((gunichar)(c) - 0xA0 + 0x0E00)

typedef struct _ThaiFontInfo ThaiFontInfo;

/* Font encodings we will use
 */
typedef enum {
  THAI_FONT_NONE,
  THAI_FONT_TIS,
  THAI_FONT_TIS_MAC,
  THAI_FONT_TIS_WIN
} ThaiFontSet;

typedef enum {
  THAI_FONTINFO_X,
  THAI_FONTINFO_XFT
} ThaiFontInfoType;

struct _ThaiFontInfo
{
  PangoFont       *font;
  ThaiFontSet      font_set;
};

/*
 * Abstract methods (implemented by each shaper module)
 */
ThaiFontInfo *
thai_get_font_info (PangoFont *font);

PangoGlyph
thai_get_glyph_tis (ThaiFontInfo *font_info, gchar c);

PangoGlyph
thai_make_glyph_tis (ThaiFontInfo *font_info, gchar c);

PangoGlyph
thai_get_glyph_uni (ThaiFontInfo *font_info, gunichar uc);

PangoGlyph
thai_make_glyph_uni (ThaiFontInfo *font_info, gunichar uc);

PangoGlyph
thai_make_unknown_glyph (ThaiFontInfo *font_info, gunichar uc);

/*
 * Public functions
 */
void
thai_engine_shape (PangoEngineShape *engine,
		   PangoFont        *font,
                   const char       *text,
                   gint              length,
                   PangoAnalysis    *analysis,
                   PangoGlyphString *glyphs);

#endif /* __THAI_SHAPER_H__ */

