/* Pango2
 *
 * Copyright (C) 2021 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <math.h>

#include <gio/gio.h>

#include "pangofc-fontmap.h"
#include "pango-hbfamily-private.h"
#include "pango-generic-family-private.h"
#include "pango-fontmap-private.h"
#include "pango-hbface-private.h"
#include "pango-hbfont-private.h"
#include "pango-context.h"
#include "pango-impl-utils.h"
#include "pango-trace-private.h"
#include "pangofc-language-set-private.h"

#include <fontconfig/fontconfig.h>
#include <hb-ot.h>


/**
 * Pango2FcFontMap:
 *
 * `Pango2FcFontMap` is a subclass of `Pango2FontMap` that uses
 * fontconfig to populate the fontmap with the available fonts.
 */


struct _Pango2FcFontMap
{
  Pango2FontMap parent_instance;

  FcConfig *config;
};

struct _Pango2FcFontMapClass
{
  Pango2FontMapClass parent_class;
};

/* {{{ Fontconfig utilities */

static gboolean
is_supported_font_format (FcPattern *pattern)
{
  FcResult res;
  const char *fontformat;
  const char *file;

  /* Harfbuzz loads woff fonts, but we don't get any glyphs */
  res = FcPatternGetString (pattern, FC_FILE, 0, (FcChar8 **)(void*)&file);
  if (res == FcResultMatch &&
      (g_str_has_suffix (file, ".woff") || g_str_has_suffix (file, ".woff2")))
    return FALSE;

  res = FcPatternGetString (pattern, FC_FONTFORMAT, 0, (FcChar8 **)(void*)&fontformat);
  if (res != FcResultMatch)
    return FALSE;

  /* Harfbuzz supports only SFNT fonts */

  /* FIXME: "CFF" is used for both CFF in OpenType and bare CFF files, but
   * HarfBuzz does not support the later and FontConfig does not seem
   * to have a way to tell them apart.
   */
  if (g_ascii_strcasecmp (fontformat, "TrueType") == 0 ||
      g_ascii_strcasecmp (fontformat, "CFF") == 0)
    return TRUE;

  return FALSE;
}

static Pango2Style
convert_fc_slant_to_pango (int fc_style)
{
  switch (fc_style)
    {
    case FC_SLANT_ROMAN:
      return PANGO2_STYLE_NORMAL;
    case FC_SLANT_ITALIC:
      return PANGO2_STYLE_ITALIC;
    case FC_SLANT_OBLIQUE:
      return PANGO2_STYLE_OBLIQUE;
    default:
      return PANGO2_STYLE_NORMAL;
    }
}

static Pango2Stretch
convert_fc_width_to_pango (int fc_stretch)
{
  switch (fc_stretch)
    {
    case FC_WIDTH_NORMAL:
      return PANGO2_STRETCH_NORMAL;
    case FC_WIDTH_ULTRACONDENSED:
      return PANGO2_STRETCH_ULTRA_CONDENSED;
    case FC_WIDTH_EXTRACONDENSED:
      return PANGO2_STRETCH_EXTRA_CONDENSED;
    case FC_WIDTH_CONDENSED:
      return PANGO2_STRETCH_CONDENSED;
    case FC_WIDTH_SEMICONDENSED:
      return PANGO2_STRETCH_SEMI_CONDENSED;
    case FC_WIDTH_SEMIEXPANDED:
      return PANGO2_STRETCH_SEMI_EXPANDED;
    case FC_WIDTH_EXPANDED:
      return PANGO2_STRETCH_EXPANDED;
    case FC_WIDTH_EXTRAEXPANDED:
      return PANGO2_STRETCH_EXTRA_EXPANDED;
    case FC_WIDTH_ULTRAEXPANDED:
      return PANGO2_STRETCH_ULTRA_EXPANDED;
    default:
      return PANGO2_STRETCH_NORMAL;
    }
}

static Pango2Weight
convert_fc_weight_to_pango (double fc_weight)
{
  return FcWeightToOpenTypeDouble (fc_weight);
}

#define PANGO2_FC_GRAVITY "gravity"

/* Create font description that matches the pattern.
 * We explicitly don't want to include variant, gravity,
 * variations and font features here, since there are
 * handled on the font level, by Pango2HbFont, not
 * by Pango2HbFace.
 */
