/* Pango
 * pango-ot-info.c: Store tables for OpenType
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
#include FT_TRUETYPE_TABLES_H

#if (!GLIB_CHECK_VERSION(2,29,15))
#define G_UNICODE_SPACING_MARK G_UNICODE_COMBINING_MARK
#endif

static void pango_ot_info_finalize   (GObject *object);

static void synthesize_class_def (PangoOTInfo *info);

G_DEFINE_TYPE (PangoOTInfo, pango_ot_info, G_TYPE_OBJECT);

static void
pango_ot_info_init (PangoOTInfo *self)
{
}

static void
pango_ot_info_class_init (PangoOTInfoClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = pango_ot_info_finalize;
}

static void
pango_ot_info_finalize (GObject *object)
{
  PangoOTInfo *info = PANGO_OT_INFO (object);

  if (info->hb_face)
    hb_face_destroy (info->hb_face);

  G_OBJECT_CLASS (pango_ot_info_parent_class)->finalize (object);
}

static void
pango_ot_info_finalizer (void *object)
{
  FT_Face face = object;
  PangoOTInfo *info = face->generic.data;

  info->face = NULL;
  g_object_unref (info);
}

static hb_blob_t *
_get_table  (hb_tag_t tag, void *user_data)
{
  PangoOTInfo *info = (PangoOTInfo *) user_data;
  FT_Byte *buffer;
  FT_ULong  length = 0;
  FT_Error error;

  error = FT_Load_Sfnt_Table (info->face, tag, 0, NULL, &length);
  if (error)
    return hb_blob_create_empty ();

  buffer = g_malloc (length);
  if (buffer == NULL)
    return hb_blob_create_empty ();

  error = FT_Load_Sfnt_Table (info->face, tag, 0, buffer, &length);
  if (error)
    return hb_blob_create_empty ();

  return hb_blob_create ((const char *) buffer, length,
			 HB_MEMORY_MODE_WRITABLE,
			 g_free, buffer);
}


/**
 * pango_ot_info_get:
 * @face: a <type>FT_Face</type>.
 *
 * Returns the #PangoOTInfo structure for the given FreeType font face.
 *
 * Return value: the #PangoOTInfo for @face. This object will have
 * the same lifetime as @face.
 *
 * Since: 1.2
 **/
PangoOTInfo *
pango_ot_info_get (FT_Face face)
{
  PangoOTInfo *info;

  if (G_LIKELY (face->generic.data && face->generic.finalizer == pango_ot_info_finalizer))
    return face->generic.data;
  else
    {
      if (face->generic.finalizer)
        face->generic.finalizer (face);

      info = face->generic.data = g_object_new (PANGO_TYPE_OT_INFO, NULL);
      face->generic.finalizer = pango_ot_info_finalizer;

      info->face = face;

      if (face->stream->read == NULL) {
	hb_blob_t *blob;

	blob = hb_blob_create ((const char *) face->stream->base,
			       (unsigned int) face->stream->size,
			       HB_MEMORY_MODE_READONLY_MAY_MAKE_WRITABLE,
			       NULL, NULL);
	info->hb_face = hb_face_create_for_data (blob, face->face_index);
	hb_blob_destroy (blob);
      } else {
	info->hb_face = hb_face_create_for_tables (_get_table, NULL, info);
      }


      hb_face_set_unicode_funcs (info->hb_face, hb_glib_get_unicode_funcs ());

      /* XXX this is such a waste if not SFNT */
      if (!hb_ot_layout_has_font_glyph_classes (info->hb_face))
	synthesize_class_def (info);
    }

  return info;
}

hb_face_t *
_pango_ot_info_get_hb_face (PangoOTInfo *info)
{
  return info->hb_face;
}

typedef struct _GlyphInfo GlyphInfo;

struct _GlyphInfo {
  unsigned short glyph;
  unsigned short class;
};

