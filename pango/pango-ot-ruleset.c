/* Pango
 * pango-ot-ruleset.c: Shaping using OpenType features
 *
 * Copyright (C) 2000 Red Hat Software
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

#include "pango-ot-private.h"
#include "pango-impl-utils.h"

typedef struct _PangoOTRule PangoOTRule;

struct _PangoOTRule
{
  gulong     property_bit;
  HB_UShort  feature_index;
  guint      table_type : 1;
};

static void pango_ot_ruleset_class_init (GObjectClass   *object_class);
static void pango_ot_ruleset_init       (PangoOTRuleset *ruleset);
static void pango_ot_ruleset_finalize   (GObject        *object);

static GObjectClass *parent_class;

GType
pango_ot_ruleset_get_type (void)
{
  static GType object_type = 0;

  if (G_UNLIKELY (!object_type))
    {
      const GTypeInfo object_info =
      {
	sizeof (PangoOTRulesetClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc)pango_ot_ruleset_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data */
	sizeof (PangoOTRuleset),
	0,              /* n_preallocs */
	(GInstanceInitFunc)pango_ot_ruleset_init,
	NULL            /* value_table */
      };

      object_type = g_type_register_static (G_TYPE_OBJECT,
					    I_("PangoOTRuleset"),
					    &object_info, 0);
    }

  return object_type;
}

static void
pango_ot_ruleset_class_init (GObjectClass *object_class)
{
  parent_class = g_type_class_peek_parent (object_class);

  object_class->finalize = pango_ot_ruleset_finalize;
}

static void
pango_ot_ruleset_init (PangoOTRuleset *ruleset)
{
  ruleset->rules = g_array_new (FALSE, FALSE, sizeof (PangoOTRule));
  ruleset->script_index[0]   = PANGO_OT_NO_SCRIPT;
  ruleset->script_index[1]   = PANGO_OT_NO_SCRIPT;
  ruleset->language_index[0] = PANGO_OT_DEFAULT_LANGUAGE;
  ruleset->language_index[1] = PANGO_OT_DEFAULT_LANGUAGE;
}

static void
pango_ot_ruleset_finalize (GObject *object)
{
  PangoOTRuleset *ruleset = PANGO_OT_RULESET (object);

  g_array_free (ruleset->rules, TRUE);
  if (ruleset->info)
    g_object_remove_weak_pointer (G_OBJECT (ruleset->info), (gpointer *) &ruleset->info);

  parent_class->finalize (object);
}

/**
 * pango_ot_ruleset_get_for_description:
 * @info: a #PangoOTInfo.
 * @desc: a #PangoOTRulesetDescription.
 *
 * Returns a ruleset for the given OpenType info and ruleset
 * description.  Rulesets are created on demand using
 * pango_ot_ruleset_new_from_description().
 * The returned ruleset should not be modified or destroyed.
 *
 * The static feature map members of @desc should be alive as
 * long as @info is.
 *
 * Return value: the #PangoOTRuleset for @desc. This object will have
 * the same lifetime as @info.
 *
 * Since: 1.18
 **/
G_CONST_RETURN PangoOTRuleset *
pango_ot_ruleset_get_for_description (PangoOTInfo                     *info,
				      const PangoOTRulesetDescription *desc)
{
  PangoOTRuleset *ruleset;
  static GQuark rulesets_quark = 0;
  GHashTable *rulesets;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (desc != NULL, NULL);

  if (!rulesets_quark)
    rulesets_quark = g_quark_from_string ("pango-info-rulesets");

  rulesets = g_object_get_qdata (G_OBJECT (info), rulesets_quark);

  if (!rulesets)
    {
      rulesets = g_hash_table_new_full ((GHashFunc)  pango_ot_ruleset_description_hash,
					(GEqualFunc) pango_ot_ruleset_description_equal,
					(GDestroyNotify) pango_ot_ruleset_description_free,
					(GDestroyNotify) g_object_unref);

      g_object_set_qdata_full (G_OBJECT (info), rulesets_quark, rulesets, (GDestroyNotify) g_hash_table_destroy);
    }

  ruleset = g_hash_table_lookup (rulesets, desc);

  if (!ruleset)
    {
      ruleset = pango_ot_ruleset_new_from_description (info, desc);

      g_hash_table_insert (rulesets,
			   pango_ot_ruleset_description_copy (desc),
			   ruleset);
    }

  return ruleset;
}

/**
 * pango_ot_ruleset_new:
 * @info: a #PangoOTInfo.
 *
 * Creates a new #PangoOTRuleset for the given OpenType info.
 *
 * Return value: the newly allocated #PangoOTRuleset, which
 *               should be freed with g_object_unref().
 **/