static Pango2FontDescription *
pango2_font_description_from_pattern (FcPattern *pattern)
{
  Pango2FontDescription *desc;
  Pango2Style style;
  Pango2Weight weight;
  Pango2Stretch stretch;
  FcChar8 *s;
  int i;
  double d;
  FcResult res;

  desc = pango2_font_description_new ();

  res = FcPatternGetString (pattern, FC_FAMILY, 0, (FcChar8 **) &s);
  g_assert (res == FcResultMatch);

  pango2_font_description_set_family (desc, (char *)s);

  if (FcPatternGetInteger (pattern, FC_SLANT, 0, &i) == FcResultMatch)
    style = convert_fc_slant_to_pango (i);
  else
    style = PANGO2_STYLE_NORMAL;

  pango2_font_description_set_style (desc, style);

  if (FcPatternGetDouble (pattern, FC_WEIGHT, 0, &d) == FcResultMatch)
    weight = convert_fc_weight_to_pango (d);
  else
    weight = PANGO2_WEIGHT_NORMAL;

  pango2_font_description_set_weight (desc, weight);

  if (FcPatternGetInteger (pattern, FC_WIDTH, 0, &i) == FcResultMatch)
    stretch = convert_fc_width_to_pango (i);
  else
    stretch = PANGO2_STRETCH_NORMAL;

  pango2_font_description_set_stretch (desc, stretch);

  return desc;
}

static const char *
style_name_from_pattern (FcPattern *pattern)
{
  const char *font_style;
  int weight, slant;

  if (FcPatternGetString (pattern, FC_STYLE, 0, (FcChar8 **)(void*)&font_style) == FcResultMatch)
    return font_style;

  if (FcPatternGetInteger(pattern, FC_WEIGHT, 0, &weight) != FcResultMatch)
    weight = FC_WEIGHT_MEDIUM;

  if (FcPatternGetInteger(pattern, FC_SLANT, 0, &slant) != FcResultMatch)
    slant = FC_SLANT_ROMAN;

  if (weight <= FC_WEIGHT_MEDIUM)
    {
      if (slant == FC_SLANT_ROMAN)
        return "Regular";
      else
        return "Italic";
    }
  else
    {
      if (slant == FC_SLANT_ROMAN)
        return "Bold";
      else
        return "Bold Italic";
    }

  return "Normal";
}

static gboolean
font_matrix_from_pattern (FcPattern    *pattern,
                          Pango2Matrix *matrix)
{
  FcMatrix fc_matrix, *fc_matrix_val;
  int i;

  FcMatrixInit (&fc_matrix);
  for (i = 0; FcPatternGetMatrix (pattern, FC_MATRIX, i, &fc_matrix_val) == FcResultMatch; i++)
    FcMatrixMultiply (&fc_matrix, &fc_matrix, fc_matrix_val);
  FcMatrixScale (&fc_matrix, 1, -1);

  if (i > 0)
    {
      matrix->xx = fc_matrix.xx;
      matrix->yx = fc_matrix.yx;
      matrix->xy = fc_matrix.xy;
      matrix->yy = fc_matrix.yy;
      return TRUE;
    }

  return FALSE;
}

static Pango2HbFace *
pango2_hb_face_from_pattern (Pango2FcFontMap *self,
                             FcPattern       *pattern,
                             GHashTable      *lang_hash)
{
  const char *family_name, *file;
  int index;
  int instance_id;
  Pango2FontDescription *description;
  const char *name;
  Pango2HbFace *face;
  Pango2Matrix font_matrix;
  FcLangSet *langs;
  gboolean variable;

  if (!is_supported_font_format (pattern))
    return NULL;

  if (FcPatternGetString (pattern, FC_FAMILY, 0, (FcChar8 **)(void*)&family_name) != FcResultMatch)
    return NULL;

  if (FcPatternGetString (pattern, FC_FILE, 0, (FcChar8 **)(void*)&file) != FcResultMatch)
    return NULL;

  if (FcPatternGetInteger (pattern, FC_INDEX, 0, &index) != FcResultMatch)
    index = 0;

  if (FcPatternGetBool (pattern, FC_VARIABLE, 0, &variable) != FcResultMatch)
    variable = FALSE;

  instance_id = (index >> 16) - 1;
  index = index & 0xffff;

  if (variable)
    instance_id = -2;

  description = pango2_font_description_from_pattern (pattern);
  name = style_name_from_pattern (pattern);

  face = pango2_hb_face_new_from_file (file, index, instance_id, name, description);

  pango2_font_description_free (description);

  if (font_matrix_from_pattern (pattern, &font_matrix))
    pango2_hb_face_set_matrix (face, &font_matrix);

  if (FcPatternGetLangSet (pattern, FC_LANG, 0, &langs) == FcResultMatch)
    {
      Pango2FcLanguageSet lookup;
      Pango2LanguageSet *languages;
      FcLangSet *empty = FcLangSetCreate ();

      if (!FcLangSetEqual (langs, empty))
        {
          lookup.langs = langs;
          languages = g_hash_table_lookup (lang_hash, &lookup);
          if (!languages)
            {
              languages = PANGO2_LANGUAGE_SET (pango2_fc_language_set_new (langs));
              g_hash_table_add (lang_hash, languages);
            }
          pango2_hb_face_set_language_set (face, languages);
        }

      FcLangSetDestroy (empty);
    }

  return face;
}