static int
compare_glyph_info (gconstpointer a,
		    gconstpointer b)
{
  const GlyphInfo *info_a = a;
  const GlyphInfo *info_b = b;

  return (info_a->glyph < info_b->glyph) ? -1 :
    (info_a->glyph == info_b->glyph) ? 0 : 1;
}

/* Make a guess at the appropriate class for a glyph given
 * a character code that maps to the glyph
 */
static gboolean
get_glyph_class (gunichar        charcode,
		 unsigned short *class)
{
  /* For characters mapped into the Arabic Presentation forms, using properties
   * derived as we apply GSUB substitutions will be more reliable
   */
  if ((charcode >= 0xFB50 && charcode <= 0xFDFF) || /* Arabic Presentation Forms-A */
      (charcode >= 0xFE70 && charcode <= 0XFEFF))   /* Arabic Presentation Forms-B */
    return FALSE;

  switch ((int) g_unichar_type (charcode))
    {
    case G_UNICODE_SPACING_MARK:
    case G_UNICODE_ENCLOSING_MARK:
    case G_UNICODE_NON_SPACING_MARK:
      *class = HB_OT_LAYOUT_GLYPH_CLASS_MARK;		/* Mark glyph (non-spacing combining glyph) */
      return TRUE;
    case G_UNICODE_UNASSIGNED:
    case G_UNICODE_PRIVATE_USE:
      return FALSE;		/* Unknown, don't assign a class; classes get
				 * propagated during GSUB application */
    default:
      *class = HB_OT_LAYOUT_GLYPH_CLASS_BASE_GLYPH;	/* Base glyph (single character, spacing glyph) */
      return TRUE;
    }
}

static gboolean
set_unicode_charmap (FT_Face face)
{
  int charmap;

  for (charmap = 0; charmap < face->num_charmaps; charmap++)
    if (face->charmaps[charmap]->encoding == ft_encoding_unicode)
      {
	FT_Error error = FT_Set_Charmap(face, face->charmaps[charmap]);
	return error == FT_Err_Ok;
      }

  return FALSE;
}

/* Synthesize a GDEF table using the font's charmap and the
 * Unicode property database. We'll fill in class definitions
 * for glyphs not in the charmap as we walk through the tables.
 */
static void
synthesize_class_def (PangoOTInfo *info)
{
  GArray *glyph_infos;
  hb_codepoint_t *glyph_indices;
  unsigned char *classes;
  gunichar charcode;
  FT_UInt glyph;
  unsigned int i, j;
  FT_CharMap old_charmap;

  old_charmap = info->face->charmap;

  if (!old_charmap || old_charmap->encoding != ft_encoding_unicode)
    if (!set_unicode_charmap (info->face))
      return;

  glyph_infos = g_array_new (FALSE, FALSE, sizeof (GlyphInfo));

  /* Collect all the glyphs in the charmap, and guess
   * the appropriate classes for them
   */
  charcode = FT_Get_First_Char (info->face, &glyph);
  while (glyph != 0)
    {
      GlyphInfo glyph_info;

      if (glyph <= 65535)
	{
	  glyph_info.glyph = glyph;
	  if (get_glyph_class (charcode, &glyph_info.class))
	    g_array_append_val (glyph_infos, glyph_info);
	}

      charcode = FT_Get_Next_Char (info->face, charcode, &glyph);
    }

  /* Sort and remove duplicates
   */
  g_array_sort (glyph_infos, compare_glyph_info);

  glyph_indices = g_new (hb_codepoint_t, glyph_infos->len);
  classes = g_new (unsigned char, glyph_infos->len);

  for (i = 0, j = 0; i < glyph_infos->len; i++)
    {
      GlyphInfo *info = &g_array_index (glyph_infos, GlyphInfo, i);

      if (j == 0 || info->glyph != glyph_indices[j - 1])
	{
	  glyph_indices[j] = info->glyph;
	  classes[j] = info->class;

	  j++;
	}
    }

  g_array_free (glyph_infos, TRUE);

  hb_ot_layout_build_glyph_classes (info->hb_face, glyph_indices, classes, j);

  g_free (glyph_indices);
  g_free (classes);

  if (old_charmap && info->face->charmap != old_charmap)
    FT_Set_Charmap (info->face, old_charmap);
}

