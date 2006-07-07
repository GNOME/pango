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

#include <config.h>

#include "pango-ot-private.h"
#include "pango-impl-utils.h"

typedef struct _PangoOTRule PangoOTRule;

struct _PangoOTRule 
{
  gulong     property_bit;
  FT_UShort  feature_index;
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

  if (!object_type)
    {
      static const GTypeInfo object_info =
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
}

static void 
pango_ot_ruleset_finalize (GObject *object)
{
  PangoOTRuleset *ruleset = PANGO_OT_RULESET (object);

  g_array_free (ruleset->rules, TRUE);
  if (ruleset->info)
    g_object_remove_weak_pointer (ruleset->info, &ruleset->info);

  parent_class->finalize (object);
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

  g_return_val_if_fail (info != NULL, NULL);

  ruleset = g_object_new (PANGO_TYPE_OT_RULESET, NULL);

  ruleset->info = info;
  g_object_add_weak_pointer (ruleset->info, &ruleset->info);

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
  g_return_if_fail (PANGO_IS_OT_INFO (ruleset->info));

  tmp_rule.table_type = table_type;
  tmp_rule.feature_index = feature_index;
  tmp_rule.property_bit = property_bit;

  g_array_append_val (ruleset->rules, tmp_rule);
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
pango_ot_ruleset_substitute  (PangoOTRuleset   *ruleset,
			      PangoOTBuffer    *buffer)
{
  unsigned int i;
  
  HB_GSUB gsub = NULL;
  
  g_return_if_fail (PANGO_IS_OT_RULESET (ruleset));
  g_return_if_fail (PANGO_IS_OT_INFO (ruleset->info));

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
 * Performs the OpenType GPOS positionoing on @buffer using the features
 * in @ruleset
 *
 * Since: 1.4
 **/
void
pango_ot_ruleset_position (PangoOTRuleset   *ruleset,
			   PangoOTBuffer    *buffer)
{
  unsigned int i;
  
  HB_GPOS gpos = NULL;
  
  g_return_if_fail (PANGO_IS_OT_RULESET (ruleset));
  g_return_if_fail (PANGO_IS_OT_INFO (ruleset->info));

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
			    buffer->rtl) == FT_Err_Ok)
    buffer->applied_gpos = TRUE;
}

