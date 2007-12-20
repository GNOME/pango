/* Pango
 * pango-ot-private.h: Implementation details for Pango OpenType code
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

#ifndef __PANGO_OT_PRIVATE_H__
#define __PANGO_OT_PRIVATE_H__

#include <glib-object.h>

#include <pango/pango-ot.h>
#include "opentype/harfbuzz.h"

G_BEGIN_DECLS

typedef struct _PangoOTInfoClass PangoOTInfoClass;

struct _PangoOTInfo
{
  GObject parent_instance;

  guint loaded;

  FT_Face face;

  HB_GSUB gsub;
  HB_GDEF gdef;
  HB_GPOS gpos;
};

struct _PangoOTInfoClass
{
  GObjectClass parent_class;
};


typedef struct _PangoOTRulesetClass PangoOTRulesetClass;

struct _PangoOTRuleset
{
  GObject parent_instance;

  GArray *rules;
  PangoOTInfo *info;

  /* the index into these arrays is a PangoOTTableType */
  guint n_features[2];
  guint script_index[2];
  guint language_index[2];
};

struct _PangoOTRulesetClass
{
  GObjectClass parent_class;
};

struct _PangoOTBuffer
{
  HB_Buffer buffer;
  gboolean should_free_hb_buffer;
  PangoFcFont *font;
  guint rtl : 1;
  guint zero_width_marks : 1;
  guint applied_gpos : 1;
};

HB_GDEF pango_ot_info_get_gdef (PangoOTInfo *info);
HB_GSUB pango_ot_info_get_gsub (PangoOTInfo *info);
HB_GPOS pango_ot_info_get_gpos (PangoOTInfo *info);

G_END_DECLS

#endif /* __PANGO_OT_PRIVATE_H__ */