static hb_tag_t
get_hb_table_type (PangoOTTableType table_type)
{
  switch (table_type) {
    case PANGO_OT_TABLE_GSUB: return HB_OT_TAG_GSUB;
    case PANGO_OT_TABLE_GPOS: return HB_OT_TAG_GPOS;
    default:                  return HB_TAG_NONE;
  }
}

/**
 * pango_ot_info_find_script:
 * @info: a #PangoOTInfo.
 * @table_type: the table type to obtain information about.
 * @script_tag: the tag of the script to find.
 * @script_index: location to store the index of the script, or %NULL.
 *
 * Finds the index of a script.  If not found, tries to find the 'DFLT'
 * and then 'dflt' scripts and return the index of that in @script_index.
 * If none of those is found either, %PANGO_OT_NO_SCRIPT is placed in
 * @script_index.
 *
 * All other functions taking an input script_index parameter know
 * how to handle %PANGO_OT_NO_SCRIPT, so one can ignore the return
 * value of this function completely and proceed, to enjoy the automatic
 * fallback to the 'DFLT'/'dflt' script.
 *
 * Return value: %TRUE if the script was found.
 **/
gboolean
pango_ot_info_find_script (PangoOTInfo      *info,
			   PangoOTTableType  table_type,
			   PangoOTTag        script_tag,
			   guint            *script_index)
{
  hb_tag_t tt = get_hb_table_type (table_type);

  return hb_ot_layout_table_find_script (info->hb_face, tt,
					 script_tag,
					 script_index);
}

/**
 * pango_ot_info_find_language:
 * @info: a #PangoOTInfo.
 * @table_type: the table type to obtain information about.
 * @script_index: the index of the script whose languages are searched.
 * @language_tag: the tag of the language to find.
 * @language_index: location to store the index of the language, or %NULL.
 * @required_feature_index: location to store the required feature index of
 *    the language, or %NULL.
 *
 * Finds the index of a language and its required feature index.
 * If the language is not found, sets @language_index to
 * PANGO_OT_DEFAULT_LANGUAGE and the required feature of the default language
 * system is returned in required_feature_index.  For best compatibility with
 * some fonts, also searches the language system tag 'dflt' before falling
 * back to the default language system, but that is transparent to the user.
 * The user can simply ignore the return value of this function to
 * automatically fall back to the default language system.
 *
 * Return value: %TRUE if the language was found.
 **/
gboolean
pango_ot_info_find_language (PangoOTInfo      *info,
			     PangoOTTableType  table_type,
			     guint             script_index,
			     PangoOTTag        language_tag,
			     guint            *language_index,
			     guint            *required_feature_index)
{
  gboolean ret;
  unsigned l_index;
  hb_tag_t tt = get_hb_table_type (table_type);

  ret = hb_ot_layout_script_find_language (info->hb_face, tt,
					   script_index,
					   language_tag,
					   &l_index);
  if (language_index) *language_index = l_index;

  hb_ot_layout_language_get_required_feature_index (info->hb_face, tt,
						    script_index,
						    l_index,
						    required_feature_index);

  return ret;
}

/**
 * pango_ot_info_find_feature:
 * @info: a #PangoOTInfo.
 * @table_type: the table type to obtain information about.
 * @feature_tag: the tag of the feature to find.
 * @script_index: the index of the script.
 * @language_index: the index of the language whose features are searched,
 *     or %PANGO_OT_DEFAULT_LANGUAGE to use the default language of the script.
 * @feature_index: location to store the index of the feature, or %NULL.
 *
 * Finds the index of a feature.  If the feature is not found, sets
 * @feature_index to PANGO_OT_NO_FEATURE, which is safe to pass to
 * pango_ot_ruleset_add_feature() and similar functions.
 *
 * In the future, this may set @feature_index to an special value that if used
 * in pango_ot_ruleset_add_feature() will ask Pango to synthesize the
 * requested feature based on Unicode properties and data.  However, this
 * function will still return %FALSE in those cases.  So, users may want to
 * ignore the return value of this function in certain cases.
 *
 * Return value: %TRUE if the feature was found.
 **/
