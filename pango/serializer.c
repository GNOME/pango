/* Pango
 * serializer.c: Code to serialize various Pango objects
 *
 * Copyright (C) 2021 Red Hat, Inc
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

#include <pango/pango-layout.h>
#include <pango/pango-layout-private.h>
#include <pango/pango-context-private.h>
#include <pango/pango-enum-types.h>
#include <pango/pango-font-private.h>

#include <hb-ot.h>
#include <json-glib/json-glib.h>

/* {{{ Error handling */

G_DEFINE_QUARK(pango-layout-deserialize-error-quark, pango_layout_deserialize_error)

/* }}} */
/* {{{ Serialization */

static GType
get_enum_type (PangoAttrType attr_type)
{
  switch ((int)attr_type)
    {
    case PANGO_ATTR_STYLE: return PANGO_TYPE_STYLE;
    case PANGO_ATTR_WEIGHT: return PANGO_TYPE_WEIGHT;
    case PANGO_ATTR_VARIANT: return PANGO_TYPE_VARIANT;
    case PANGO_ATTR_STRETCH: return PANGO_TYPE_STRETCH;
    case PANGO_ATTR_GRAVITY: return PANGO_TYPE_GRAVITY;
    case PANGO_ATTR_GRAVITY_HINT: return PANGO_TYPE_GRAVITY_HINT;
    case PANGO_ATTR_UNDERLINE: return PANGO_TYPE_UNDERLINE;
    case PANGO_ATTR_OVERLINE: return PANGO_TYPE_OVERLINE;
    case PANGO_ATTR_BASELINE_SHIFT: return PANGO_TYPE_BASELINE_SHIFT;
    case PANGO_ATTR_FONT_SCALE: return PANGO_TYPE_FONT_SCALE;
    case PANGO_ATTR_TEXT_TRANSFORM: return PANGO_TYPE_TEXT_TRANSFORM;
    default: return G_TYPE_INVALID;
    }
}

static void
add_enum_value (JsonBuilder   *builder,
                GType          type,
                int            value,
                gboolean       allow_extra)
{
  GEnumClass *enum_class;
  GEnumValue *enum_value;

  enum_class = g_type_class_ref (type);
  enum_value = g_enum_get_value (enum_class, value);

  if (enum_value)
    json_builder_add_string_value (builder, enum_value->value_nick);
  else if (allow_extra)
    {
      char buf[128];
      g_snprintf (buf, 128, "%d", value);
      json_builder_add_string_value (builder, buf);
    }
  else
    json_builder_add_string_value (builder, "ERROR");

}

static void
add_attribute (JsonBuilder    *builder,
               PangoAttribute *attr)
{
  char *str;

  json_builder_begin_object (builder);

  if (attr->start_index != PANGO_ATTR_INDEX_FROM_TEXT_BEGINNING)
    {
      json_builder_set_member_name (builder, "start");
      json_builder_add_int_value (builder, (int)attr->start_index);
    }
  if (attr->end_index != PANGO_ATTR_INDEX_TO_TEXT_END)
    {
      json_builder_set_member_name (builder, "end");
      json_builder_add_int_value (builder, (int)attr->end_index);
    }
  json_builder_set_member_name (builder, "type");
  add_enum_value (builder, PANGO_TYPE_ATTR_TYPE, attr->klass->type, FALSE);

  json_builder_set_member_name (builder, "value");
  switch (attr->klass->type)
    {
    default:
    case PANGO_ATTR_INVALID:
      g_assert_not_reached ();
    case PANGO_ATTR_LANGUAGE:
      json_builder_add_string_value (builder, pango_language_to_string (((PangoAttrLanguage*)attr)->value));
      break;
    case PANGO_ATTR_FAMILY:
    case PANGO_ATTR_FONT_FEATURES:
      json_builder_add_string_value (builder, ((PangoAttrString*)attr)->value);
      break;
    case PANGO_ATTR_STYLE:
    case PANGO_ATTR_VARIANT:
    case PANGO_ATTR_STRETCH:
    case PANGO_ATTR_UNDERLINE:
    case PANGO_ATTR_OVERLINE:
    case PANGO_ATTR_GRAVITY:
    case PANGO_ATTR_GRAVITY_HINT:
    case PANGO_ATTR_TEXT_TRANSFORM:
    case PANGO_ATTR_FONT_SCALE:
      add_enum_value (builder, get_enum_type (attr->klass->type), ((PangoAttrInt*)attr)->value, FALSE);
      break;
    case PANGO_ATTR_WEIGHT:
    case PANGO_ATTR_BASELINE_SHIFT:
      add_enum_value (builder, get_enum_type (attr->klass->type), ((PangoAttrInt*)attr)->value, TRUE);
      break;
    case PANGO_ATTR_SIZE:
    case PANGO_ATTR_RISE:
    case PANGO_ATTR_LETTER_SPACING:
    case PANGO_ATTR_ABSOLUTE_SIZE:
    case PANGO_ATTR_FOREGROUND_ALPHA:
    case PANGO_ATTR_BACKGROUND_ALPHA:
    case PANGO_ATTR_SHOW:
    case PANGO_ATTR_WORD:
    case PANGO_ATTR_SENTENCE:
    case PANGO_ATTR_ABSOLUTE_LINE_HEIGHT:
      json_builder_add_int_value (builder, ((PangoAttrInt*)attr)->value);
      break;
    case PANGO_ATTR_FONT_DESC:
      str = pango_font_description_to_string (((PangoAttrFontDesc*)attr)->desc);
      json_builder_add_string_value (builder, str);
      g_free (str);
      break;
    case PANGO_ATTR_FOREGROUND:
    case PANGO_ATTR_BACKGROUND:
    case PANGO_ATTR_UNDERLINE_COLOR:
    case PANGO_ATTR_OVERLINE_COLOR:
    case PANGO_ATTR_STRIKETHROUGH_COLOR:
      str = pango_color_to_string (&((PangoAttrColor*)attr)->color);
      json_builder_add_string_value (builder, str);
      g_free (str);
      break;
    case PANGO_ATTR_STRIKETHROUGH:
    case PANGO_ATTR_FALLBACK:
    case PANGO_ATTR_ALLOW_BREAKS:
    case PANGO_ATTR_INSERT_HYPHENS:
      json_builder_add_boolean_value (builder, ((PangoAttrInt*)attr)->value != 0);
      break;
    case PANGO_ATTR_SHAPE:
      json_builder_add_string_value (builder, "shape");
      break;
    case PANGO_ATTR_SCALE:
    case PANGO_ATTR_LINE_HEIGHT:
      json_builder_add_double_value (builder, ((PangoAttrFloat*)attr)->value);
    }

  json_builder_end_object (builder);
}