/* }}} */
/* {{{ Pango2FontMap implementation */

static void
pango2_fc_font_map_populate (Pango2FontMap *map)
{
  Pango2FcFontMap *self = PANGO2_FC_FONT_MAP (map);
  FcPattern *pat;
  FcFontSet *fontset;
  int k;
  GHashTable *lang_hash;
  FcObjectSet *os;
  gint64 before G_GNUC_UNUSED;

  before = PANGO2_TRACE_CURRENT_TIME;

  os = FcObjectSetBuild (FC_FAMILY, FC_SPACING, FC_STYLE, FC_WEIGHT,
                         FC_WIDTH, FC_SLANT, FC_VARIABLE, FC_FONTFORMAT,
                         FC_FILE, FC_INDEX, FC_LANG, NULL);

  pat = FcPatternCreate ();
  fontset = FcFontList (self->config, pat, os);
  FcPatternDestroy (pat);

  lang_hash = g_hash_table_new_full ((GHashFunc) pango2_fc_language_set_hash,
                                     (GEqualFunc) pango2_fc_language_set_equal,
                                     NULL, g_object_unref);

  for (k = 0; k < fontset->nfont; k++)
    {
      Pango2HbFace *face = pango2_hb_face_from_pattern (self, fontset->fonts[k], lang_hash);
      if (!face)
        continue;

      pango2_font_map_add_face (PANGO2_FONT_MAP (self), PANGO2_FONT_FACE (face));
    }

  g_hash_table_unref (lang_hash);

  FcFontSetDestroy (fontset);

  /* Add aliases */
  const char *alias_names[] = {
    "System-ui",
    "Emoji"
  };

  for (int i = 0; i < G_N_ELEMENTS (alias_names); i++)
    {
      FcPattern *pattern;
      FcPattern *ret;
      FcResult res;
      const char *family_name;

      if (pango2_font_map_get_family (map, alias_names[i]))
        continue;

      pattern = FcPatternCreate ();
      FcPatternAddString (pattern, FC_FAMILY, (FcChar8 *) alias_names[i]);

      FcConfigSubstitute (self->config, pattern, FcMatchPattern);
      FcDefaultSubstitute (pattern);

      ret = FcFontMatch (self->config, pattern, &res);

      if (FcPatternGetString (ret, FC_FAMILY, 0, (FcChar8 **)(void*)&family_name) == FcResultMatch)
        {
          Pango2FontFamily *family = pango2_font_map_get_family (map, family_name);
          if (family)
            {
              Pango2GenericFamily *alias_family;

              alias_family = pango2_generic_family_new (alias_names[i]);
              pango2_generic_family_add_family (alias_family, family);
              pango2_font_map_add_family (map, PANGO2_FONT_FAMILY (alias_family));
            }
        }

      FcPatternDestroy (ret);
      FcPatternDestroy (pattern);
    }

  /* Add generic aliases. Unfortunately, we need both sans-serif and sans,
   * since the old fontconfig backend had 'Sans', and fontconfig itself
   * has 'sans-serif'
   */
  const char *generic_names[] = {
    "Monospace",
    "Sans-serif",
    "Sans",
    "Serif",
    "Cursive",
    "Fantasy",
  };
  FcLangSet *no_langs = FcLangSetCreate ();
  FcLangSet *emoji_langs = FcLangSetCreate ();

  FcLangSetAdd (emoji_langs, (FcChar8 *)"und-zsye");

  for (int i = 0; i < G_N_ELEMENTS (generic_names); i++)
    {
      Pango2GenericFamily *generic_family;
      GHashTable *families_hash;
      FcPattern *pattern;
      FcFontSet *ret;
      FcResult res;

      if (pango2_font_map_get_family (map, generic_names[i]))
        continue;

      generic_family = pango2_generic_family_new (generic_names[i]);

      families_hash = g_hash_table_new (NULL, NULL);

      pattern = FcPatternCreate ();
      FcPatternAddString (pattern, FC_FAMILY, (FcChar8 *) generic_names[i]);

      FcConfigSubstitute (self->config, pattern, FcMatchPattern);
      FcDefaultSubstitute (pattern);

      ret = FcFontSort (self->config, pattern, TRUE, NULL, &res);
      for (int j = 0; j < ret->nfont; j++)
        {
          Pango2HbFamily *family;
          const char *file;
          int index;
          FcLangSet *langs;
          int spacing;
          const char *family_name;

          pat = ret->fonts[j];

          if (!is_supported_font_format (pat))
            continue;

          if (FcPatternGetLangSet (pat, FC_LANG, 0, &langs) != FcResultMatch)
            continue;

          if (FcLangSetEqual (langs, no_langs))
            continue;

          if ((strcmp (generic_names[i], "emoji") == 0) != FcLangSetEqual (langs, emoji_langs))
            continue;

          if (FcPatternGetInteger (pat, FC_SPACING, 0, &spacing) != FcResultMatch)
            spacing = FC_PROPORTIONAL;

          if (strcmp (generic_names[i], "monospace") == 0 && spacing != FC_MONO)
            continue;

          if (FcPatternGetString (pat, FC_FAMILY, 0, (FcChar8 **)(void*)&family_name) != FcResultMatch)
            continue;

          if (FcPatternGetString (pat, FC_FILE, 0, (FcChar8 **)(void*)&file) != FcResultMatch)
            continue;

          if (FcPatternGetInteger (pat, FC_INDEX, 0, &index) != FcResultMatch)
            index = 0;

          index = index & 0xffff;

          family = PANGO2_HB_FAMILY (pango2_font_map_get_family (map, family_name));
          if (!family)
            continue;

          if (g_hash_table_contains (families_hash, family))
            continue;

          pango2_generic_family_add_family (generic_family, PANGO2_FONT_FAMILY (family));
          g_hash_table_add (families_hash, family);
       }

      FcFontSetDestroy (ret);
      FcPatternDestroy (pattern);
      g_hash_table_unref (families_hash);

      if (g_list_model_get_n_items (G_LIST_MODEL (generic_family)) > 0)
        pango2_font_map_add_family (map, PANGO2_FONT_FAMILY (generic_family));
      else
        g_object_unref (generic_family);
    }

  FcLangSetDestroy (no_langs);
  FcLangSetDestroy (emoji_langs);

  FcObjectSetDestroy (os);

  pango2_trace_mark (before, "populate FcFontMap", NULL);
}