PangoOTRuleset *
pango_ot_ruleset_new (PangoOTInfo *info)
{
  PangoOTRuleset *ruleset;

  g_return_val_if_fail (PANGO_IS_OT_INFO (info), NULL);

  ruleset = g_object_new (PANGO_TYPE_OT_RULESET, NULL);

  ruleset->info = info;
  g_object_add_weak_pointer (G_OBJECT (ruleset->info), (gpointer *) &ruleset->info);

  return ruleset;
}

/**
 * pango_ot_ruleset_new_for:
 * @info: a #PangoOTInfo.
 * @script: a #PangoScript.
 * @language: a #PangoLanguage.
 *
 * Creates a new #PangoOTRuleset for the given OpenType info, script, and
 * language.
 *
 * This function is part of a convenience scheme that highly simplifies
 * using a #PangoOTRuleset to represent features for a specific pair of script
 * and language.  So one can use this function passing in the script and
 * language of interest, and later try to add features to the ruleset by just
 * specifying the feature name or tag, without having to deal with finding
 * script, language, or feature indices manually.
 *
 * In excess to what pango_ot_ruleset_new() does, this function will:
 * <itemizedlist>
 *   <listitem>
 *   Find the #PangoOTTag script and language tags associated with
 *   @script and @language using pango_ot_tag_from_script() and
 *   pango_ot_tag_from_language(),
 *   </listitem>
 *   <listitem>
 *   For each of table types %PANGO_OT_TABLE_GSUB and %PANGO_OT_TABLE_GPOS,
 *   find the script index of the script tag found and the language
 *   system index of the language tag found in that script system, using
 *   pango_ot_info_find_script() and pango_ot_info_find_language(),
 *   </listitem>
 *   <listitem>
 *   For found language-systems, if they have required feature
 *   index, add that feature to the ruleset using
 *   pango_ot_ruleset_add_feature(),
 *   </listitem>
 *   <listitem>
 *   Remember found script and language indices for both table types,
 *   and use them in future pango_ot_ruleset_maybe_add_feature() and
 *   pango_ot_ruleset_maybe_add_features().
 *   </listitem>
 * </itemizedlist>
 *
 * Because of the way return values of pango_ot_info_find_script() and
 * pango_ot_info_find_language() are ignored, this function automatically
 * finds and uses the 'DFLT' script and the default language-system.
 *
 * Return value: the newly allocated #PangoOTRuleset, which
 *               should be freed with g_object_unref().
 *
 * Since: 1.18
 **/
PangoOTRuleset *
pango_ot_ruleset_new_for (PangoOTInfo       *info,
			  PangoScript        script,
			  PangoLanguage     *language)
{
  PangoOTRuleset *ruleset;
  PangoOTTag script_tag, language_tag;
  PangoOTTableType table_type;

  g_return_val_if_fail (PANGO_IS_OT_INFO (info), NULL);

  ruleset = pango_ot_ruleset_new (info);

  script_tag   = pango_ot_tag_from_script (script);
  language_tag = pango_ot_tag_from_language (language);

  for (table_type = PANGO_OT_TABLE_GSUB; table_type <= PANGO_OT_TABLE_GPOS; table_type++)
    {
      guint script_index, language_index, feature_index;

      pango_ot_info_find_script   (ruleset->info, table_type,
				   script_tag, &script_index);
      pango_ot_info_find_language (ruleset->info, table_type, script_index,
				   language_tag, &language_index,
				   &feature_index);

      ruleset->script_index[table_type] = script_index;
      ruleset->language_index[table_type] = language_index;

      /* add required feature of the language */
      pango_ot_ruleset_add_feature (ruleset, table_type,
				    feature_index, PANGO_OT_ALL_GLYPHS);
    }

  return ruleset;
}

/**
 * pango_ot_ruleset_new_from_description:
 * @info: a #PangoOTInfo.
 * @desc: a #PangoOTRulesetDescription.
 *
 * Creates a new #PangoOTRuleset for the given OpenType infor and
 * matching the given ruleset description.
 *
 * This is a convenience function that calls pango_ot_ruleset_new_for() and
 * adds the static GSUB/GPOS features to the resulting ruleset, followed by
 * adding other features to both GSUB and GPOS.
 *
 * The static feature map members of @desc should be alive as
 * long as @info is.
 *
 * Return value: the newly allocated #PangoOTRuleset, which
 *               should be freed with g_object_unref().
 *
 * Since: 1.18
 **/