static void
add_attr_list (JsonBuilder   *builder,
               PangoAttrList *attrs)
{
  GSList *attributes, *l;

  if (!attrs)
    return;

  attributes = pango_attr_list_get_attributes (attrs);

  if (!attributes)
    return;

  json_builder_set_member_name (builder, "attributes");
  json_builder_begin_array (builder);

  for (l = attributes; l; l = l->next)
    {
      PangoAttribute *attr = l->data;
      add_attribute (builder, attr);
    }
  g_slist_free_full (attributes, (GDestroyNotify) pango_attribute_destroy);

  json_builder_end_array (builder);
}

static void
add_tab_array (JsonBuilder   *builder,
               PangoTabArray *tabs)
{
  if (!tabs || pango_tab_array_get_size (tabs) == 0)
    return;

  json_builder_set_member_name (builder, "tabs");

  json_builder_begin_object (builder);

  json_builder_set_member_name (builder, "positions-in-pixels");
  json_builder_add_boolean_value (builder, pango_tab_array_get_positions_in_pixels (tabs));
  json_builder_set_member_name (builder, "positions");
  json_builder_begin_array (builder);
  for (int i = 0; i < pango_tab_array_get_size (tabs); i++)
    {
      PangoTabAlign align;
      int pos;
      pango_tab_array_get_tab (tabs, i, &align, &pos);
      json_builder_begin_object (builder);
      json_builder_set_member_name (builder, "position");
      json_builder_add_int_value (builder, pos);
      json_builder_set_member_name (builder, "alignment");
      add_enum_value (builder, PANGO_TYPE_TAB_ALIGN, align, FALSE);
      json_builder_set_member_name (builder, "decimal-point");
      json_builder_add_int_value (builder, pango_tab_array_get_decimal_point (tabs, i));
      json_builder_end_object (builder);
    }
  json_builder_end_array (builder);

  json_builder_end_object (builder);
}

static void
add_context (JsonBuilder  *builder,
             PangoContext *context)
{
  char *str;
  const PangoMatrix *matrix;
  PangoMatrix identity = PANGO_MATRIX_INIT;

  json_builder_begin_object (builder);

  /* Note: since we don't create the context when deserializing,
   * we don't strip out default values here to ensure that the
   * context gets updated as expected.
   */

  str = pango_font_description_to_string (context->font_desc);
  json_builder_set_member_name (builder, "font");
  json_builder_add_string_value (builder, str);
  g_free (str);

  if (context->set_language)
    {
      json_builder_set_member_name (builder, "language");
      json_builder_add_string_value (builder, pango_language_to_string (context->set_language));
    }

  json_builder_set_member_name (builder, "base-gravity");
  add_enum_value (builder, PANGO_TYPE_GRAVITY, context->base_gravity, FALSE);

  json_builder_set_member_name (builder, "gravity-hint");
  add_enum_value (builder, PANGO_TYPE_GRAVITY_HINT, context->gravity_hint, FALSE);

  json_builder_set_member_name (builder, "base-dir");
  add_enum_value (builder, PANGO_TYPE_DIRECTION, context->base_dir, FALSE);

  json_builder_set_member_name (builder, "round-glyph-positions");
  json_builder_add_boolean_value (builder, context->round_glyph_positions);

  json_builder_set_member_name (builder, "transform");
  matrix = pango_context_get_matrix (context);
  if (!matrix)
    matrix = &identity;

  json_builder_begin_array (builder);
  json_builder_add_double_value (builder, matrix->xx);
  json_builder_add_double_value (builder, matrix->xy);
  json_builder_add_double_value (builder, matrix->yx);
  json_builder_add_double_value (builder, matrix->yy);
  json_builder_add_double_value (builder, matrix->x0);
  json_builder_add_double_value (builder, matrix->y0);
  json_builder_end_array (builder);

  json_builder_end_object (builder);
}

static void
add_log_attrs (JsonBuilder *builder,
               PangoLayout *layout)
{
  const PangoLogAttr *log_attrs;
  int n_attrs;

  json_builder_set_member_name (builder, "log-attrs");
  json_builder_begin_array (builder);

  log_attrs = pango_layout_get_log_attrs_readonly (layout, &n_attrs);
  for (int i = 0; i < n_attrs; i++)
    {
      json_builder_begin_object (builder);
      if (log_attrs[i].is_line_break)
        {
          json_builder_set_member_name (builder, "line-break");
          json_builder_add_boolean_value (builder, TRUE);
        }
      if (log_attrs[i].is_mandatory_break)
        {
          json_builder_set_member_name (builder, "mandatory-break");
          json_builder_add_boolean_value (builder, TRUE);
        }
      if (log_attrs[i].is_char_break)
        {
          json_builder_set_member_name (builder, "char-break");
          json_builder_add_boolean_value (builder, TRUE);
        }
      if (log_attrs[i].is_white)
        {
          json_builder_set_member_name (builder, "white");
          json_builder_add_boolean_value (builder, TRUE);
        }
      if (log_attrs[i].is_cursor_position)
        {
          json_builder_set_member_name (builder, "cursor-position");
          json_builder_add_boolean_value (builder, TRUE);
        }
      if (log_attrs[i].is_word_start)
        {
          json_builder_set_member_name (builder, "word-start");
          json_builder_add_boolean_value (builder, TRUE);
        }
      if (log_attrs[i].is_word_end)
        {
          json_builder_set_member_name (builder, "word-end");
          json_builder_add_boolean_value (builder, TRUE);
        }
      if (log_attrs[i].is_sentence_boundary)
        {
          json_builder_set_member_name (builder, "sentence-boundary");
          json_builder_add_boolean_value (builder, TRUE);
        }
      if (log_attrs[i].is_sentence_start)
        {
          json_builder_set_member_name (builder, "sentence-start");
          json_builder_add_boolean_value (builder, TRUE);
        }
      if (log_attrs[i].is_sentence_end)
        {
          json_builder_set_member_name (builder, "sentence-end");
          json_builder_add_boolean_value (builder, TRUE);
        }
      if (log_attrs[i].backspace_deletes_character)
        {
          json_builder_set_member_name (builder, "backspace-deletes-character");
          json_builder_add_boolean_value (builder, TRUE);
        }
      if (log_attrs[i].is_expandable_space)
        {
          json_builder_set_member_name (builder, "expandable-space");
          json_builder_add_boolean_value (builder, TRUE);
        }
      if (log_attrs[i].is_word_boundary)
        {
          json_builder_set_member_name (builder, "word-boundary");
          json_builder_add_boolean_value (builder, TRUE);
        }
      if (log_attrs[i].break_inserts_hyphen)
        {
          json_builder_set_member_name (builder, "break-inserts-hyphen");
          json_builder_add_boolean_value (builder, TRUE);
        }
      if (log_attrs[i].break_removes_preceding)
        {
          json_builder_set_member_name (builder, "break-removes_preceding");
          json_builder_add_boolean_value (builder, TRUE);
        }
      json_builder_end_object (builder);
    }

  json_builder_end_array (builder);
}