gboolean
pango_ot_info_find_feature  (PangoOTInfo      *info,
			     PangoOTTableType  table_type,
			     PangoOTTag        feature_tag,
			     guint             script_index,
			     guint             language_index,
			     guint            *feature_index)
{
  hb_tag_t tt = get_hb_table_type (table_type);

  return hb_ot_layout_language_find_feature (info->hb_face, tt,
					     script_index,
					     language_index,
					     feature_tag,
					     feature_index);
}

/**
 * pango_ot_info_list_scripts:
 * @info: a #PangoOTInfo.
 * @table_type: the table type to obtain information about.
 *
 * Obtains the list of available scripts.
 *
 * Return value: a newly-allocated zero-terminated array containing the tags of the
 *   available scripts.  Should be freed using g_free().
 **/
PangoOTTag *
pango_ot_info_list_scripts (PangoOTInfo      *info,
			    PangoOTTableType  table_type)
{
  hb_tag_t tt = get_hb_table_type (table_type);
  PangoOTTag *result;
  unsigned int count = 0;

  hb_ot_layout_table_get_script_tags (info->hb_face, tt, &count, NULL);
  result = g_new (PangoOTTag, count + 1);
  hb_ot_layout_table_get_script_tags (info->hb_face, tt, &count, result);
  result[count] = 0;

  return result;
}

/**
 * pango_ot_info_list_languages:
 * @info: a #PangoOTInfo.
 * @table_type: the table type to obtain information about.
 * @script_index: the index of the script to list languages for.
 * @language_tag: unused parameter.
 *
 * Obtains the list of available languages for a given script.
 *
 * Return value: a newly-allocated zero-terminated array containing the tags of the
 *   available languages.  Should be freed using g_free().
 **/
PangoOTTag *
pango_ot_info_list_languages (PangoOTInfo      *info,
			      PangoOTTableType  table_type,
			      guint             script_index,
			      PangoOTTag        language_tag G_GNUC_UNUSED)
{
  hb_tag_t tt = get_hb_table_type (table_type);
  PangoOTTag *result;
  unsigned int count = 0;

  hb_ot_layout_script_get_language_tags (info->hb_face, tt, script_index, &count, NULL);
  result = g_new (PangoOTTag, count + 1);
  hb_ot_layout_script_get_language_tags (info->hb_face, tt, script_index, &count, result);
  result[count] = 0;

  return result;
}

/**
 * pango_ot_info_list_features:
 * @info: a #PangoOTInfo.
 * @table_type: the table type to obtain information about.
 * @tag: unused parameter.
 * @script_index: the index of the script to obtain information about.
 * @language_index: the index of the language to list features for, or
 *     %PANGO_OT_DEFAULT_LANGUAGE, to list features for the default
 *     language of the script.
 *
 * Obtains the list of features for the given language of the given script.
 *
 * Return value: a newly-allocated zero-terminated array containing the tags of the
 * available features.  Should be freed using g_free().
 **/
PangoOTTag *
pango_ot_info_list_features  (PangoOTInfo      *info,
			      PangoOTTableType  table_type,
			      PangoOTTag        tag G_GNUC_UNUSED,
			      guint             script_index,
			      guint             language_index)
{
  hb_tag_t tt = get_hb_table_type (table_type);
  PangoOTTag *result;
  unsigned int count = 0;

  hb_ot_layout_language_get_feature_tags (info->hb_face, tt, script_index, language_index, &count, NULL);
  result = g_new (PangoOTTag, count + 1);
  hb_ot_layout_language_get_feature_tags (info->hb_face, tt, script_index, language_index, &count, result);
  result[count] = 0;

  return result;
}