PangoOTRuleset *
pango_ot_ruleset_new_from_description (PangoOTInfo                     *info,
				       const PangoOTRulesetDescription *desc)
{
  PangoOTRuleset *ruleset;

  g_return_val_if_fail (info != NULL, NULL);
  g_return_val_if_fail (desc != NULL, NULL);

  ruleset = pango_ot_ruleset_new_for (info,
				      desc->script,
				      desc->language);

  if (desc->n_static_gsub_features)
    pango_ot_ruleset_maybe_add_features (ruleset, PANGO_OT_TABLE_GSUB,
					 desc->static_gsub_features,
					 desc->n_static_gsub_features);
  if (desc->n_static_gpos_features)
    pango_ot_ruleset_maybe_add_features (ruleset, PANGO_OT_TABLE_GPOS,
					 desc->static_gpos_features,
					 desc->n_static_gpos_features);

  if (desc->n_other_features)
    {
      pango_ot_ruleset_maybe_add_features (ruleset, PANGO_OT_TABLE_GSUB,
					   desc->other_features,
					   desc->n_other_features);
      pango_ot_ruleset_maybe_add_features (ruleset, PANGO_OT_TABLE_GPOS,
					   desc->other_features,
					   desc->n_other_features);
    }

  return ruleset;
}

/**
 * pango_ot_ruleset_add_feature:
 * @ruleset: a #PangoOTRuleset.
 * @table_type: the table type to add a feature to.
 * @feature_index: the index of the feature to add.
 * @property_bit: the property bit to use for this feature. Used to identify
 *                the glyphs that this feature should be applied to, or
 *                %PANGO_OT_ALL_GLYPHS if it should be applied to all glyphs.
 *
 * Adds a feature to the ruleset.
 **/
void
pango_ot_ruleset_add_feature (PangoOTRuleset   *ruleset,
			      PangoOTTableType  table_type,
			      guint             feature_index,
			      gulong            property_bit)
{
  PangoOTRule tmp_rule;

  g_return_if_fail (PANGO_IS_OT_RULESET (ruleset));
  g_return_if_fail (ruleset->info != NULL);

  if (feature_index == PANGO_OT_NO_FEATURE)
    return;

  tmp_rule.table_type = table_type;
  tmp_rule.feature_index = feature_index;
  tmp_rule.property_bit = property_bit;

  g_array_append_val (ruleset->rules, tmp_rule);

  ruleset->n_features[table_type]++;
}

/**
 * pango_ot_ruleset_maybe_add_feature:
 * @ruleset: a #PangoOTRuleset.
 * @table_type: the table type to add a feature to.
 * @feature_tag: the tag of the feature to add.
 * @property_bit: the property bit to use for this feature. Used to identify
 *                the glyphs that this feature should be applied to, or
 *                %PANGO_OT_ALL_GLYPHS if it should be applied to all glyphs.
 *
 * This is a convenience function that first tries to find the feature
 * using pango_ot_info_find_feature() and the ruleset script and language
 * passed to pango_ot_ruleset_new_for(),
 * and if the feature is found, adds it to the ruleset.
 *
 * If @ruleset was not created using pango_ot_ruleset_new_for(), this function
 * does nothing.
 *
 * Return value: %TRUE if the feature was found and added to ruleset,
 *               %FALSE otherwise.
 *
 * Since: 1.18
 **/
gboolean
pango_ot_ruleset_maybe_add_feature (PangoOTRuleset          *ruleset,
				    PangoOTTableType         table_type,
				    PangoOTTag               feature_tag,
				    gulong                   property_bit)
{
  guint feature_index;

  g_return_val_if_fail (PANGO_IS_OT_RULESET (ruleset), FALSE);
  g_return_val_if_fail (ruleset->info != NULL, FALSE);

  pango_ot_info_find_feature (ruleset->info, table_type,
			      feature_tag,
			      ruleset->script_index[table_type],
			      ruleset->language_index[table_type],
			      &feature_index);

  if (feature_index != PANGO_OT_NO_FEATURE)
    {
      pango_ot_ruleset_add_feature (ruleset, table_type,
				    feature_index, property_bit);
      return TRUE;
    }

  return FALSE;
}

/**
 * pango_ot_ruleset_maybe_add_features:
 * @ruleset: a #PangoOTRuleset.
 * @table_type: the table type to add features to.
 * @features: array of feature name and property bits to add.
 * @n_features: number of feature records in @features array.
 *
 * This is a convenience function that 
 * for each feature in the feature map array @features
 * converts the feature name to a #PangoOTTag feature tag using PANGO_OT_TAG_MAKE()
 * and calls pango_ot_ruleset_maybe_add_feature() on it.
 *
 * Return value: The number of features in @features that were found
 *               and added to @ruleset.
 *
 * Since: 1.18
 **/