static void
add_font (JsonBuilder *builder,
          PangoFont   *font)
{
  PangoFontDescription *desc;
  char *str;
  hb_font_t *hb_font;
  hb_face_t *face;
  hb_blob_t *blob;
  const char *data;
  guint length;
  const int *coords;
  hb_feature_t features[32];
  PangoMatrix matrix;

  json_builder_begin_object (builder);

  json_builder_set_member_name (builder, "description");

  desc = pango_font_describe (font);
  str = pango_font_description_to_string (desc);
  json_builder_add_string_value (builder, str);
  g_free (str);
  pango_font_description_free (desc);

  hb_font = pango_font_get_hb_font (font);
  face = hb_font_get_face (hb_font);
  blob = hb_face_reference_blob (face);

  data = hb_blob_get_data (blob, &length);
  str = g_compute_checksum_for_data (G_CHECKSUM_SHA256, (const guchar *)data, length);

  json_builder_set_member_name (builder, "checksum");
  json_builder_add_string_value (builder, str);

  g_free (str);
  hb_blob_destroy (blob);

  coords = hb_font_get_var_coords_normalized (hb_font, &length);
  if (length > 0)
    {
      guint count;
      hb_ot_var_axis_info_t *axes;

      count = hb_ot_var_get_axis_count (face);
      g_assert (count == length);

      axes = g_alloca (count * sizeof (hb_ot_var_axis_info_t));
      hb_ot_var_get_axis_infos (face, 0, &count, axes);

      json_builder_set_member_name (builder, "variations");
      json_builder_begin_object (builder);

      for (int i = 0; i < length; i++)
        {
          char buf[5] = { 0, };

          hb_tag_to_string (axes[i].tag, buf);
          json_builder_set_member_name (builder, buf);
          json_builder_add_int_value (builder, coords[i]);
        }

      json_builder_end_object (builder);
    }

  length = 0;
  pango_font_get_features (font, features, G_N_ELEMENTS (features), &length);
  if (length > 0)
    {
      json_builder_set_member_name (builder, "features");
      json_builder_begin_object (builder);

      for (int i = 0; i < length; i++)
        {
          char buf[5] = { 0, };

          hb_tag_to_string (features[i].tag, buf);
          json_builder_set_member_name (builder, buf);
          json_builder_add_int_value (builder, features[i].value);
        }

      json_builder_end_object (builder);
    }

  pango_font_get_matrix (font, &matrix);
  if (memcmp (&matrix, &(PangoMatrix)PANGO_MATRIX_INIT, sizeof (PangoMatrix)) != 0)
    {
      json_builder_set_member_name (builder, "matrix");
      json_builder_begin_array (builder);
      json_builder_add_double_value (builder, matrix.xx);
      json_builder_add_double_value (builder, matrix.xy);
      json_builder_add_double_value (builder, matrix.yx);
      json_builder_add_double_value (builder, matrix.yy);
      json_builder_add_double_value (builder, matrix.x0);
      json_builder_add_double_value (builder, matrix.y0);
      json_builder_end_array (builder);
    }

  json_builder_end_object (builder);
}

#define ANALYSIS_FLAGS (PANGO_ANALYSIS_FLAG_CENTERED_BASELINE | \
                        PANGO_ANALYSIS_FLAG_IS_ELLIPSIS | \
                        PANGO_ANALYSIS_FLAG_NEED_HYPHEN)

static void
add_run (JsonBuilder    *builder,
         PangoLayout    *layout,
         PangoLayoutRun *run)
{
  json_builder_begin_object (builder);
  char *str;

  json_builder_set_member_name (builder, "offset");
  json_builder_add_int_value (builder, run->item->offset);

  json_builder_set_member_name (builder, "length");
  json_builder_add_int_value (builder, run->item->length);

  str = g_strndup (layout->text + run->item->offset, run->item->length);
  json_builder_set_member_name (builder, "text");
  json_builder_add_string_value (builder, str);
  g_free (str);

  json_builder_set_member_name (builder, "bidi-level");
  json_builder_add_int_value (builder, run->item->analysis.level);

  json_builder_set_member_name (builder, "gravity");
  add_enum_value (builder, PANGO_TYPE_GRAVITY, run->item->analysis.gravity, FALSE);

  json_builder_set_member_name (builder, "language");
  json_builder_add_string_value (builder, pango_language_to_string (run->item->analysis.language));

  json_builder_set_member_name (builder, "script");
  add_enum_value (builder, PANGO_TYPE_SCRIPT, run->item->analysis.script, FALSE);

  json_builder_set_member_name (builder, "font");
  add_font (builder, run->item->analysis.font);

  json_builder_set_member_name (builder, "flags");
  json_builder_add_int_value (builder, run->item->analysis.flags & ANALYSIS_FLAGS);

  if (run->item->analysis.extra_attrs)
    {
      GSList *l;

      json_builder_set_member_name (builder, "extra-attributes");

      json_builder_begin_array (builder);
      for (l = run->item->analysis.extra_attrs; l; l = l->next)
        {
          PangoAttribute *attr = l->data;
          add_attribute (builder, attr);
        }
      json_builder_end_array (builder);
    }

  json_builder_set_member_name (builder, "y-offset");
  json_builder_add_int_value (builder, run->y_offset);

  json_builder_set_member_name (builder, "start-x-offset");
  json_builder_add_int_value (builder, run->start_x_offset);

  json_builder_set_member_name (builder, "end-x-offset");
  json_builder_add_int_value (builder, run->end_x_offset);

  json_builder_set_member_name (builder, "glyphs");
  json_builder_begin_array (builder);
  for (int i = 0; i < run->glyphs->num_glyphs; i++)
    {
      json_builder_begin_object (builder);

      json_builder_set_member_name (builder, "glyph");
      json_builder_add_int_value (builder, run->glyphs->glyphs[i].glyph);

      json_builder_set_member_name (builder, "width");
      json_builder_add_int_value (builder, run->glyphs->glyphs[i].geometry.width);

      if (run->glyphs->glyphs[i].geometry.x_offset != 0)
        {
          json_builder_set_member_name (builder, "x-offset");
          json_builder_add_int_value (builder, run->glyphs->glyphs[i].geometry.x_offset);
        }

      if (run->glyphs->glyphs[i].geometry.y_offset != 0)
        {
          json_builder_set_member_name (builder, "y-offset");
          json_builder_add_int_value (builder, run->glyphs->glyphs[i].geometry.y_offset);
        }

      if (run->glyphs->glyphs[i].attr.is_cluster_start)
        {
          json_builder_set_member_name (builder, "is-cluster-start");
          json_builder_add_boolean_value (builder, TRUE);
        }

      if (run->glyphs->glyphs[i].attr.is_color)
        {
          json_builder_set_member_name (builder, "is-color");
          json_builder_add_boolean_value (builder, TRUE);
        }

      json_builder_set_member_name (builder, "log-cluster");
      json_builder_add_int_value (builder, run->glyphs->log_clusters[i]);

      json_builder_end_object (builder);
    }

  json_builder_end_array (builder);

  json_builder_end_object (builder);
}

