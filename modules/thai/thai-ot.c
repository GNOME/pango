/* Pango
 * thai-ot.c:
 *
 * Copyright (C) 2004 Theppitak Karoonboonyanan
 * Author: Theppitak Karoonboonyanan <thep@linux.thai.net>
 */

#include <string.h>

#include "thai-ot.h"

static gint
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
      return 1;
    }
  return 0;
}

static gint
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
      return 1;
    }
  return 0;
}

PangoOTRuleset *
thai_ot_get_ruleset (PangoFont *font)
{
  PangoFcFont    *fc_font;
  FT_Face         face;
  PangoOTInfo    *info;
  PangoOTRuleset *ruleset = NULL;

  g_return_val_if_fail (font != NULL, NULL);

  fc_font = PANGO_FC_FONT (font);
  face = pango_fc_font_lock_face (fc_font);
  g_assert (face != NULL);

  info = pango_ot_info_get (face);
  if (info != NULL)
    {
      static GQuark ruleset_quark = 0;

      if (!ruleset_quark)
        ruleset_quark = g_quark_from_string ("thai-ot-ruleset");

      ruleset = g_object_get_qdata (G_OBJECT (info), ruleset_quark);
      if (!ruleset)
        {
          PangoOTTag thai_tag = FT_MAKE_TAG ('t', 'h', 'a', 'i');
          guint      script_index;
          gint       n = 0;

          ruleset = pango_ot_ruleset_new (info);

          if (pango_ot_info_find_script (info, PANGO_OT_TABLE_GSUB,
				         thai_tag, &script_index))
	    {
	      n += maybe_add_gsub_feature (ruleset, info, script_index,
			                   FT_MAKE_TAG ('c','c','m','p'),
					   0xFFFF);
	      n += maybe_add_gsub_feature (ruleset, info, script_index,
			                   FT_MAKE_TAG ('l','i','g','a'),
					   0xFFFF);
	    }

          if (pango_ot_info_find_script (info, PANGO_OT_TABLE_GPOS,
				         thai_tag, &script_index))
	    {
	      n += maybe_add_gpos_feature (ruleset, info, script_index,
			                   FT_MAKE_TAG ('k','e','r','n'),
					   0xFFFF);
	      n += maybe_add_gpos_feature (ruleset, info, script_index,
			                   FT_MAKE_TAG ('m','a','r','k'),
					   0xFFFF);
	      n += maybe_add_gpos_feature (ruleset, info, script_index,
			                   FT_MAKE_TAG ('m','k','m','k'),
					   0xFFFF);
	    }

	  if (n > 0)
            g_object_set_qdata_full (G_OBJECT (info), ruleset_quark, ruleset,
	    		             (GDestroyNotify)g_object_unref);
	  else
	    {
	      g_object_unref (ruleset);
	      ruleset = NULL;
	    }
        }
    }

  pango_fc_font_unlock_face (fc_font);

  return ruleset;
}


void 
thai_ot_shape (PangoFont        *font,
               PangoGlyphString *glyphs)
{
  PangoOTRuleset *ot_ruleset;

  g_return_if_fail (font != NULL);
  g_return_if_fail (glyphs != NULL);

  ot_ruleset = thai_ot_get_ruleset (font);

  if (ot_ruleset != NULL)
    {
      gint i;
      PangoOTBuffer *buffer;

      /* prepare ot buffer */
      buffer = pango_ot_buffer_new (PANGO_FC_FONT (font));
      for (i = 0; i < glyphs->num_glyphs; i++)
        {
          pango_ot_buffer_add_glyph (buffer,
				     glyphs->glyphs[i].glyph,
				     0,
				     glyphs->log_clusters[i]);
        }

      pango_ot_ruleset_substitute (ot_ruleset, buffer);
      pango_ot_ruleset_position (ot_ruleset, buffer);

      pango_ot_buffer_output (buffer, glyphs);
      pango_ot_buffer_destroy (buffer);
    }
}