guint
pango_ot_ruleset_maybe_add_features (PangoOTRuleset          *ruleset,
				     PangoOTTableType         table_type,
				     const PangoOTFeatureMap *features,
				     guint                    n_features)
{
  guint i, n_found_features = 0;

  g_return_val_if_fail (PANGO_IS_OT_RULESET (ruleset), 0);
  g_return_val_if_fail (ruleset->info != NULL, 0);

  for (i = 0; i < n_features; i++)
    {
      PangoOTTag feature_tag = PANGO_OT_TAG_MAKE (features[i].feature_name[0],
						  features[i].feature_name[1],
						  features[i].feature_name[2],
						  features[i].feature_name[3]);

      n_found_features += pango_ot_ruleset_maybe_add_feature (ruleset,
							      table_type,
							      feature_tag,
							      features[i].property_bit);
    }

  return n_found_features;
}

/**
 * pango_ot_ruleset_get_feature_count:
 * @ruleset: a #PangoOTRuleset.
 * @n_gsub_features: location to store number of GSUB features, or %NULL.
 * @n_gpos_features: location to store number of GPOS features, or %NULL.
 *
 * Gets the number of GSUB and GPOS features in the ruleset.
 *
 * Return value: Total number of features in the @ruleset.
 *
 * Since: 1.18
 **/
guint
pango_ot_ruleset_get_feature_count (const PangoOTRuleset   *ruleset,
				    guint                  *n_gsub_features,
				    guint                  *n_gpos_features)
{
  g_return_val_if_fail (PANGO_IS_OT_RULESET (ruleset), 0);
  
  if (n_gsub_features)
    *n_gsub_features = ruleset->n_features[PANGO_OT_TABLE_GSUB];

  if (n_gpos_features)
    *n_gpos_features = ruleset->n_features[PANGO_OT_TABLE_GPOS];

  return ruleset->n_features[PANGO_OT_TABLE_GSUB] + ruleset->n_features[PANGO_OT_TABLE_GPOS];
}

/**
 * pango_ot_ruleset_substitute:
 * @ruleset: a #PangoOTRuleset.
 * @buffer: a #PangoOTBuffer.
 *
 * Performs the OpenType GSUB substitution on @buffer using the features
 * in @ruleset
 *
 * Since: 1.4
 **/
void
pango_ot_ruleset_substitute  (const PangoOTRuleset *ruleset,
			      PangoOTBuffer        *buffer)
{
  unsigned int i;

  HB_GSUB gsub = NULL;

  g_return_if_fail (PANGO_IS_OT_RULESET (ruleset));
  g_return_if_fail (ruleset->info != NULL);

  for (i = 0; i < ruleset->rules->len; i++)
    {
      PangoOTRule *rule = &g_array_index (ruleset->rules, PangoOTRule, i);

      if (rule->table_type != PANGO_OT_TABLE_GSUB)
	continue;

      if (!gsub)
	{
	  gsub = pango_ot_info_get_gsub (ruleset->info);

	  if (gsub)
	    HB_GSUB_Clear_Features (gsub);
	  else
	    return;
	}

      HB_GSUB_Add_Feature (gsub, rule->feature_index, rule->property_bit);
    }

  HB_GSUB_Apply_String (gsub, buffer->buffer);
}

/**
 * pango_ot_ruleset_position:
 * @ruleset: a #PangoOTRuleset.
 * @buffer: a #PangoOTBuffer.
 *
 * Performs the OpenType GPOS positioning on @buffer using the features
 * in @ruleset
 *
 * Since: 1.4
 **/
void
pango_ot_ruleset_position (const PangoOTRuleset *ruleset,
			   PangoOTBuffer        *buffer)
{
  unsigned int i;

  HB_GPOS gpos = NULL;

  g_return_if_fail (PANGO_IS_OT_RULESET (ruleset));
  g_return_if_fail (ruleset->info != NULL);

  for (i = 0; i < ruleset->rules->len; i++)
    {
      PangoOTRule *rule = &g_array_index (ruleset->rules, PangoOTRule, i);

      if (rule->table_type != PANGO_OT_TABLE_GPOS)
	continue;

      if (!gpos)
	{
	  gpos = pango_ot_info_get_gpos (ruleset->info);

	  if (gpos)
	    HB_GPOS_Clear_Features (gpos);
	  else
	    return;
	}

      HB_GPOS_Add_Feature (gpos, rule->feature_index, rule->property_bit);
    }

  if (HB_GPOS_Apply_String (ruleset->info->face, gpos, 0, buffer->buffer,
			    FALSE /* enable device-dependant values */,
			    buffer->rtl) == HB_Err_Ok)
    buffer->applied_gpos = TRUE;
}