#undef ANALYSIS_FLAGS

static void
add_line (JsonBuilder     *builder,
          PangoLayoutLine *line)
{
  json_builder_begin_object (builder);

  json_builder_set_member_name (builder, "start-index");
  json_builder_add_int_value (builder, line->start_index);

  json_builder_set_member_name (builder, "length");
  json_builder_add_int_value (builder, line->length);

  json_builder_set_member_name (builder, "paragraph-start");
  json_builder_add_boolean_value (builder, line->is_paragraph_start);

  json_builder_set_member_name (builder, "direction");
  add_enum_value (builder, PANGO_TYPE_DIRECTION, line->resolved_dir, FALSE);

  json_builder_set_member_name (builder, "runs");
  json_builder_begin_array (builder);
  for (GSList *l = line->runs; l; l = l->next)
    {
      PangoLayoutRun *run = l->data;
      add_run (builder, line->layout, run);
    }
  json_builder_end_array (builder);

  json_builder_end_object (builder);
}

static void
add_output (JsonBuilder *builder,
            PangoLayout *layout)
{
  int width, height;

  json_builder_begin_object (builder);

  json_builder_set_member_name (builder, "is-wrapped");
  json_builder_add_boolean_value (builder, pango_layout_is_wrapped (layout));

  json_builder_set_member_name (builder, "is-ellipsized");
  json_builder_add_boolean_value (builder, pango_layout_is_ellipsized (layout));

  json_builder_set_member_name (builder, "unknown-glyphs");
  json_builder_add_int_value (builder, pango_layout_get_unknown_glyphs_count (layout));

  pango_layout_get_size (layout, &width, &height);
  json_builder_set_member_name (builder, "width");
  json_builder_add_int_value (builder, width);
  json_builder_set_member_name (builder, "height");
  json_builder_add_int_value (builder, height);

  add_log_attrs (builder, layout);
  json_builder_set_member_name (builder, "lines");
  json_builder_begin_array (builder);
  for (GSList *l = layout->lines; l; l = l->next)
    {
      PangoLayoutLine *line = l->data;
      add_line (builder, line);
    }
  json_builder_end_array (builder);

  json_builder_end_object (builder);
}

static JsonNode *
layout_to_json (PangoLayout               *layout,
                PangoLayoutSerializeFlags  flags)
{
  JsonBuilder *builder;
  JsonNode *root;
  const char *str;

  builder = json_builder_new_immutable ();

  json_builder_begin_object (builder);

  if (flags & PANGO_LAYOUT_SERIALIZE_CONTEXT)
    {
      json_builder_set_member_name (builder, "context");
      add_context (builder, layout->context);
    }

  str = (const char *) g_object_get_data (G_OBJECT (layout), "comment");
  if (str)
    {
      json_builder_set_member_name (builder, "comment");
      if (strstr (str, "\n") != NULL)
        {
          char **strs = g_strsplit (str, "\n", -1);

          json_builder_begin_array (builder);
          for (int i = 0; strs[i]; i++)
            json_builder_add_string_value (builder, strs[i]);
          json_builder_end_array (builder);

          g_strfreev (strs);
        }
      else
        json_builder_add_string_value (builder, str);
    }

  json_builder_set_member_name (builder, "text");
  json_builder_add_string_value (builder, layout->text);

  add_attr_list (builder, layout->attrs);

  if (layout->font_desc)
    {
      char *str = pango_font_description_to_string (layout->font_desc);
      json_builder_set_member_name (builder, "font");
      json_builder_add_string_value (builder, str);
      g_free (str);
    }

  add_tab_array (builder, layout->tabs);

  if (layout->justify)
    {
      json_builder_set_member_name (builder, "justify");
      json_builder_add_boolean_value (builder, TRUE);
    }

  if (layout->justify_last_line)
    {
      json_builder_set_member_name (builder, "justify-last-line");
      json_builder_add_boolean_value (builder, TRUE);
    }

  if (layout->single_paragraph)
    {
      json_builder_set_member_name (builder, "single-paragraph");
      json_builder_add_boolean_value (builder, TRUE);
    }

  if (!layout->auto_dir)
    {
      json_builder_set_member_name (builder, "auto-dir");
      json_builder_add_boolean_value (builder, FALSE);
    }

  if (layout->alignment != PANGO_ALIGN_LEFT)
    {
      json_builder_set_member_name (builder, "alignment");
      add_enum_value (builder, PANGO_TYPE_ALIGNMENT, layout->alignment, FALSE);
    }

  if (layout->wrap != PANGO_WRAP_WORD)
    {
      json_builder_set_member_name (builder, "wrap");
      add_enum_value (builder, PANGO_TYPE_WRAP_MODE, layout->wrap, FALSE);
    }

  if (layout->ellipsize != PANGO_ELLIPSIZE_NONE)
    {
      json_builder_set_member_name (builder, "ellipsize");
      add_enum_value (builder, PANGO_TYPE_ELLIPSIZE_MODE, layout->ellipsize, FALSE);
    }

  if (layout->width != -1)
    {
      json_builder_set_member_name (builder, "width");
      json_builder_add_int_value (builder, layout->width);
    }

  if (layout->height != -1)
    {
      json_builder_set_member_name (builder, "height");
      json_builder_add_int_value (builder, layout->height);
    }

  if (layout->indent != 0)
    {
      json_builder_set_member_name (builder, "indent");
      json_builder_add_int_value (builder, layout->indent);
    }

  if (layout->spacing != 0)
    {
      json_builder_set_member_name (builder, "spacing");
      json_builder_add_int_value (builder, layout->spacing);
    }

  if (layout->line_spacing != 0.)
    {
      json_builder_set_member_name (builder, "line-spacing");
      json_builder_add_double_value (builder, layout->line_spacing);
    }

  if (flags & PANGO_LAYOUT_SERIALIZE_OUTPUT)
    {
      json_builder_set_member_name (builder, "output");
      add_output (builder, layout);
    }

  json_builder_end_object (builder);

  root = json_builder_get_root (builder);
  g_object_unref (builder);

  return root;
}

