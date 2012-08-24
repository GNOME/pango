/* Pango
 * pango-ot.h:
 *
 * Copyright (C) 2000,2007 Red Hat Software
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

#ifndef __PANGO_OT_H__
#define __PANGO_OT_H__

/* Deprecated.  Use HarfBuzz directly! */

#include <pango/pangofc-font.h>
#include <pango/pango-glyph.h>
#include <pango/pango-font.h>
#include <pango/pango-script.h>
#include <pango/pango-language.h>

G_BEGIN_DECLS

#ifdef PANGO_ENABLE_ENGINE

typedef guint32 PangoOTTag;

#define PANGO_OT_TAG_MAKE(c1,c2,c3,c4)		((PangoOTTag) FT_MAKE_TAG (c1, c2, c3, c4))
#define PANGO_OT_TAG_MAKE_FROM_STRING(s)	(PANGO_OT_TAG_MAKE(((const char *) s)[0], \
								   ((const char *) s)[1], \
								   ((const char *) s)[2], \
								   ((const char *) s)[3]))

typedef struct _PangoOTInfo       PangoOTInfo;
typedef struct _PangoOTBuffer     PangoOTBuffer;
typedef struct _PangoOTGlyph      PangoOTGlyph;
typedef struct _PangoOTRuleset    PangoOTRuleset;
typedef struct _PangoOTFeatureMap PangoOTFeatureMap;
typedef struct _PangoOTRulesetDescription PangoOTRulesetDescription;

typedef enum
{
  PANGO_OT_TABLE_GSUB,
  PANGO_OT_TABLE_GPOS
} PangoOTTableType;

#define PANGO_OT_ALL_GLYPHS			((guint) 0xFFFF)
#define PANGO_OT_NO_FEATURE			((guint) 0xFFFF)
#define PANGO_OT_NO_SCRIPT			((guint) 0xFFFF)
#define PANGO_OT_DEFAULT_LANGUAGE		((guint) 0xFFFF)

#define PANGO_OT_TAG_DEFAULT_SCRIPT		PANGO_OT_TAG_MAKE ('D', 'F', 'L', 'T')
#define PANGO_OT_TAG_DEFAULT_LANGUAGE		PANGO_OT_TAG_MAKE ('d', 'f', 'l', 't')

/* Note that this must match hb_glyph_info_t */
struct _PangoOTGlyph
{
  guint32  glyph;
  guint    properties;
  guint    cluster;
  gushort  component;
  gushort  ligID;

  guint    internal;
};

struct _PangoOTFeatureMap
{
  char     feature_name[5];
  gulong   property_bit;
};

struct _PangoOTRulesetDescription {
  PangoScript               script;
  PangoLanguage            *language;
  const PangoOTFeatureMap  *static_gsub_features;
  guint                   n_static_gsub_features;
  const PangoOTFeatureMap  *static_gpos_features;
  guint                   n_static_gpos_features;
  const PangoOTFeatureMap  *other_features;
  guint                   n_other_features;
};


#define PANGO_TYPE_OT_INFO              (pango_ot_info_get_type ())
#define PANGO_OT_INFO(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_OT_INFO, PangoOTInfo))
#define PANGO_IS_OT_INFO(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_OT_INFO))
GType pango_ot_info_get_type (void) G_GNUC_CONST;

#define PANGO_TYPE_OT_RULESET           (pango_ot_ruleset_get_type ())
#define PANGO_OT_RULESET(object)        (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_OT_RULESET, PangoOTRuleset))
#define PANGO_IS_OT_RULESET(object)     (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_OT_RULESET))
GType pango_ot_ruleset_get_type (void) G_GNUC_CONST;


G_DEPRECATED
PangoOTInfo *pango_ot_info_get (FT_Face face);

G_DEPRECATED
gboolean pango_ot_info_find_script   (PangoOTInfo      *info,
				      PangoOTTableType  table_type,
				      PangoOTTag        script_tag,
				      guint            *script_index);

G_DEPRECATED
gboolean pango_ot_info_find_language (PangoOTInfo      *info,
				      PangoOTTableType  table_type,
				      guint             script_index,
				      PangoOTTag        language_tag,
				      guint            *language_index,
				      guint            *required_feature_index);
G_DEPRECATED
gboolean pango_ot_info_find_feature  (PangoOTInfo      *info,
				      PangoOTTableType  table_type,
				      PangoOTTag        feature_tag,
				      guint             script_index,
				      guint             language_index,
				      guint            *feature_index);

G_DEPRECATED
PangoOTTag *pango_ot_info_list_scripts   (PangoOTInfo      *info,
					  PangoOTTableType  table_type);