void
_pango_ot_info_substitute  (const PangoOTInfo    *info,
			    const PangoOTRuleset *ruleset,
			    PangoOTBuffer        *buffer)
{
  unsigned int i;

  for (i = 0; i < ruleset->rules->len; i++)
    {
      PangoOTRule *rule = &g_array_index (ruleset->rules, PangoOTRule, i);
      unsigned int lookup_count, j;
      unsigned int lookup_indexes[1000];

      if (rule->table_type != PANGO_OT_TABLE_GSUB)
	continue;

      lookup_count = G_N_ELEMENTS (lookup_indexes);
      hb_ot_layout_feature_get_lookup_indexes (info->hb_face,
					       HB_OT_TAG_GSUB,
					       rule->feature_index,
					       &lookup_count,
					       lookup_indexes);

      lookup_count = MIN (G_N_ELEMENTS (lookup_indexes), lookup_count);
      for (j = 0; j < lookup_count; j++)
	hb_ot_layout_substitute_lookup (info->hb_face,
					buffer->buffer,
					lookup_indexes[j],
					rule->property_bit);
    }
}

void
_pango_ot_info_position    (const PangoOTInfo    *info,
			    const PangoOTRuleset *ruleset,
			    PangoOTBuffer        *buffer)
{
  unsigned int i;
  gboolean is_hinted;
  hb_font_t *hb_font;

  hb_buffer_clear_positions (buffer->buffer);

  /* XXX reuse hb_font */
  hb_font = hb_font_create ();
  hb_font_set_scale (hb_font,
		     info->face->size->metrics.x_scale,
		     info->face->size->metrics.y_scale);
  is_hinted = buffer->font->is_hinted;
  hb_font_set_ppem (hb_font,
		    is_hinted ? info->face->size->metrics.x_ppem : 0,
		    is_hinted ? info->face->size->metrics.y_ppem : 0);

  for (i = 0; i < ruleset->rules->len; i++)
    {
      PangoOTRule *rule = &g_array_index (ruleset->rules, PangoOTRule, i);
      hb_mask_t mask;
      unsigned int lookup_count, j;
      unsigned int lookup_indexes[1000];

      if (rule->table_type != PANGO_OT_TABLE_GPOS)
	continue;

      mask = rule->property_bit;
      lookup_count = G_N_ELEMENTS (lookup_indexes);
      hb_ot_layout_feature_get_lookup_indexes (info->hb_face,
					       HB_OT_TAG_GPOS,
					       rule->feature_index,
					       &lookup_count,
					       lookup_indexes);

      lookup_count = MIN (G_N_ELEMENTS (lookup_indexes), lookup_count);
      for (j = 0; j < lookup_count; j++)
	hb_ot_layout_position_lookup (info->hb_face, hb_font,
				      buffer->buffer,
				      lookup_indexes[j],
				      rule->property_bit);

      buffer->applied_gpos = TRUE;
    }

    if (buffer->applied_gpos)
    {
      unsigned int i, j;
      unsigned int len = hb_buffer_get_len (buffer->buffer);
      hb_glyph_position_t *positions = hb_buffer_get_glyph_positions (buffer->buffer);

      /* First handle all left-to-right connections */
      for (j = 0; j < len; j++)
      {
	if (positions[j].cursive_chain > 0)
	  positions[j].y_pos += positions[j - positions[j].cursive_chain].y_pos;
      }

      /* Then handle all right-to-left connections */
      for (i = len; i > 0; i--)
      {
	j = i - 1;

	if (positions[j].cursive_chain < 0)
	  positions[j].y_pos += positions[j - positions[j].cursive_chain].y_pos;
      }
    }

  hb_font_destroy (hb_font);
}