static JsonNode *
font_to_json (PangoFont *font)
{
  JsonBuilder *builder;
  JsonNode *root;

  builder = json_builder_new_immutable ();
  add_font (builder, font);
  root = json_builder_get_root (builder);
  g_object_unref (builder);

  return root;
}

/* }}} */
/* {{{ Deserialization */

static int
get_enum_value (GType        type,
                const char  *str,
                gboolean     allow_extra,
                GError     **error)
{
  GEnumClass *enum_class;
  GEnumValue *enum_value;

  enum_class = g_type_class_ref (type);
  enum_value = g_enum_get_value_by_nick (enum_class, str);

  if (enum_value)
    return enum_value->value;

  if (allow_extra)
    {
      gint64 value;
      char *endp;

      value = g_ascii_strtoll (str, &endp, 10);
      if (*endp == '\0')
        return value;
    }

  g_set_error (error,
               PANGO_LAYOUT_DESERIALIZE_ERROR,
               PANGO_LAYOUT_DESERIALIZE_INVALID_VALUE,
               "Could not parse enum value of type %s: %s",
               g_type_name (type),
               str);

  return -1;
}

static PangoAttribute *
json_to_attribute (JsonReader  *reader,
                   GError     **error)
{
  PangoAttribute *attr = NULL;
  PangoAttrType type = PANGO_ATTR_INVALID;
  guint start = PANGO_ATTR_INDEX_FROM_TEXT_BEGINNING;
  guint end = PANGO_ATTR_INDEX_TO_TEXT_END;
  PangoFontDescription *desc;
  PangoColor color;
  int value;

  if (!json_reader_is_object (reader))
    {
      g_set_error (error,
                   PANGO_LAYOUT_DESERIALIZE_ERROR,
                   PANGO_LAYOUT_DESERIALIZE_INVALID_SYNTAX,
                   "Attribute must be a Json object");
      return NULL;
    }

  if (json_reader_read_member (reader, "start"))
    start = json_reader_get_int_value (reader);
  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "end"))
    end = json_reader_get_int_value (reader);
  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "type"))
    {
      type = get_enum_value (PANGO_TYPE_ATTR_TYPE, json_reader_get_string_value (reader), FALSE, error);
      if (type == -1)
        return NULL;
    }
  else
    {
      g_set_error (error,
                   PANGO_LAYOUT_DESERIALIZE_ERROR,
                   PANGO_LAYOUT_DESERIALIZE_MISSING_VALUE,
                   "Attribute \"type\" missing");
      return NULL;
    }
  json_reader_end_member (reader);

  if (!json_reader_read_member (reader, "value"))
    {
      g_set_error (error,
                   PANGO_LAYOUT_DESERIALIZE_ERROR,
                   PANGO_LAYOUT_DESERIALIZE_MISSING_VALUE,
                   "Attribute \"value\" missing");
      return NULL;
    }

  switch (type)
    {
    default:
    case PANGO_ATTR_INVALID:
      g_assert_not_reached ();

    case PANGO_ATTR_LANGUAGE:
      attr = pango_attr_language_new (pango_language_from_string (json_reader_get_string_value (reader)));
      break;
    case PANGO_ATTR_FAMILY:
      attr = pango_attr_family_new (json_reader_get_string_value (reader));
      break;
    case PANGO_ATTR_STYLE:
      value = get_enum_value (PANGO_TYPE_STYLE, json_reader_get_string_value (reader), FALSE, error);
      if (value == -1)
        return NULL;
      attr = pango_attr_style_new ((PangoStyle)value);
      break;
    case PANGO_ATTR_WEIGHT:
      value = get_enum_value (PANGO_TYPE_WEIGHT, json_reader_get_string_value (reader), TRUE, error);
      if (value == -1)
        return NULL;
      attr = pango_attr_weight_new (value);
      break;
    case PANGO_ATTR_VARIANT:
      value = get_enum_value (PANGO_TYPE_VARIANT, json_reader_get_string_value (reader), FALSE, error);
      if (value == -1)
        return NULL;
      attr = pango_attr_variant_new ((PangoVariant)value);
      break;
    case PANGO_ATTR_STRETCH:
      value = get_enum_value (PANGO_TYPE_STRETCH, json_reader_get_string_value (reader), FALSE, error);
      if (value == -1)
      attr = pango_attr_stretch_new ((PangoStretch)value);
      break;
    case PANGO_ATTR_SIZE:
      attr = pango_attr_size_new (json_reader_get_int_value (reader));
      break;
    case PANGO_ATTR_FONT_DESC:
      desc = pango_font_description_from_string (json_reader_get_string_value (reader));
      attr = pango_attr_font_desc_new (desc);
      pango_font_description_free (desc);
      break;
    case PANGO_ATTR_FOREGROUND:
      pango_color_parse (&color, json_reader_get_string_value (reader));
      attr = pango_attr_foreground_new (color.red, color.green, color.blue);
      break;
    case PANGO_ATTR_BACKGROUND:
      pango_color_parse (&color, json_reader_get_string_value (reader));
      attr = pango_attr_background_new (color.red, color.green, color.blue);
      break;
    case PANGO_ATTR_UNDERLINE:
      value = get_enum_value (PANGO_TYPE_UNDERLINE, json_reader_get_string_value (reader), FALSE, error);
      if (value == -1)
        return NULL;
      attr = pango_attr_underline_new ((PangoUnderline)value);
      break;
    case PANGO_ATTR_STRIKETHROUGH:
      attr = pango_attr_strikethrough_new (json_reader_get_boolean_value (reader));
      break;
    case PANGO_ATTR_RISE:
      attr = pango_attr_rise_new (json_reader_get_int_value (reader));
      break;
    case PANGO_ATTR_SHAPE:
      /* FIXME */
      attr = pango_attr_shape_new (&(PangoRectangle) { 0, 0, 0, 0}, &(PangoRectangle) { 0, 0, 0, 0});
      break;
    case PANGO_ATTR_SCALE:
      attr = pango_attr_scale_new (json_reader_get_double_value (reader));
      break;
    case PANGO_ATTR_FALLBACK:
      attr = pango_attr_fallback_new (json_reader_get_boolean_value (reader));
      break;
    case PANGO_ATTR_LETTER_SPACING:
      attr = pango_attr_letter_spacing_new (json_reader_get_int_value (reader));
      break;
    case PANGO_ATTR_UNDERLINE_COLOR:
      pango_color_parse (&color, json_reader_get_string_value (reader));
      attr = pango_attr_underline_color_new (color.red, color.green, color.blue);
      break;
    case PANGO_ATTR_STRIKETHROUGH_COLOR:
      pango_color_parse (&color, json_reader_get_string_value (reader));
      attr = pango_attr_strikethrough_color_new (color.red, color.green, color.blue);
      break;
    case PANGO_ATTR_ABSOLUTE_SIZE:
      attr = pango_attr_size_new_absolute (json_reader_get_int_value (reader));
      break;
    case PANGO_ATTR_GRAVITY:
      value = get_enum_value (PANGO_TYPE_GRAVITY, json_reader_get_string_value (reader), FALSE, error);
      if (value == -1)
        return NULL;
      attr = pango_attr_gravity_new ((PangoGravity)value);
      break;
    case PANGO_ATTR_GRAVITY_HINT:
      value = get_enum_value (PANGO_TYPE_GRAVITY_HINT, json_reader_get_string_value (reader), FALSE, error);
      if (value == -1)
        return NULL;
      attr = pango_attr_gravity_hint_new ((PangoGravityHint)value);
      break;
    case PANGO_ATTR_FONT_FEATURES:
      attr = pango_attr_font_features_new (json_reader_get_string_value (reader));
      break;
    case PANGO_ATTR_FOREGROUND_ALPHA:
      attr = pango_attr_foreground_alpha_new (json_reader_get_int_value (reader));
      break;
    case PANGO_ATTR_BACKGROUND_ALPHA:
      attr = pango_attr_background_alpha_new (json_reader_get_int_value (reader));
      break;
    case PANGO_ATTR_ALLOW_BREAKS:
      attr = pango_attr_allow_breaks_new (json_reader_get_boolean_value (reader));
      break;
    case PANGO_ATTR_SHOW:
      attr = pango_attr_show_new (json_reader_get_int_value (reader));
      break;
    case PANGO_ATTR_INSERT_HYPHENS:
      attr = pango_attr_insert_hyphens_new (json_reader_get_boolean_value (reader));
      break;
    case PANGO_ATTR_OVERLINE:
      value = get_enum_value (PANGO_TYPE_OVERLINE, json_reader_get_string_value (reader), FALSE, error);
      if (value == -1)
        return NULL;
      attr = pango_attr_overline_new ((PangoOverline)value);
      break;
    case PANGO_ATTR_OVERLINE_COLOR:
      pango_color_parse (&color, json_reader_get_string_value (reader));
      attr = pango_attr_overline_color_new (color.red, color.green, color.blue);
      break;
    case PANGO_ATTR_LINE_HEIGHT:
      attr = pango_attr_line_height_new (json_reader_get_double_value (reader));
      break;
    case PANGO_ATTR_ABSOLUTE_LINE_HEIGHT:
      attr = pango_attr_line_height_new_absolute (json_reader_get_int_value (reader));
      break;
    case PANGO_ATTR_TEXT_TRANSFORM:
      value = get_enum_value (PANGO_TYPE_TEXT_TRANSFORM, json_reader_get_string_value (reader), FALSE, error);
      if (value == -1)
        return NULL;
      attr = pango_attr_text_transform_new ((PangoTextTransform)value);
      break;
    case PANGO_ATTR_WORD:
      attr = pango_attr_word_new ();
      break;
    case PANGO_ATTR_SENTENCE:
      attr = pango_attr_sentence_new ();
      break;
    case PANGO_ATTR_BASELINE_SHIFT:
      value = get_enum_value (PANGO_TYPE_BASELINE_SHIFT, json_reader_get_string_value (reader), TRUE, error);
      if (value == -1)
        return NULL;
      attr = pango_attr_baseline_shift_new (value);
      break;
    case PANGO_ATTR_FONT_SCALE:
      value = get_enum_value (PANGO_TYPE_FONT_SCALE, json_reader_get_string_value (reader), FALSE, error);
      if (value == -1)
        return NULL;
      attr = pango_attr_font_scale_new ((PangoFontScale)value);
      break;
    }

  attr->start_index = start;
  attr->end_index = end;

  json_reader_end_member (reader);

  return attr;
}

