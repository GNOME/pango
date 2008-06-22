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

#include "config.h"
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

static PangoEngineScriptInfo beng_scripts[] = {
  { PANGO_SCRIPT_BENGALI, "*" }
};

static PangoEngineScriptInfo guru_scripts[] = {
  { PANGO_SCRIPT_GURMUKHI, "*" }
};

static PangoEngineScriptInfo gujr_scripts[] = {
  { PANGO_SCRIPT_GUJARATI, "*" }
};

static PangoEngineScriptInfo orya_scripts[] = {
  { PANGO_SCRIPT_ORIYA, "*" }
};

static PangoEngineScriptInfo taml_scripts[] = {
  { PANGO_SCRIPT_TAMIL, "*" }
};

static PangoEngineScriptInfo telu_scripts[] = {
  { PANGO_SCRIPT_TELUGU, "*" }
};

static PangoEngineScriptInfo knda_scripts[] = {
  { PANGO_SCRIPT_KANNADA, "*" }
};

static PangoEngineScriptInfo mlym_scripts[] = {
  { PANGO_SCRIPT_MALAYALAM, "*" }
};

static PangoEngineScriptInfo sinh_scripts[] = {
  { PANGO_SCRIPT_SINHALA, "*" }
};

static PangoEngineInfo script_engines[] = {
  INDIC_ENGINE_INFO(deva), INDIC_ENGINE_INFO(beng), INDIC_ENGINE_INFO(guru),
  INDIC_ENGINE_INFO(gujr), INDIC_ENGINE_INFO(orya), INDIC_ENGINE_INFO(taml),
  INDIC_ENGINE_INFO(telu), INDIC_ENGINE_INFO(knda), INDIC_ENGINE_INFO(mlym),
  INDIC_ENGINE_INFO(sinh)
};

static void
not_cursor_position (PangoLogAttr *attr)
{
  attr->is_cursor_position = FALSE;
  attr->is_char_break = FALSE;
  attr->is_line_break = FALSE;
  attr->is_mandatory_break = FALSE;
}

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
  gboolean is_conjunct = FALSE;
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

      switch (analysis->script)
      {
        case PANGO_SCRIPT_SINHALA:
	  /*
	   * TODO: The cursor position should be based on the state table.
	   *       This is the wrong place to be doing this.
	   */

	  /*
	   * The cursor should treat as a single glyph:
	   * SINHALA CONS + 0x0DCA + 0x200D + SINHALA CONS
	   * SINHALA CONS + 0x200D + 0x0DCA + SINHALA CONS
	   */
	  if ((this_wc == 0x0DCA && next_wc == 0x200D)
	      || (this_wc == 0x200D && next_wc == 0x0DCA))
	    {
	      not_cursor_position(&attrs[i]);
	      not_cursor_position(&attrs[i + 1]);
	      is_conjunct = TRUE;
	    }
	  else if (is_conjunct
		   && (prev_wc == 0x200D || prev_wc == 0x0DCA)
		   && this_wc >= 0x0D9A
		   && this_wc <= 0x0DC6)
	    {
	      not_cursor_position(&attrs[i]);
	      is_conjunct = FALSE;
	    }
	  /*
	   * Consonant clusters do NOT result in implicit conjuncts
	   * in SINHALA orthography.
	   */
	  else if (!is_conjunct && prev_wc == 0x0DCA && this_wc != 0x200D)
	    {
	      attrs[i].is_cursor_position = TRUE;
	    }

	  break;

	default:

	  if (prev_wc != 0 && (this_wc == 0x200D || this_wc == 0x200C))
	    {
	      not_cursor_position(&attrs[i]);
	      if (next_wc != 0)
		{
		  not_cursor_position(&attrs[i+1]);
		  if ((next_next_wc != 0) &&
		       (next_wc == 0x09CD ||	/* Bengali */
			next_wc == 0x0ACD ||	/* Gujarati */
			next_wc == 0x094D ||	/* Hindi */
			next_wc == 0x0CCD ||	/* Kannada */
			next_wc == 0x0D4D ||	/* Malayalam */
			next_wc == 0x0B4D ||	/* Oriya */
			next_wc == 0x0A4D ||	/* Punjabi */
			next_wc == 0x0BCD ||	/* Tamil */
			next_wc == 0x0C4D))	/* Telugu */
		    {
		      not_cursor_position(&attrs[i+2]);
		    }
		}
	    }

	  break;
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
