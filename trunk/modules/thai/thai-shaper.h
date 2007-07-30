/* Pango
 * thai-shaper.h:
 *
 * Copyright (C) 1999 Red Hat Software
 * Author: Owen Taylor <otaylor@redhat.com>
 *
 * Copyright (C) 2004 Theppitak Karoonboonyanan
 * Copyright (C) 2002 Software and Language Engineering Laboratory, NECTEC
 * Author: Theppitak Karoonboonyanan <thep@linux.thai.net>
 *
 * Copyright (c) 1996-2000 by Sun Microsystems, Inc.
 * Author: Chookij Vanatham <Chookij.Vanatham@Eng.Sun.COM>
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

#ifndef __THAI_SHAPER_H__
#define __THAI_SHAPER_H__

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
PangoGlyph
thai_get_glyph_tis (ThaiFontInfo *font_info, guchar c);

PangoGlyph
thai_make_glyph_tis (ThaiFontInfo *font_info, guchar c);

PangoGlyph
thai_get_glyph_uni (ThaiFontInfo *font_info, gunichar uc);

PangoGlyph
thai_make_glyph_uni (ThaiFontInfo *font_info, gunichar uc);

PangoGlyph
thai_make_unknown_glyph (ThaiFontInfo *font_info, gunichar uc);

void
thai_set_glyphs (ThaiFontInfo     *font_info,
		 const char       *text,
		 gint              length,
		 PangoScript       script,
		 PangoGlyphString *glyphs);

#endif /* __THAI_SHAPER_H__ */