static PangoAttrList *
json_to_attr_list (JsonReader  *reader,
                   GError     **error)
{
  PangoAttrList *attributes;

  attributes = pango_attr_list_new ();

  if (!json_reader_is_array (reader))
    {
      g_set_error (error,
                   PANGO_LAYOUT_DESERIALIZE_ERROR,
                   PANGO_LAYOUT_DESERIALIZE_INVALID_SYNTAX,
                   "\"attributes\" must be a Json array");
      goto fail;
    }

  for (int i = 0; i < json_reader_count_elements (reader); i++)
    {
      PangoAttribute *attr;
      json_reader_read_element (reader, i);
      attr = json_to_attribute (reader, error);
      if (!attr)
        goto fail;
      pango_attr_list_insert (attributes, attr);
      json_reader_end_element (reader);
    }

  return attributes;

fail:
  if (attributes)
    pango_attr_list_unref (attributes);
  return NULL;
}

static PangoTabArray *
json_to_tab_array (JsonReader  *reader,
                   GError     **error)
{
  PangoTabArray *tabs;
  gboolean positions_in_pixels = FALSE;

  if (json_reader_read_member (reader, "positions-in-pixels"))
    positions_in_pixels = json_reader_get_boolean_value (reader);
  json_reader_end_member (reader);

  tabs = pango_tab_array_new (0, positions_in_pixels);

  if (json_reader_read_member (reader, "positions"))
    {
      if (!json_reader_is_array (reader))
        {
          g_set_error (error,
                       PANGO_LAYOUT_DESERIALIZE_ERROR,
                       PANGO_LAYOUT_DESERIALIZE_INVALID_SYNTAX,
                       "Tab \"positions\"  must be a Json array");
          goto fail;
        }

      pango_tab_array_resize (tabs, json_reader_count_elements (reader));
      for (int i = 0; i < json_reader_count_elements (reader); i++)
        {
          int pos;
          PangoTabAlign align = PANGO_TAB_LEFT;
          gunichar ch = 0;

          json_reader_read_element (reader, i);
          if (json_reader_is_object (reader))
            {
              json_reader_read_member (reader, "position");
              pos = json_reader_get_int_value (reader);
              json_reader_end_member (reader);
              json_reader_read_member (reader, "alignment");

              align = get_enum_value (PANGO_TYPE_TAB_ALIGN,
                                      json_reader_get_string_value (reader),
                                      FALSE,
                                      error);
              if (align == -1)
                goto fail;
              json_reader_end_member (reader);
              json_reader_read_member (reader, "decimal-point");
              ch = json_reader_get_int_value (reader);
              json_reader_end_member (reader);
            }
          else
            {
              pos = json_reader_get_int_value (reader);
            }

          pango_tab_array_set_tab (tabs, i, align, pos);
          pango_tab_array_set_decimal_point (tabs, i, ch);
          json_reader_end_element (reader);
        }
    }
  json_reader_end_member (reader);

  return tabs;

fail:
  if (tabs)
    pango_tab_array_free (tabs);
  return NULL;
}

