/* Pango
 * pango-indic.c: Library for Indic script rendering
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

#include "pango.h"
#include "pango-indic.h"

/**
 * pango_indic_shift_vowels:
 * @script: A #PangoIndicScript
 * @chars: Array of #gunichar
 * @end: Pointer to just after the end of @chars
 * 
 * This causes the any vowels in @chars which are
 * left-joining vowels to move to the start of @chars.
 *
 * It determines whether the vowels are left-joining 
 * by calling is_prefixing_vowel from @script.
 */
void 
pango_indic_shift_vowels (PangoIndicScript *script, 
                          gunichar         *chars, 
                          gunichar         *end) 
{
  int length = end - chars;
  int i, j;
  
  for (i = 1 ; i < length ; i++)
    {
      if (script->is_prefixing_vowel (chars[i]))
        {
           gunichar tmp = chars[i];
           
           for (j = i; j > 0; j--) {
             chars[j] = chars[j - 1];
           }

           chars[0] = tmp;
        }
    }
}

/**
 * pango_indic_compact:
 * @script: A #PangoIndicScript
 * @num: The number of glyphs
 * @chars: An array of glyphs/characters
 * @cluster: The cluster array.
 *
 * This eliminates any blank space in the @chars
 * array, updated @clusters and @num also. 
 * (Blank space is defines as U+0000)
 */
void
pango_indic_compact (PangoIndicScript *script, 
                     int              *num, 
                     gunichar         *chars, 
                     int              *cluster)
{
  gunichar *dest = chars;
  gunichar *end = chars + *num;
  int *cluster_dest = cluster;
  while (chars < end)
    {
      if (*chars)
        {
          *dest = *chars;
          *cluster_dest = *cluster;
          dest++;
          chars++;
          cluster++;
          cluster_dest++;
        }
      else
        {
          chars++;
          cluster++;
        }
    }
  *num -= (chars - dest);
}

static gunichar 
default_vowel_sign_to_matra (gunichar a) 
{
  return (a + 0xc000);
}

/**
 * pango_indic_convert_vowels:
 * @script: A #PangoIndicScript
 * @in_middle: Whether vowels should be converted 
 * @num: The number of elements in @chars.
 * @chars: An array of glyphs/characters
 * @has_standalone_vowels: Whether the font has standalone vowels.
 *
 * This converts the second two vowel signs in a row
 * in a string, to either a vowel letter or spacing forms
 * of the combining vowel.
 */
void
pango_indic_convert_vowels (PangoIndicScript *script,
                            gboolean          in_middle,
                            int              *num, 
                            gunichar         *chars,
                            gboolean          has_standalone_vowels)
{
  /* goes along and converts vowel signs to vowel letters if needed. */
  gunichar *end = chars + *num;
  gunichar *start = chars;
  gunichar (*vowel_sign_to_matra)(gunichar a) = script->vowel_sign_to_matra;
  gboolean last_was_vowel_sign = 0;

  vowel_sign_to_matra = FALSE;
  if (!vowel_sign_to_matra || has_standalone_vowels)
    vowel_sign_to_matra = default_vowel_sign_to_matra;

  while (chars < end)
    {
      gboolean convert = FALSE;
      gboolean is_vowel_sign = script->is_vowel_sign (chars[0]);
      if (is_vowel_sign) 
        {
          if (chars==start) 
            convert = TRUE;

          if (chars > start && in_middle &&
              (last_was_vowel_sign ||
               (script->is_vowel_half && script->is_vowel_half (chars[-1]))))
            convert = TRUE;
        }
      
      if (convert)
        chars[0] = vowel_sign_to_matra (chars[0]);
      
      last_was_vowel_sign = is_vowel_sign;

      chars++;
    }
}

/**
 * pango_indic_split_out_characters
 * @script: A #PangoIndicScript
 * @text: A UTF-8 string
 * @n_chars: The number of UTF-8 sequences in @text
 * @wc: Pointer to array of #gunichar (output param)
 * @n_glyph: Pointer to number of elements in @wc. (output param)
 * @glyphs: A #PangoGlyphString.
 *
 * This splits out the string @text into characters. It will
 * split out two-part vowels using @script->vowel_split if
 * this function is available.
 *
 * *@n_chars is allocated with g_new, you must free it.
 */
void 
pango_indic_split_out_characters (PangoIndicScript *script, 
                                       const char       *text, 
                                       int               n_chars, 
                                       gunichar        **wc, 
                                       int              *n_glyph, 
                                       PangoGlyphString *glyphs)
{
  const char *p = text;
  int i;
  *n_glyph = n_chars;

  if (script->vowel_split) 
    for (i = 0; i < n_chars; i++) 
      {
        gunichar u = g_utf8_get_char (p);
        if (script->vowel_split (u, NULL, NULL))
          (*n_glyph)++;
        p = g_utf8_next_char (p);
      }
  
  p = text;
  *wc = (gunichar *) g_new (gunichar, *n_glyph);
  pango_glyph_string_set_size (glyphs, *n_glyph);
  for (i = 0; i < *n_glyph; i++)
    {
      (*wc)[i] = g_utf8_get_char (p);
      glyphs->log_clusters[i] = p - text;
   
      if (script->vowel_split) 
        if (script->vowel_split ((*wc)[i], &(*wc)[i], &(*wc)[i+1]))
          {
            i++;
            glyphs->log_clusters[i] = p - text;
          }
      
      p = g_utf8_next_char (p);
    }
}


