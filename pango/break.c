/* Pango
 * break.c:
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

#include "pango.h"
#include "pango-modules.h"

/**
 * pango_break:
 * @text:      the text to process
 * @length:    the length (in bytes) of @text
 * @analysis:  #PangoAnalysis structure from PangoItemize
 * @attrs:     an array to store character information in
 *
 * Determines possible line, word, and character breaks
 * for a string of Unicode text.
 */
void pango_break (const gchar   *text, 
		  gint           length, 
		  PangoAnalysis *analysis, 
		  PangoLogAttr  *attrs)
{
  /* Pseudo-implementation */

  const gchar *cur = text;
  gint i = 0;
  gunichar wc;
  
  while (*cur && cur - text < length)
    {
      wc = g_utf8_get_char (cur);
      if (wc == (gunichar)-1)
	break;			/* FIXME: ERROR */

      attrs[i].is_white = (wc == ' ' || wc == '\t' || wc == '\n' || wc == 0x200b) ? 1 : 0;
      attrs[i].is_break = i == 0 || attrs[i-1].is_white || attrs[i].is_white;
      attrs[i].is_char_stop = 1;
      attrs[i].is_word_stop = ((i == 0) || attrs[i-1].is_white) && !attrs[i].is_white;
      
      i++;
      cur = g_utf8_next_char (cur);
    }
}

/**
 * pango_get_log_attrs:
 * @text: text to process
 * @length: length in bytes of @text
 * @level: embedding level, or -1 if unknown
 * @language: language code
 * @log_attrs: array with one PangoLogAttr per character in @text, to be filled in
 *
 * Computes a PangoLogAttr for each character in @text
 */
void
pango_get_log_attrs (const char    *text,
                     int            length,
                     int            level,
                     const char    *language,
                     PangoLogAttr  *log_attrs)
{
  int n_chars;
  PangoMap *lang_map;
  int chars_broken;
  const char *pos;
  const char *end;
  PangoEngineLang* range_engine;
  const char *range_start;
  int chars_in_range;
  static guint engine_type_id = 0;
  static guint render_type_id = 0;  
  PangoAnalysis analysis = { NULL, NULL, NULL, 0 };

  analysis.level = level;
  
  g_return_if_fail (length == 0 || text != NULL);
  g_return_if_fail (log_attrs != NULL);
  
  if (length == 0)
    return;
  
  if (engine_type_id == 0)
    {
      engine_type_id = g_quark_from_static_string (PANGO_ENGINE_TYPE_LANG);
      render_type_id = g_quark_from_static_string (PANGO_RENDER_TYPE_NONE);
    }

  n_chars = g_utf8_strlen (text, length);

  lang_map = pango_find_map (language, engine_type_id, render_type_id);
    
  range_start = text;
  range_engine = (PangoEngineLang*) pango_map_get_engine (lang_map,
                                                          g_utf8_get_char (text));
  analysis.lang_engine = range_engine;
  chars_broken = 0;
  chars_in_range = 1;
  
  end = text + length;
  pos = g_utf8_next_char (text);
  
  while (pos != end)
    {
      analysis.lang_engine =
        (PangoEngineLang*) pango_map_get_engine (lang_map,
                                                 g_utf8_get_char (pos));
      
      if (range_engine != analysis.lang_engine)
        {
          /* Engine has changed; do the breaking for the current range,
           * then start a new range.
           */
          pango_break (range_start,
                       pos - range_start,
                       &analysis,
                       log_attrs + chars_broken);

          chars_broken += chars_in_range;
          
          range_start = pos;
          range_engine = analysis.lang_engine;
          chars_in_range = 1;
        }
      else
        {
          chars_in_range += 1;
        }
      
      pos = g_utf8_next_char (pos);
    }
    
    g_assert (chars_in_range > 0);
    g_assert (range_start != end);
    g_assert (pos == end);
    g_assert (range_engine == analysis.lang_engine);
    
    pango_break (range_start,
                 end - range_start,
                 &analysis,
                 log_attrs + chars_broken);
}