/* ruleset descriptions */

/**
 * pango_ot_ruleset_description_hash:
 * @desc: a ruleset description
 *
 * Computes a hash of a #PangoOTRulesetDescription structure suitable
 * to be used, for example, as an argument to g_hash_table_new().
 *
 * Return value: the hash value.
 *
 * Since: 1.18
 **/
guint
pango_ot_ruleset_description_hash  (const PangoOTRulesetDescription *desc)
{
  guint hash = 0;
  guint i;

  hash ^= desc->script;
  hash ^= GPOINTER_TO_UINT (desc->language);

  hash ^= desc->n_static_gsub_features << 8;
  hash ^= GPOINTER_TO_UINT (desc->static_gsub_features);

  hash ^= desc->n_static_gpos_features << 12;
  hash ^= GPOINTER_TO_UINT (desc->static_gpos_features);

  hash ^= desc->n_other_features << 16;
  for (i = 0; i < desc->n_other_features; i++)
    {
      hash ^= * (guint32 *) desc->other_features[i].feature_name;
      hash ^= desc->other_features[i].property_bit;
    }

  return hash;
}

/**
 * pango_ot_ruleset_description_equal:
 * @desc1: a ruleset description
 * @desc2: a ruleset description
 *
 * Compares two ruleset descriptions for equality.
 * Two ruleset descriptions are considered equal if the rulesets
 * they describe are provably identical.  This means that their
 * script, language, and all feature sets should be equal.  For static feature
 * sets, the array addresses are compared directly, while for other
 * features, the list of features is compared one by one.
 * (Two ruleset descriptions may result in identical rulesets
 * being created, but still compare %FALSE.)
 *
 * Return value: %TRUE if two ruleset descriptions are identical,
 *               %FALSE otherwise.
 *
 * Since: 1.18
 **/
gboolean
pango_ot_ruleset_description_equal (const PangoOTRulesetDescription *desc1,
				    const PangoOTRulesetDescription *desc2)
{
  guint i;

#undef CHECK
#define CHECK(x) if (desc1->x != desc2->x) return FALSE;
#define CHECK_FEATURE_NAME(x) if (*(guint32 *)desc1->x != *(guint32 *)desc2->x) return FALSE

  CHECK (script);
  CHECK (language);

  CHECK (static_gsub_features);
  CHECK (n_static_gsub_features);
  CHECK (static_gpos_features);
  CHECK (n_static_gpos_features);

  CHECK (n_other_features);

  for (i = 0; i < desc1->n_other_features; i++)
    {
      CHECK_FEATURE_NAME (other_features[i].feature_name);
      CHECK (other_features[i].property_bit);
    }

#undef CHECK

  return TRUE;
}

/**
 * pango_ot_ruleset_description_copy:
 * @desc: ruleset description to copy
 *
 * Creates a copy of @desc, which should be freed with
 * pango_ot_ruleset_description_free(). Primarily used internally
 * by pango_ot_ruleset_get_for_description() to cache rulesets for
 * ruleset descriptions.
 *
 * Return value: the newly allocated #PangoOTRulesetDescription, which
 *               should be freed with pango_ot_ruleset_description_free().
 *
 * Since: 1.18
 **/
PangoOTRulesetDescription *
pango_ot_ruleset_description_copy  (const PangoOTRulesetDescription *desc)
{
  PangoOTRulesetDescription *copy;

  g_return_val_if_fail (desc != NULL, NULL);

  copy = g_slice_new (PangoOTRulesetDescription);

  *copy = *desc;

  if (desc->n_other_features)
    {
      PangoOTFeatureMap *map = g_new (PangoOTFeatureMap, desc->n_other_features);
      memcpy (map, desc->other_features, desc->n_other_features * sizeof (PangoOTFeatureMap));
      copy->other_features = map;
    }
  else
    {
      copy->other_features = NULL;
    }

  return copy;
}

/**
 * pango_ot_ruleset_description_free:
 * @desc: an allocated #PangoOTRulesetDescription
 *
 * Frees a ruleset description allocated by 
 * pango_ot_ruleset_description_copy().
 *
 * Since: 1.18
 **/
void
pango_ot_ruleset_description_free  (PangoOTRulesetDescription *desc)
{
  g_return_if_fail (desc != NULL);

  free ((gpointer) desc->other_features);

  g_slice_free (PangoOTRulesetDescription, desc);
}
