/* Pango
 * indic-xft.c:
 *
 * Copyright (C) 2001, 2002 IBM Corporation
 * Author: Eric Mader <mader@jtcsv.com>
 * Based on arabic-xft.c by Owen Taylor <otaylor@redhat.com>
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

#include "indic-ot.h"

#include "pango-engine.h"
#include "pango-ot.h"
#include "pango-utils.h"
#include "pangofc-font.h"

typedef struct _PangoIndicInfo PangoIndicInfo;

typedef struct _IndicEngineFc IndicEngineFc;
typedef PangoEngineShapeClass IndicEngineFcClass ; /* No extra fields needed */

struct _IndicEngineFc
{
  PangoEngineShape shapeEngine;
  const IndicOTClassTable *classTable;
};

#define ENGINE_SUFFIX "ScriptEngineFc"
#define RENDER_TYPE PANGO_RENDER_TYPE_FC

#define INDIC_ENGINE_INFO(script) {#script ENGINE_SUFFIX, PANGO_ENGINE_TYPE_SHAPE, RENDER_TYPE, script##_scripts, G_N_ELEMENTS(script##_scripts)}

#define INDIC_OT_CLASS_TABLE(script) &script##_class_table

static PangoEngineScriptInfo deva_scripts[] = {
  { PANGO_SCRIPT_DEVANAGARI, "*" }
};