G_DEPRECATED
PangoOTTag *pango_ot_info_list_languages (PangoOTInfo      *info,
					  PangoOTTableType  table_type,
					  guint             script_index,
					  PangoOTTag        language_tag);
G_DEPRECATED
PangoOTTag *pango_ot_info_list_features  (PangoOTInfo      *info,
					  PangoOTTableType  table_type,
					  PangoOTTag        tag,
					  guint             script_index,
					  guint             language_index);

G_DEPRECATED
PangoOTBuffer *pango_ot_buffer_new        (PangoFcFont       *font);
G_DEPRECATED
void           pango_ot_buffer_destroy    (PangoOTBuffer     *buffer);
G_DEPRECATED
void           pango_ot_buffer_clear      (PangoOTBuffer     *buffer);
G_DEPRECATED
void           pango_ot_buffer_set_rtl    (PangoOTBuffer     *buffer,
					   gboolean           rtl);
G_DEPRECATED
void           pango_ot_buffer_add_glyph  (PangoOTBuffer     *buffer,
					   guint              glyph,
					   guint              properties,
					   guint              cluster);
G_DEPRECATED
void           pango_ot_buffer_get_glyphs (const PangoOTBuffer  *buffer,
					   PangoOTGlyph        **glyphs,
					   int                  *n_glyphs);
G_DEPRECATED
void           pango_ot_buffer_output     (const PangoOTBuffer  *buffer,
					   PangoGlyphString     *glyphs);

G_DEPRECATED
void           pango_ot_buffer_set_zero_width_marks (PangoOTBuffer     *buffer,
						     gboolean           zero_width_marks);

G_DEPRECATED
const PangoOTRuleset *pango_ot_ruleset_get_for_description (PangoOTInfo                     *info,
							    const PangoOTRulesetDescription *desc);
G_DEPRECATED
PangoOTRuleset *pango_ot_ruleset_new (PangoOTInfo       *info);
G_DEPRECATED
PangoOTRuleset *pango_ot_ruleset_new_for (PangoOTInfo       *info,
					  PangoScript        script,
					  PangoLanguage     *language);
G_DEPRECATED
PangoOTRuleset *pango_ot_ruleset_new_from_description (PangoOTInfo                     *info,
						       const PangoOTRulesetDescription *desc);
G_DEPRECATED
void            pango_ot_ruleset_add_feature (PangoOTRuleset   *ruleset,
					      PangoOTTableType  table_type,
					      guint             feature_index,
					      gulong            property_bit);
G_DEPRECATED
gboolean        pango_ot_ruleset_maybe_add_feature (PangoOTRuleset   *ruleset,
						    PangoOTTableType  table_type,
						    PangoOTTag        feature_tag,
						    gulong            property_bit);
G_DEPRECATED
guint           pango_ot_ruleset_maybe_add_features (PangoOTRuleset          *ruleset,
						     PangoOTTableType         table_type,
						     const PangoOTFeatureMap *features,
						     guint                    n_features);
G_DEPRECATED
guint           pango_ot_ruleset_get_feature_count (const PangoOTRuleset   *ruleset,
						    guint                  *n_gsub_features,
						    guint                  *n_gpos_features);

G_DEPRECATED
void            pango_ot_ruleset_substitute  (const PangoOTRuleset   *ruleset,
					      PangoOTBuffer          *buffer);

G_DEPRECATED
void            pango_ot_ruleset_position    (const PangoOTRuleset   *ruleset,
					      PangoOTBuffer          *buffer);

G_DEPRECATED
PangoScript     pango_ot_tag_to_script     (PangoOTTag     script_tag) G_GNUC_CONST;

G_DEPRECATED
PangoOTTag      pango_ot_tag_from_script   (PangoScript    script) G_GNUC_CONST;

G_DEPRECATED
PangoLanguage  *pango_ot_tag_to_language   (PangoOTTag     language_tag) G_GNUC_CONST;

G_DEPRECATED
PangoOTTag      pango_ot_tag_from_language (PangoLanguage *language) G_GNUC_CONST;

G_DEPRECATED
guint           pango_ot_ruleset_description_hash  (const PangoOTRulesetDescription *desc) G_GNUC_PURE;

G_DEPRECATED
gboolean        pango_ot_ruleset_description_equal (const PangoOTRulesetDescription *desc1,
						    const PangoOTRulesetDescription *desc2) G_GNUC_PURE;

G_DEPRECATED
PangoOTRulesetDescription *pango_ot_ruleset_description_copy  (const PangoOTRulesetDescription *desc);

G_DEPRECATED
void            pango_ot_ruleset_description_free  (PangoOTRulesetDescription       *desc);


#endif /* PANGO_ENABLE_ENGINE */

G_END_DECLS

#endif /* __PANGO_OT_H__ */