static gboolean
apply_json_to_context (JsonReader    *reader,
                       PangoContext  *context,
                       GError       **error)
{
  if (json_reader_read_member (reader, "language"))
    {
      const char *value;
      PangoLanguage *language;

      value = json_reader_get_string_value (reader);
      language = pango_language_from_string (value);
      pango_context_set_language (context, language);
    }
  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "font"))
    {
      PangoFontDescription *desc;

      desc = pango_font_description_from_string (json_reader_get_string_value (reader));
      pango_context_set_font_description (context, desc);
      pango_font_description_free (desc);
    }
  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "base-gravity"))
    {
      PangoGravity gravity = get_enum_value (PANGO_TYPE_GRAVITY,
                                             json_reader_get_string_value (reader),
                                             FALSE,
                                             error);
      if (gravity == -1)
        return FALSE;

      pango_context_set_base_gravity (context, gravity);
    }
  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "gravity-hint"))
    {
      PangoGravityHint gravity_hint = get_enum_value (PANGO_TYPE_GRAVITY_HINT,
                                                      json_reader_get_string_value (reader),
                                                      FALSE,
                                                      error);
      if (gravity_hint == -1)
        return FALSE;

      pango_context_set_gravity_hint (context, gravity_hint);
    }
  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "base-dir"))
    {
      PangoDirection direction = get_enum_value (PANGO_TYPE_DIRECTION,
                                                 json_reader_get_string_value (reader),
                                                 FALSE,
                                                 error);
      if (direction == -1)
        return FALSE;

      pango_context_set_base_dir (context, direction);
    }
  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "round-glyph-positions"))
    {
      pango_context_set_round_glyph_positions (context, json_reader_get_boolean_value (reader));
    }
  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "transform"))
    {
      PangoMatrix m;

      json_reader_read_element (reader, 0);
      m.xx = json_reader_get_double_value (reader);
      json_reader_end_element (reader);
      json_reader_read_element (reader, 1);
      m.xy = json_reader_get_double_value (reader);
      json_reader_end_element (reader);
      json_reader_read_element (reader, 2);
      m.yx = json_reader_get_double_value (reader);
      json_reader_end_element (reader);
      json_reader_read_element (reader, 3);
      m.yy = json_reader_get_double_value (reader);
      json_reader_end_element (reader);
      json_reader_read_element (reader, 4);
      m.x0 = json_reader_get_double_value (reader);
      json_reader_end_element (reader);
      json_reader_read_element (reader, 5);
      m.y0 = json_reader_get_double_value (reader);
      json_reader_end_element (reader);

      pango_context_set_matrix (context, &m);
    }
  json_reader_end_member (reader);

  return TRUE;
}

static PangoLayout *
json_to_layout (PangoContext                 *context,
                JsonNode                     *node,
                PangoLayoutDeserializeFlags   flags,
                GError                      **error)
{
  JsonReader *reader;
  PangoLayout *layout;

  reader = json_reader_new (node);
  if (!json_reader_is_object (reader))
    {
      g_set_error (error,
                   PANGO_LAYOUT_DESERIALIZE_ERROR,
                   PANGO_LAYOUT_DESERIALIZE_INVALID_SYNTAX,
                   "Layout must be a Json object");
      goto fail;
    }

  if (flags & PANGO_LAYOUT_DESERIALIZE_CONTEXT)
    {
      if (json_reader_read_member (reader, "context"))
        {
          if (!apply_json_to_context (reader, context, error))
            goto fail;
        }
      json_reader_end_member (reader);
    }

  layout = pango_layout_new (context);

  if (json_reader_read_member (reader, "comment"))
    {
      if (json_reader_is_array (reader))
        {
          GString *s;

          s = g_string_new ("");
          for (int i = 0; i < json_reader_count_elements (reader); i++)
            {
              json_reader_read_element (reader, i);
              if (s->len > 0)
                g_string_append_c (s, '\n');
              g_string_append (s, json_reader_get_string_value (reader));
              json_reader_end_element (reader);
            }

          g_object_set_data_full (G_OBJECT (layout), "comment",
                                  g_string_free (s, FALSE),
                                  g_free);
        }
      else
        g_object_set_data_full (G_OBJECT (layout), "comment",
                                g_strdup (json_reader_get_string_value (reader)),
                                g_free);
    }
  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "text"))
    pango_layout_set_text (layout, json_reader_get_string_value (reader), -1);
  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "attributes"))
    {
      PangoAttrList *attributes;

      attributes = json_to_attr_list (reader, error);

      if (!attributes)
        goto fail;

      pango_layout_set_attributes (layout, attributes);
      pango_attr_list_unref (attributes);
    }
  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "font"))
    {
      PangoFontDescription *desc;

      desc = pango_font_description_from_string ( json_reader_get_string_value (reader));
      if (!desc)
        {
          g_set_error (error,
                       PANGO_LAYOUT_DESERIALIZE_ERROR,
                       PANGO_LAYOUT_DESERIALIZE_INVALID_VALUE,
                       "Could not parse \"%s\" value: %s",
                       "font",
                       json_reader_get_string_value (reader));
          goto fail;
        }
      pango_layout_set_font_description (layout, desc);
      pango_font_description_free (desc);
    }
  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "tabs"))
    {
      PangoTabArray *tabs;

      tabs = json_to_tab_array (reader, error);

      if (!tabs)
        goto fail;

      pango_layout_set_tabs (layout, tabs);
      pango_tab_array_free (tabs);
    }
  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "justify"))
    pango_layout_set_justify (layout, json_reader_get_boolean_value (reader));
  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "justify-last-line"))
    pango_layout_set_justify_last_line (layout, json_reader_get_boolean_value (reader));
  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "single-paragraph"))
    pango_layout_set_single_paragraph_mode (layout, json_reader_get_boolean_value (reader));
  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "auto-dir"))
    pango_layout_set_auto_dir (layout, json_reader_get_boolean_value (reader));
  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "alignment"))
    {
      PangoAlignment align = get_enum_value (PANGO_TYPE_ALIGNMENT,
                                             json_reader_get_string_value (reader),
                                             FALSE,
                                             error);
      if (align == -1)
        goto fail;

      pango_layout_set_alignment (layout, align);
    }

  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "wrap"))
    {
      PangoWrapMode wrap = get_enum_value (PANGO_TYPE_WRAP_MODE,
                                           json_reader_get_string_value (reader),
                                           FALSE,
                                           error);

      if (wrap == -1)
        goto fail;

      pango_layout_set_wrap (layout, wrap);
    }

  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "ellipsize"))
    {
      PangoEllipsizeMode ellipsize = get_enum_value (PANGO_TYPE_ELLIPSIZE_MODE,
                                                     json_reader_get_string_value (reader),
                                                     FALSE,
                                                     error);

      if (ellipsize == -1)
        goto fail;

      pango_layout_set_ellipsize (layout, ellipsize);
    }

  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "width"))
    pango_layout_set_width (layout, json_reader_get_int_value (reader));
  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "height"))
    pango_layout_set_height (layout, json_reader_get_int_value (reader));
  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "indent"))
    pango_layout_set_indent (layout, json_reader_get_int_value (reader));
  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "spacing"))
    pango_layout_set_spacing (layout, json_reader_get_int_value (reader));
  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "line-spacing"))
    pango_layout_set_line_spacing (layout, json_reader_get_double_value (reader));
  json_reader_end_member (reader);

  g_object_unref (reader);

  return layout;

