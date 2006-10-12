/* Pango
 * indic-lang.c:
 *
 * Copyright (C) 2006 Red Hat Software
 * Author: Akira TAGOH <tagoh@redhat.com>
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

#include <config.h>
#include <string.h>

#include "pango-engine.h"
#include "pango-break.h"

typedef PangoEngineLang	IndicEngineLang;
typedef PangoEngineLangClass	IndicEngineLangClass;

#define ENGINE_SUFFIX		"IndicScriptEngineLang"
#define RENDER_TYPE		PANGO_RENDER_TYPE_NONE
#define INDIC_ENGINE_INFO(script)					\
  {#script ENGINE_SUFFIX, PANGO_ENGINE_TYPE_LANG, RENDER_TYPE, script##_scripts, G_N_ELEMENTS(script##_scripts)}


static PangoEngineScriptInfo deva_scripts[] = {
  { PANGO_SCRIPT_DEVANAGARI, "*" }
};

static PangoEngineScriptInfo sinh_scripts[] = {
  { PANGO_SCRIPT_SINHALA, "*" }
};

static PangoEngineInfo script_engines[] = {
  INDIC_ENGINE_INFO(deva),
  INDIC_ENGINE_INFO(sinh)
};


static void
indic_engine_break (PangoEngineLang *engine,
		    const char      *text,
		    int              length,
		    PangoAnalysis   *analysis,
		    PangoLogAttr    *attrs,
		    int              attrs_len)
{
  const gchar *p, *next = NULL, *next_next;
  gunichar prev_wc, this_wc, next_wc, next_next_wc;
  gboolean makes_rephaya_syllable = FALSE;
  int i;

  for (p = text, prev_wc = 0, i = 0;
       p != NULL && p < (text + length);
       p = next, prev_wc = this_wc, i++)
    {
      this_wc = g_utf8_get_char (p);
      next = g_utf8_next_char (p);
      if (next != NULL && next < (text + length))
	{
	  next_wc = g_utf8_get_char (next);
	  next_next = g_utf8_next_char (next);
	}
      else
	{
	  next_wc = 0;
	  next_next = NULL;
	}
      if (next_next != NULL && next_next < (text + length))
	next_next_wc = g_utf8_get_char (next_next);
      else
	next_next_wc = 0;

      if (prev_wc != 0 && this_wc == 0x0DBB &&
	  next_wc == 0x0DCA && next_next_wc == 0x200D)
	{
	  /* Determine whether or not this forms a Rephaya syllable.
	   * SINHALA LETTER + U+0DBB U+0DCA U+200D + SINHALA LETTER is
	   * the kind of Rephaya.
	   */
	  makes_rephaya_syllable = TRUE;
	  attrs[i].is_cursor_position = FALSE;
	  attrs[i].is_char_break = FALSE;
	  attrs[i].is_line_break = FALSE;
	  attrs[i].is_mandatory_break = FALSE;
	}
      else if (prev_wc == 0x200D &&
		(makes_rephaya_syllable || this_wc == 0x0DBB || this_wc == 0x0DBA))
	{
	  /* fixes the cursor break in Sinhala.
	   * SINHALA LETTER + SINHALA VOWEL + ZWJ + 0x0DBB/0x0DBA is
	   * the kind of Rakaransaya/Yansaya. these characters has to
	   * be dealt as one character.
	   */
	  attrs[i].is_cursor_position = FALSE;
	  attrs[i].is_char_break = FALSE;
	  attrs[i].is_line_break = FALSE;
	  attrs[i].is_mandatory_break = FALSE;
	  makes_rephaya_syllable = FALSE;
	}
      else if (this_wc == 0x200D &&
		((makes_rephaya_syllable && next_wc != 0) ||
		 (next_wc == 0x0DBB || next_wc == 0x0DBA)))
	{
	  attrs[i].is_cursor_position = FALSE;
	  attrs[i].is_char_break = FALSE;
	  attrs[i].is_line_break = FALSE;
	  attrs[i].is_mandatory_break = FALSE;
	}
    }
}

static void
indic_engine_lang_class_init (PangoEngineLangClass *klass)
{
  klass->script_break = indic_engine_break;
}

PANGO_ENGINE_LANG_DEFINE_TYPE (IndicEngineLang, indic_engine_lang,
			       indic_engine_lang_class_init, NULL)

void
PANGO_MODULE_ENTRY(init) (GTypeModule *module)
{
  indic_engine_lang_register_type (module);
}

void
PANGO_MODULE_ENTRY(exit) (void)
{
}

void
PANGO_MODULE_ENTRY(list) (PangoEngineInfo **engines,
			  int               *n_engines)
{
  *engines = script_engines;
  *n_engines = G_N_ELEMENTS (script_engines);
}

PangoEngine *
PANGO_MODULE_ENTRY(create) (const char *id)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS(script_engines); i++)
    {
      if (!strcmp (id, script_engines[i].id))
	return g_object_new (indic_engine_lang_type, NULL);
    }

  return NULL;
}
