/* Pango
 * thai-lang.c:
 *
 * Copyright (C) 2003 Theppitak Karoonboonyanan <thep@linux.thai.net>
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */


#include <string.h>
#include <glib.h>
#include <pango/pango-engine.h>
#include <pango/pango-break.h>
#include <thai/thwchar.h>
#include <thai/thbrk.h>

/* No extra fields needed */
typedef PangoEngineLang      ThaiEngineLang;
typedef PangoEngineLangClass ThaiEngineLangClass;

#define SCRIPT_ENGINE_NAME "ThaiScriptEngineLang"

static PangoEngineScriptInfo thai_scripts[] = {
  { PANGO_SCRIPT_THAI, "*" }
};

static PangoEngineInfo script_engines[] = {
  {
    SCRIPT_ENGINE_NAME,
    PANGO_ENGINE_TYPE_LANG,
    PANGO_RENDER_TYPE_NONE,
    thai_scripts, G_N_ELEMENTS(thai_scripts)
  }
};

/*
 * tis_text is assumed to be large enough to hold the converted string,
 * i.e. it must be at least g_utf8_strlen(text, len)+1 bytes.
 */
static thchar_t *
utf8_to_tis (const char *text, int len, thchar_t *tis_text, int *tis_cnt)
{
  thchar_t   *tis_p;
  const char *text_p;

  tis_p = tis_text;
  for (text_p = text; text_p < text + len; text_p = g_utf8_next_char (text_p))
    *tis_p++ = th_uni2tis (g_utf8_get_char (text_p));
  *tis_p++ = '\0';

  *tis_cnt = tis_p - tis_text;
  return tis_text;
}

static void
thai_engine_break (PangoEngineLang *engine G_GNUC_UNUSED,
		   const char      *text,
		   int              len,
		   PangoAnalysis   *analysis G_GNUC_UNUSED,
		   PangoLogAttr    *attrs,
		   int              attrs_len G_GNUC_UNUSED)
{
  thchar_t tis_stack[512];
  int brk_stack[512];
  thchar_t *tis_text;
  int *brk_pnts;
  int cnt;

  if (len < 0)
    len = strlen (text);
  cnt = g_utf8_strlen (text, len) + 1;

  tis_text = tis_stack;
  if (cnt > (int) G_N_ELEMENTS (tis_stack))
    tis_text = g_new (thchar_t, cnt);

  utf8_to_tis (text, len, tis_text, &cnt);

  brk_pnts = brk_stack;
  if (cnt > (int) G_N_ELEMENTS (brk_stack))
    brk_pnts = g_new (int, cnt);

  /* find line break positions */
  len = th_brk (tis_text, brk_pnts, len);
  for (cnt = 0; cnt < len; cnt++)
    {
      attrs[brk_pnts[cnt]].is_line_break = TRUE;
      attrs[brk_pnts[cnt]].is_word_start = TRUE;
      attrs[brk_pnts[cnt]].is_word_end = TRUE;
    }

  if (brk_pnts != brk_stack)
    g_free (brk_pnts);

  if (tis_text != tis_stack)
    g_free (tis_text);
}

static void
thai_engine_lang_class_init (PangoEngineLangClass *class)
{
  class->script_break = thai_engine_break;
}

PANGO_ENGINE_LANG_DEFINE_TYPE (ThaiEngineLang, thai_engine_lang,
			       thai_engine_lang_class_init, NULL);

void
PANGO_MODULE_ENTRY(init) (GTypeModule *module)
{
  thai_engine_lang_register_type (module);
}

void
PANGO_MODULE_ENTRY(exit) (void)
{
}

void
PANGO_MODULE_ENTRY(list) (PangoEngineInfo **engines, gint *n_engines)
{
  *engines = script_engines;
  *n_engines = G_N_ELEMENTS (script_engines);
}

/* Load a particular engine given the ID for the engine
 */
PangoEngine *
PANGO_MODULE_ENTRY(create) (const char *id)
{
  if (!strcmp (id, SCRIPT_ENGINE_NAME))
    return g_object_new (thai_engine_lang_type, NULL);
  else
    return NULL;
}

