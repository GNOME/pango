/* Pango
 * thai-ot.c:
 *
 * Copyright (C) 2004 Theppitak Karoonboonyanan
 * Author: Theppitak Karoonboonyanan <thep@linux.thai.net>
 */

#include <string.h>

#include "thai-ot.h"

static void
maybe_add_gsub_feature (PangoOTRuleset *ruleset,
			PangoOTInfo    *info,
			guint           script_index,
			PangoOTTag      feature_tag,
			gulong          property_bit)
{
  guint feature_index;
  
  /* 0xffff == default language system */
  if (pango_ot_info_find_feature (info, PANGO_OT_TABLE_GSUB,
				  feature_tag, script_index, 0xffff, &feature_index))
    {
      pango_ot_ruleset_add_feature (ruleset, PANGO_OT_TABLE_GSUB, feature_index,
				    property_bit);
    }
}

static void
maybe_add_gpos_feature (PangoOTRuleset *ruleset,
		        PangoOTInfo    *info,
			guint           script_index,
			PangoOTTag      feature_tag,
			gulong          property_bit)
{
  guint feature_index;

  if (pango_ot_info_find_feature (info, PANGO_OT_TABLE_GPOS,
				  feature_tag, script_index, 0xffff, &feature_index))
    {
      pango_ot_ruleset_add_feature (ruleset, PANGO_OT_TABLE_GPOS, feature_index,
				    property_bit);
    }
}

static PangoOTRuleset *
get_gsub_ruleset (FT_Face face)
{
  PangoOTInfo    *info = pango_ot_info_get (face);
  GQuark          ruleset_quark = g_quark_from_string ("thai-gsub-ruleset");
  PangoOTRuleset *ruleset;

  if (!info)
    return NULL;

  ruleset = g_object_get_qdata (G_OBJECT (info), ruleset_quark);

  if (!ruleset)
    {
      PangoOTTag thai_tag = FT_MAKE_TAG ('t', 'h', 'a', 'i');
      guint      script_index;

      ruleset = pango_ot_ruleset_new (info);

     if (pango_ot_info_find_script (info, PANGO_OT_TABLE_GSUB,
				     thai_tag, &script_index))
	{
	  maybe_add_gsub_feature (ruleset, info, script_index, FT_MAKE_TAG ('c','c','m','p'), 0xFFFF);
	  maybe_add_gsub_feature (ruleset, info, script_index, FT_MAKE_TAG ('l','i','g','a'), 0xFFFF);
	}

      g_object_set_qdata_full (G_OBJECT (info), ruleset_quark, ruleset,
			       (GDestroyNotify)g_object_unref);
    }

  return ruleset;
}


static PangoOTRuleset *
get_gpos_ruleset (FT_Face face)
{
  PangoOTInfo    *info = pango_ot_info_get (face);
  GQuark          ruleset_quark = g_quark_from_string ("thai-gpos-ruleset");
  PangoOTRuleset *ruleset;

  if (!info)
    return NULL;

  ruleset = g_object_get_qdata (G_OBJECT (info), ruleset_quark);

  if (!ruleset)
    {
      PangoOTTag thai_tag = FT_MAKE_TAG ('t', 'h', 'a', 'i');
      guint      script_index;

      ruleset = pango_ot_ruleset_new (info);

      if (pango_ot_info_find_script (info, PANGO_OT_TABLE_GPOS,
				     thai_tag, &script_index))
	{
	  maybe_add_gpos_feature (ruleset, info, script_index, FT_MAKE_TAG ('k','e','r','n'), 0xFFFF);
	  maybe_add_gpos_feature (ruleset, info, script_index, FT_MAKE_TAG ('m','a','r','k'), 0xFFFF);
	  maybe_add_gpos_feature (ruleset, info, script_index, FT_MAKE_TAG ('m','k','m','k'), 0xFFFF);
	}

      g_object_set_qdata_full (G_OBJECT (info), ruleset_quark, ruleset,
			       (GDestroyNotify)g_object_unref);
    }

  return ruleset;
}


void 
thai_ot_shape (PangoFont        *font,
               PangoGlyphString *glyphs)
{
  FT_Face         face;
  PangoOTRuleset *gsub_ruleset, *gpos_ruleset;
  PangoFcFont    *fc_font;

  g_return_if_fail (font != NULL);
  g_return_if_fail (glyphs != NULL);

  fc_font = PANGO_FC_FONT (font);
  face = pango_fc_font_lock_face (fc_font);
  g_assert (face != NULL);

  gsub_ruleset = get_gsub_ruleset (face);
  gpos_ruleset = get_gpos_ruleset (face);

  if (gsub_ruleset != NULL || gpos_ruleset != NULL)
    {
      gint i;
      PangoOTBuffer *buffer;

      /* prepare ot buffer */
      buffer = pango_ot_buffer_new (fc_font);
      for (i = 0; i < glyphs->num_glyphs; i++)
        {
          pango_ot_buffer_add_glyph (buffer,
				     glyphs->glyphs[i].glyph,
				     0,
				     glyphs->log_clusters[i]);
        }

      /* do gsub processing */
      if (gsub_ruleset != NULL)
        pango_ot_ruleset_substitute (gsub_ruleset, buffer);

      /* do gpos processing */
      if (gpos_ruleset != NULL)
        pango_ot_ruleset_position (gpos_ruleset, buffer);

      pango_ot_buffer_output (buffer, glyphs);
      pango_ot_buffer_destroy (buffer);
    }

  pango_fc_font_unlock_face (fc_font);
}