static PangoEngineScriptInfo beng_scripts[] = {
  {PANGO_SCRIPT_BENGALI, "*" }
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

/*
 * WARNING: These entries need to be in the same order as the entries
 * in script_engines[].
 *
 * FIXME: remove this requirement, either by encapsulating the order
 * in a macro that calls a body macro that can be redefined, or by
 * putting the pointers to the PangoEngineInfo in PangoIndicInfo...
 */
static const IndicOTClassTable *indic_ot_class_tables[] = {
  INDIC_OT_CLASS_TABLE(deva), INDIC_OT_CLASS_TABLE(beng), INDIC_OT_CLASS_TABLE(guru),
  INDIC_OT_CLASS_TABLE(gujr), INDIC_OT_CLASS_TABLE(orya), INDIC_OT_CLASS_TABLE(taml),
  INDIC_OT_CLASS_TABLE(telu), INDIC_OT_CLASS_TABLE(knda), INDIC_OT_CLASS_TABLE(mlym),
  INDIC_OT_CLASS_TABLE(sinh)
};

static const PangoOTFeatureMap gsub_features[] =
{
  {"ccmp", PANGO_OT_ALL_GLYPHS},
  {"locl", PANGO_OT_ALL_GLYPHS},
  {"init", init},
  {"nukt", nukt},
  {"akhn", akhn},
  {"rphf", rphf},
  {"blwf", blwf},
  {"half", half},
  {"pstf", pstf},
  {"vatu", vatu},
  {"pres", pres},
  {"blws", blws},
  {"abvs", abvs},
  {"psts", psts},
  {"haln", haln},
  {"calt", PANGO_OT_ALL_GLYPHS}
};

static const PangoOTFeatureMap gpos_features[] =
{
  {"blwm", blwm},
  {"abvm", abvm},
  {"dist", dist},
  {"kern", PANGO_OT_ALL_GLYPHS},
  {"mark", PANGO_OT_ALL_GLYPHS},
  {"mkmk", PANGO_OT_ALL_GLYPHS}
};

static void
set_glyphs (PangoFont      *font,
	    const gunichar *wcs,
	    gulong         *tags,
	    glong           n_glyphs,
	    PangoOTBuffer  *buffer,
	    gboolean        process_zwj)
{
  gint i;
  PangoFcFont *fc_font;

  g_assert (font);

  fc_font = PANGO_FC_FONT (font);

  for (i = 0; i < n_glyphs; i++)
    {
      guint glyph;

      if (pango_is_zero_width (wcs[i]) &&
	  (!process_zwj || wcs[i] != 0x200D))
	glyph = PANGO_GLYPH_EMPTY;
      else
	{
	  glyph = pango_fc_font_get_glyph (fc_font, wcs[i]);

	  if (!glyph)
	    glyph = PANGO_GET_UNKNOWN_GLYPH ( wcs[i]);
	}
      pango_ot_buffer_add_glyph (buffer, glyph, tags[i], i);
    }
}

/*
 * FIXME: should this check for null pointers, etc.?
 */
static gunichar *
expand_text(const gchar *text, glong length, glong **offsets, glong *n_chars)
{
  const gchar *p;
  gunichar *wcs, *wco;
  glong i, *oo;

  *n_chars = g_utf8_strlen (text, length);
  wcs      = g_new (gunichar, *n_chars);
  *offsets = g_new (glong, *n_chars + 1);

  p   = text;
  wco = wcs;
  oo  = *offsets;
  for (i = 0; i < *n_chars; i++)
    {
      *wco++ = g_utf8_get_char (p);
      *oo++  = p - text;

      p = g_utf8_next_char (p);
    }

  *oo = p - text;

  return wcs;
}


/* analysis->shape_engine has the PangoEngine... */
static void
indic_engine_shape (PangoEngineShape *engine,
		    PangoFont        *font,
		    const char       *text,
		    gint              length,
		    const PangoAnalysis *analysis,
		    PangoGlyphString *glyphs)
{
  PangoFcFont *fc_font;
  FT_Face face;
  PangoOTRulesetDescription desc;
  const PangoOTRuleset *ruleset;
  PangoOTBuffer *buffer;
  glong i, n_chars, n_glyphs;
  gulong *tags = NULL;
  gunichar *wc_in = NULL, *wc_out = NULL;
  glong *utf8_offsets = NULL;
  glong *indices = NULL;
  IndicEngineFc *indic_shape_engine = NULL;
  MPreFixups *mprefixups;

  g_return_if_fail (font != NULL);
  g_return_if_fail (text != NULL);
  g_return_if_fail (length >= 0);
  g_return_if_fail (analysis != NULL);

  fc_font = PANGO_FC_FONT (font);
  face = pango_fc_font_lock_face (fc_font);
  if (!face)
    return;

  indic_shape_engine = (IndicEngineFc *) engine;

  wc_in    = expand_text (text, length, &utf8_offsets, &n_chars);

  n_glyphs = indic_ot_reorder (wc_in, utf8_offsets, n_chars, indic_shape_engine->classTable, NULL, NULL, NULL, NULL);

  wc_out  = g_new (gunichar, n_glyphs);
  indices = g_new (glong,    n_glyphs);
  tags    = g_new (gulong,   n_glyphs);

  n_glyphs  = indic_ot_reorder (wc_in, utf8_offsets, n_chars, indic_shape_engine->classTable, wc_out, indices, tags, &mprefixups);

  pango_glyph_string_set_size (glyphs, n_glyphs);
  buffer = pango_ot_buffer_new (fc_font);

  set_glyphs(font, wc_out, tags, n_glyphs, buffer,
	     (indic_shape_engine->classTable->scriptFlags & SF_PROCESS_ZWJ) != 0);

  desc.script = analysis->script;
  desc.language = analysis->language;

  desc.n_static_gsub_features = G_N_ELEMENTS (gsub_features);
  desc.static_gsub_features = gsub_features;
  desc.n_static_gpos_features = G_N_ELEMENTS (gpos_features);
  desc.static_gpos_features = gpos_features;

  /* TODO populate other_features from analysis->extra_attrs */
  desc.n_other_features = 0;
  desc.other_features = NULL;

  ruleset = pango_ot_ruleset_get_for_description (pango_ot_info_get (face), &desc);

  /* do gsub processing */
  pango_ot_ruleset_substitute (ruleset, buffer);

  /* Fix pre-modifiers for some scripts before base consonant */
  if (mprefixups)
    {
      indic_mprefixups_apply (mprefixups, buffer);
      indic_mprefixups_free (mprefixups);
    }

  /* do gpos processing */
  pango_ot_ruleset_position (ruleset, buffer);

  pango_ot_buffer_output (buffer, glyphs);

  /* Get the right log_clusters values */
  for (i = 0; i < glyphs->num_glyphs; i += 1)
    glyphs->log_clusters[i] = indices[glyphs->log_clusters[i]];

  pango_fc_font_unlock_face (fc_font);

  pango_ot_buffer_destroy (buffer);
  g_free (tags);
  g_free (indices);
  g_free (wc_out);
  g_free (wc_in);
  g_free (utf8_offsets);
}

static void
indic_engine_fc_class_init (PangoEngineShapeClass *class)
{
  class->script_shape = indic_engine_shape;
}

PANGO_ENGINE_SHAPE_DEFINE_TYPE (IndicEngineFc, indic_engine_fc,
				indic_engine_fc_class_init, NULL)

void
PANGO_MODULE_ENTRY(init) (GTypeModule *module)
{
  indic_engine_fc_register_type (module);
}

void
PANGO_MODULE_ENTRY(exit) (void)
{
}

void
PANGO_MODULE_ENTRY(list) (PangoEngineInfo **engines,
			  int              *n_engines)
{
  *engines = script_engines;
  *n_engines = G_N_ELEMENTS (script_engines);
}

PangoEngine *
PANGO_MODULE_ENTRY(create) (const char *id)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS(script_engines); i += 1)
    {
      if (!strcmp(id, script_engines[i].id))
	{
	  IndicEngineFc *engine = g_object_new (indic_engine_fc_type, NULL);
	  engine->classTable = indic_ot_class_tables[i];

	  return (PangoEngine *)engine;
	}
    }

  return NULL;
}