fail:
  g_object_unref (reader);
  if (layout)
    g_object_unref (layout);
  return NULL;
}

/* }}} */
/* {{{ Public API */

/**
 * pango_layout_serialize:
 * @layout: a `PangoLayout`
 * @flags: `PangoLayoutSerializeFlags`
 *
 * Serializes the @layout for later deserialization via [method@Pango.Layout.deserialize].
 *
 * There are no guarantees about the format of the output across different
 * versions of Pango and [method@Pango.Layout.deserialize] will reject data
 * that it cannot parse.
 *
 * The intended use of this function is testing, benchmarking and debugging.
 * The format is not meant as a permanent storage format.
 *
 * Returns: a `GBytes` containing the serialized form of @layout
 *
 * Since: 1.50
 */
GBytes *
pango_layout_serialize (PangoLayout               *layout,
                        PangoLayoutSerializeFlags  flags)
{
  JsonGenerator *generator;
  JsonNode *node;
  GString *str;
  char *data;
  gsize size;

  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), NULL);

  node = layout_to_json (layout, flags);

  generator = json_generator_new ();
  json_generator_set_pretty (generator, TRUE);
  json_generator_set_indent (generator, 2);

  json_generator_set_root (generator, node);
  str = g_string_new ("");
  g_string_append_c (json_generator_to_gstring (generator, str), '\n');
  size = str->len;
  data = g_string_free (str, FALSE);

  json_node_free (node);
  g_object_unref (generator);

  return g_bytes_new_take (data, size);
}

/**
 * pango_layout_write_to_file:
 * @layout: a `PangoLayout`
 * @flags: `PangoLayoutSerializeFlags`
 * @filename: (type filename): the file to save it to
 * @error: Return location for a potential error
 *
 * This function is equivalent to calling [method@Pango.Layout.serialize]
 * followed by g_file_set_contents().
 *
 * See those two functions for details on the arguments.
 *
 * It is mostly intended for use inside a debugger to quickly dump
 * a layout to a file for later inspection.
 *
 * Returns: %TRUE if saving was successful
 *
 * Since: 1.50
 */
gboolean
pango_layout_write_to_file (PangoLayout                *layout,
                            PangoLayoutSerializeFlags   flags,
                            const char                 *filename,
                            GError                    **error)
{
  GBytes *bytes;
  gboolean result;

  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  bytes = pango_layout_serialize (layout, flags);
  result = g_file_set_contents (filename,
                                g_bytes_get_data (bytes, NULL),
                                g_bytes_get_size (bytes),
                                error);
  g_bytes_unref (bytes);

  return result;

}

/**
 * pango_layout_deserialize:
 * @context: a `PangoContext`
 * @flags: `PangoLayoutDeserializeFlags`
 * @bytes: the bytes containing the data
 * @error: return location for an error
 *
 * Loads data previously created via [method@Pango.Layout.serialize].
 *
 * For a discussion of the supported format, see that function.
 *
 * Returns: (nullable) (transfer full): a new `PangoLayout`
 *
 * Since: 1.50
 */
PangoLayout *
pango_layout_deserialize (PangoContext                 *context,
                          GBytes                       *bytes,
                          PangoLayoutDeserializeFlags   flags,
                          GError                      **error)
{
  JsonParser *parser;
  JsonNode *node;
  PangoLayout *layout;

  g_return_val_if_fail (PANGO_IS_CONTEXT (context), NULL);

  parser = json_parser_new_immutable ();
  if (!json_parser_load_from_data (parser,
                                   g_bytes_get_data (bytes, NULL),
                                   g_bytes_get_size (bytes),
                                   error))
    {
      g_object_unref (parser);
      return NULL;
    }

  node = json_parser_get_root (parser);
  layout = json_to_layout (context, node, flags, error);

  g_object_unref (parser);

  return layout;
}

/**
 * pango_font_serialize:
 * @font: a `PangoFont`
 *
 * Serializes the @font in a way that can be uniquely identified.
 *
 * There are no guarantees about the format of the output across different
 * versions of Pango.
 *
 * The intended use of this function is testing, benchmarking and debugging.
 * The format is not meant as a permanent storage format.
 *
 * Returns: a `GBytes` containing the serialized form of @font
 *
 * Since: 1.50
 */
GBytes *
pango_font_serialize (PangoFont *font)
{
  JsonGenerator *generator;
  JsonNode *node;
  char *data;
  gsize size;

  g_return_val_if_fail (PANGO_IS_FONT (font), NULL);

  node = font_to_json (font);

  generator = json_generator_new ();
  json_generator_set_pretty (generator, TRUE);
  json_generator_set_indent (generator, 2);

  json_generator_set_root (generator, node);
  data = json_generator_to_data (generator, &size);

  json_node_free (node);
  g_object_unref (generator);

  return g_bytes_new_take (data, size);
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
