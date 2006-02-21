/* Pango
 * thai-ot.c:
 *
 * Copyright (C) 2004 Theppitak Karoonboonyanan
 * Author: Theppitak Karoonboonyanan <thep@linux.thai.net>
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
  if (!face)
    return NULL;

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

static PangoOTRuleset *
lao_ot_get_ruleset (PangoFont *font)
{
  PangoFcFont    *fc_font;
  FT_Face         face;
  PangoOTInfo    *info;
  PangoOTRuleset *ruleset = NULL;

  g_return_val_if_fail (font != NULL, NULL);

  fc_font = PANGO_FC_FONT (font);
  face = pango_fc_font_lock_face (fc_font);
  if (!face)
    return NULL;

  info = pango_ot_info_get (face);
  if (info != NULL)
    {
      static GQuark ruleset_quark = 0;

      if (!ruleset_quark)
        ruleset_quark = g_quark_from_string ("lao-ot-ruleset");

      ruleset = g_object_get_qdata (G_OBJECT (info), ruleset_quark);
      if (!ruleset)
        {
          PangoOTTag thai_tag = FT_MAKE_TAG ('l', 'a', 'o', ' ');
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
  PangoOTRuleset *th_ruleset;
  PangoOTRuleset *lo_ruleset;

  th_ruleset = thai_ot_get_ruleset (font);
  lo_ruleset = lao_ot_get_ruleset (font);

  if (th_ruleset != NULL || lo_ruleset != NULL)
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

      if (th_ruleset != NULL)
        {
          pango_ot_ruleset_substitute (th_ruleset, buffer);
          pango_ot_ruleset_position (th_ruleset, buffer);
        }
      if (lo_ruleset != NULL)
        {
          pango_ot_ruleset_substitute (lo_ruleset, buffer);
          pango_ot_ruleset_position (lo_ruleset, buffer);
        }

      pango_ot_buffer_output (buffer, glyphs);
      pango_ot_buffer_destroy (buffer);
    }
}

