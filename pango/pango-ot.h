/* Pango
 * pango-ot.h:
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

#ifndef __PANGO_OT_H__
#define __PANGO_OT_H__

#include <freetype/freetype.h>
#include <pango/pango-glyph.h>

G_BEGIN_DECLS

#ifdef PANGO_ENABLE_ENGINE

typedef guint32  PangoOTTag;

typedef struct _PangoOTInfo    PangoOTInfo;
typedef struct _PangoOTRuleset PangoOTRuleset;

typedef enum 
{
  PANGO_OT_TABLE_GSUB,
  PANGO_OT_TABLE_GPOS
} PangoOTTableType;

PangoOTInfo *pango_ot_info_new (FT_Face face);

gboolean pango_ot_info_find_script   (PangoOTInfo      *info,
				      PangoOTTableType  table_type,
				      PangoOTTag        script_tag,
				      guint            *script_index);
gboolean pango_ot_info_find_language (PangoOTInfo      *info,
				      PangoOTTableType  table_type,
				      guint             script_index,
				      PangoOTTag        language_tag,
				      guint            *language_index,
				      guint            *required_feature_index);
gboolean pango_ot_info_find_feature  (PangoOTInfo      *info,
				      PangoOTTableType  table_type,
				      PangoOTTag        feature_tag,
				      guint             script_index,
				      guint             language_index,
				      guint            *feature_index);

PangoOTTag *pango_ot_info_list_scripts   (PangoOTInfo      *info,
					  PangoOTTableType  table_type);
PangoOTTag *pango_ot_info_list_languages (PangoOTInfo      *info,
					  PangoOTTableType  table_type,
					  guint             script_index,
					  PangoOTTag        language_tag);
PangoOTTag *pango_ot_info_list_features  (PangoOTInfo      *info,
					  PangoOTTableType  table_type,
					  PangoOTTag        tag,
					  guint             script_index,
					  guint             language_index);

  /* A pointer to a function which loads a glyph.  Its parameters are
   * the same as in a call to TT_Load_Glyph() -- if no glyph loading
   * function will be registered with pango_ot_set_glyph_loader(),
   * TT_Load_Glyph() will be called indeed.  The purpose of this function
   * pointer is to provide a hook for caching glyph outlines and sbits
   * (using the instance's generic pointer to hold the data).
   *
   * If for some reason no outline data is available (e.g. for an
   * embedded bitmap glyph), _glyph->outline.n_points should be set to
   * zero.  _glyph can be computed with
   *
   *    _glyph = HANDLE_Glyph( glyph )                                    
   */
typedef FT_Error  (*PangoOTGlyphLoader) (FT_Face      face,
				         FT_UInt      glyphIndex,
				         FT_Int       loadFlags,
					 gpointer     data);

 /* A pointer to a function which selects the alternate glyph.  `pos' is
  * the position of the glyph with index `glyphID', `num_alternates'
  * gives the number of alternates in the `alternates' array.  `data'
  * points to the user-defined structure specified during a call to
  * TT_GSUB_Register_Alternate_Function().  The function must return an
  * index into the `alternates' array.
  */
typedef FT_UShort  (*PangoOTAlternateFunc) (FT_ULong    pos,
					    FT_UShort   glyphID,
					    FT_UShort   num_alternates,
					    FT_UShort*  alternates,
					    gpointer    data);


PangoOTRuleset *pango_ot_ruleset_new (PangoOTInfo       *info);

void pango_ot_ruleset_set_glyph_loader   (PangoOTRuleset       *ruleset,
					  PangoOTGlyphLoader    func,
					  gpointer              data,
					  GDestroyNotify        notify);
void pango_ot_ruleset_set_alternate_func (PangoOTRuleset       *ruleset,
					  PangoOTAlternateFunc  func,
					  gpointer              data,
					  GDestroyNotify        notify);

void            pango_ot_ruleset_add_feature (PangoOTRuleset   *ruleset,
					      PangoOTTableType  table_type,
					      guint             feature_index,
					      gulong            property_bit);
void            pango_ot_ruleset_shape       (PangoOTRuleset   *ruleset,
					      PangoGlyphString *glyphs,
					      gulong           *properties);

#endif /* PANGO_ENABLE_ENGINE */

G_END_DECLS

#endif /* __PANGO_OT_H__ */
