/* Pango
 * pango-indic.h: Library for Indic script rendering
 *
 * Copyright (C) 2000 SuSE Linux Ltd.
 *
 * Author: Robert Brady <rwb197@zepler.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * Licence as published by the Free Software Foundation; either
 * version 2 of the Licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public Licence for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * Licence along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * This library is basically internal
 * 
 * Don't use it, unless you are me, or are in contact with me,
 * or like living dangerously.
 */

#ifndef __PANGO_INDIC_H__
#define __PANGO_INDIC_H__

#include <pango-glyph.h>

G_BEGIN_DECLS

#ifdef PANGO_ENABLE_ENGINE

#define PANGO_ZERO_WIDTH_NON_JOINER 0x200c
#define PANGO_ZERO_WIDTH_JOINER     0x200d

typedef struct _PangoIndicScript PangoIndicScript;

struct _PangoIndicScript {
  /* Compulsory */
  gchar *name;
  /* Compulsory */
  gboolean (*is_prefixing_vowel)  (gunichar  what);
  /* Compulsory */
  gboolean (*is_vowel_sign)       (gunichar  what);
  /* Optional */
  gunichar (*vowel_sign_to_matra) (gunichar  what);
  /* Optional */
  gboolean (*is_vowel_half)       (gunichar  what);
  
  /* Optional */
  gboolean (*vowel_split)         (gunichar  what, 
                                   gunichar *prefix, 
                                   gunichar *suffix);
};

void pango_indic_shift_vowels   (PangoIndicScript *script,
                                 gunichar         *chars, 
                                 gunichar         *end);

void pango_indic_compact        (PangoIndicScript *script,
                                 int              *num, 
                                 gunichar         *chars,
                                 int              *cluster);

void pango_indic_convert_vowels (PangoIndicScript *script,
                                 gboolean          in_middle,
                                 int              *num,
                                 gunichar         *chars,
                                 gboolean         has_standalone_vowels);

void pango_indic_split_out_characters (PangoIndicScript *script, 
                                       const gchar *text, 
                                       int n_chars, 
                                       gunichar **wc, 
                                       int *n_glyph, 
                                       PangoGlyphString *glyphs);

#endif /* PANGO_ENABLE_ENGINE */

G_END_DECLS

#endif /* __PANGO_INDIC_H */