/* }}} */
/* {{{ Pango2FcFontMap implementation */

G_DEFINE_FINAL_TYPE (Pango2FcFontMap, pango2_fc_font_map, PANGO2_TYPE_FONT_MAP)

static void
pango2_fc_font_map_init (Pango2FcFontMap *self)
{
  pango2_font_map_repopulate (PANGO2_FONT_MAP (self), TRUE);
}

static void
pango2_fc_font_map_finalize (GObject *object)
{
  Pango2FcFontMap *self = PANGO2_FC_FONT_MAP (object);

  if (self->config)
    FcConfigDestroy (self->config);

  G_OBJECT_CLASS (pango2_fc_font_map_parent_class)->finalize (object);
}

static void
pango2_fc_font_map_class_init (Pango2FcFontMapClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  Pango2FontMapClass *font_map_class = PANGO2_FONT_MAP_CLASS (class);
  gint64 before G_GNUC_UNUSED;

  before = PANGO2_TRACE_CURRENT_TIME;

  FcInit ();

  pango2_trace_mark (before, "FcInit", NULL);

  object_class->finalize = pango2_fc_font_map_finalize;

  font_map_class->populate = pango2_fc_font_map_populate;
}

/* }}} */
/* {{{ Public API */

/**
 * pango2_fc_font_map_new:
 *
 * Creates a new `Pango2FcFontMap` using the default
 * fontconfig configuration.
 *
 * Returns: a new `Pango2FcFontMap`
 */
Pango2FcFontMap *
pango2_fc_font_map_new (void)
{
  return g_object_new (PANGO2_TYPE_FC_FONT_MAP, NULL);
}

/**
 * pango2_fc_font_map_set_config: (skip)
 * @self: a `Pango2FcFontMap`
 * @config: (nullable): the `FcConfig` to use, or `NULL` to use
 *   the default configuration
 *
 * Sets the fontconfig configuration to use.
 *
 * Note that changing the fontconfig configuration
 * removes all cached font families, faces and fonts.
 */
void
pango2_fc_font_map_set_config (Pango2FcFontMap *self,
                               FcConfig        *config)
{
  g_return_if_fail (PANGO2_IS_FC_FONT_MAP (self));

  if (self->config == config && FcConfigUptoDate (config))
    return;

  if (self->config != config)
    {
      if (self->config)
        FcConfigDestroy (self->config);

      self->config = config;

      if (self->config)
        FcConfigReference (self->config);
    }

  pango2_font_map_repopulate (PANGO2_FONT_MAP (self), TRUE);
}

/**
 * pango2_fc_font_map_get_config: (skip)
 * @self: a `Pango2FcFontMap`
 *
 * Fetches the fontconfig configuration of the fontmap.
 *
 * See also: [method@Pango2Fc.FontMap.set_config].
 *
 * Returns: (nullable): the `FcConfig` object attached to
 *   @self, which might be `NULL`. The return value is
 *   owned by Pango2 and should not be freed.
 */
FcConfig *
pango2_fc_font_map_get_config (Pango2FcFontMap *self)
{
  g_return_val_if_fail (PANGO2_IS_FC_FONT_MAP (self), NULL);

  return self->config;
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
