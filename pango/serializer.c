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
#include <pango/pango-enum-types.h>

#include <json-glib/json-glib.h>

/* {{{ Error handling */

G_DEFINE_QUARK(pango-layout-serialize-error-quark, pango_layout_serialize_error)

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

  json_builder_begin_array (builder);

  attributes = pango_attr_list_get_attributes (attrs);
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
  json_builder_begin_object (builder);

  json_builder_set_member_name (builder, "positions-in-pixels");
  json_builder_add_boolean_value (builder, pango_tab_array_get_positions_in_pixels (tabs));
  json_builder_set_member_name (builder, "positions");
  json_builder_begin_array (builder);
  for (int i = 0; i < pango_tab_array_get_size (tabs); i++)
    {
      int pos;
      pango_tab_array_get_tab (tabs, i, NULL, &pos);
      json_builder_add_int_value (builder, pos);
    }
  json_builder_end_array (builder);

  json_builder_end_object (builder);
}

static JsonNode *
layout_to_json (PangoLayout *layout)
{
  JsonBuilder *builder;
  JsonNode *root;

  builder = json_builder_new_immutable ();

  json_builder_begin_object (builder);

  json_builder_set_member_name (builder, "text");
  json_builder_add_string_value (builder, layout->text);

  if (layout->attrs)
    {
      json_builder_set_member_name (builder, "attributes");
      add_attr_list (builder, layout->attrs);
    }

  if (layout->font_desc)
    {
      char *str = pango_font_description_to_string (layout->font_desc);
      json_builder_set_member_name (builder, "font");
      json_builder_add_string_value (builder, str);
      g_free (str);
    }

  if (layout->tabs)
    {
      json_builder_set_member_name (builder, "tabs");
      add_tab_array (builder, layout->tabs);
    }

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

  json_builder_end_object (builder);

  root = json_builder_get_root (builder);
  g_object_unref (builder);

  return root;
}

/* }}} */
/* {{{ Deserialization */

static int
get_enum_value (GType       type,
                const char *str,
                gboolean    allow_extra)
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

  if (!json_reader_is_object (reader))
    {
      g_set_error (error,
                   PANGO_LAYOUT_SERIALIZE_ERROR, PANGO_LAYOUT_SERIALIZE_INVALID_SYNTAX,
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
      type = get_enum_value (PANGO_TYPE_ATTR_TYPE, json_reader_get_string_value (reader), FALSE);
      if (type == -1 || type == PANGO_ATTR_INVALID)
        {
          g_set_error (error,
                       PANGO_LAYOUT_SERIALIZE_ERROR, PANGO_LAYOUT_SERIALIZE_INVALID_VALUE,
                       "Attribute \"type\" invalid: %s",
                       json_reader_get_string_value (reader));
          return NULL;
        }
      json_reader_end_member (reader);
    }
  else
    {
      g_set_error (error,
                   PANGO_LAYOUT_SERIALIZE_ERROR, PANGO_LAYOUT_SERIALIZE_MISSING_VALUE,
                   "Attribute \"type\" missing");
      json_reader_end_member (reader);
      return NULL;
    }

  if (!json_reader_read_member (reader, "value"))
    {
      g_set_error (error,
                   PANGO_LAYOUT_SERIALIZE_ERROR, PANGO_LAYOUT_SERIALIZE_MISSING_VALUE,
                   "Attribute \"value\" missing");
      json_reader_end_member (reader);
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
      attr = pango_attr_style_new (get_enum_value (PANGO_TYPE_STYLE, json_reader_get_string_value (reader), FALSE));
      break;
    case PANGO_ATTR_WEIGHT:
      attr = pango_attr_weight_new (get_enum_value (PANGO_TYPE_WEIGHT, json_reader_get_string_value (reader), TRUE));
      break;
    case PANGO_ATTR_VARIANT:
      attr = pango_attr_variant_new (get_enum_value (PANGO_TYPE_VARIANT, json_reader_get_string_value (reader), FALSE));
      break;
    case PANGO_ATTR_STRETCH:
      attr = pango_attr_stretch_new (get_enum_value (PANGO_TYPE_STRETCH, json_reader_get_string_value (reader), FALSE));
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
      attr = pango_attr_underline_new (get_enum_value (PANGO_TYPE_UNDERLINE, json_reader_get_string_value (reader), FALSE));
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
      attr = pango_attr_gravity_new (get_enum_value (PANGO_TYPE_GRAVITY, json_reader_get_string_value (reader), FALSE));
      break;
    case PANGO_ATTR_GRAVITY_HINT:
      attr = pango_attr_gravity_hint_new (get_enum_value (PANGO_TYPE_GRAVITY_HINT, json_reader_get_string_value (reader), FALSE));
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
      attr = pango_attr_overline_new (get_enum_value (PANGO_TYPE_OVERLINE, json_reader_get_string_value (reader), FALSE));
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
      attr = pango_attr_text_transform_new (get_enum_value (PANGO_TYPE_TEXT_TRANSFORM, json_reader_get_string_value (reader), FALSE));
      break;
    case PANGO_ATTR_WORD:
      attr = pango_attr_word_new ();
      break;
    case PANGO_ATTR_SENTENCE:
      attr = pango_attr_sentence_new ();
      break;
    case PANGO_ATTR_BASELINE_SHIFT:
      attr = pango_attr_baseline_shift_new (get_enum_value (PANGO_TYPE_BASELINE_SHIFT, json_reader_get_string_value (reader), TRUE));
      break;
    case PANGO_ATTR_FONT_SCALE:
      attr = pango_attr_font_scale_new (get_enum_value (PANGO_TYPE_BASELINE_SHIFT, json_reader_get_string_value (reader), FALSE));
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

  if (!json_reader_is_array (reader))
    {
      g_set_error (error,
                   PANGO_LAYOUT_SERIALIZE_ERROR, PANGO_LAYOUT_SERIALIZE_INVALID_SYNTAX,
                   "\"attributes\" must be a Json array");
      goto fail;
    }

  attributes = pango_attr_list_new ();

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
                       PANGO_LAYOUT_SERIALIZE_ERROR,
                       PANGO_LAYOUT_SERIALIZE_INVALID_SYNTAX,
                       "Tab \"positions\"  must be a Json array");
          goto fail;
        }

      pango_tab_array_resize (tabs, json_reader_count_elements (reader));
      for (int i = 0; i < json_reader_count_elements (reader); i++)
        {
          int pos;
          json_reader_read_element (reader, i);
          pos = json_reader_get_int_value (reader);
          pango_tab_array_set_tab (tabs, i, PANGO_TAB_LEFT, pos);
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

static PangoLayout *
json_to_layout (PangoContext *context,
                JsonNode     *node,
                GError      **error)
{
  JsonReader *reader;
  PangoLayout *layout;

  layout = pango_layout_new (context);

  reader = json_reader_new (node);
  if (!json_reader_is_object (reader))
    {
      g_set_error (error,
                   PANGO_LAYOUT_SERIALIZE_ERROR,
                   PANGO_LAYOUT_SERIALIZE_INVALID_SYNTAX,
                   "Layout must be a Json object");
      goto fail;
    }

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
                       PANGO_LAYOUT_SERIALIZE_ERROR,
                       PANGO_LAYOUT_SERIALIZE_INVALID_VALUE,
                       "Could not parse \"font\" value: %s",
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
                                             FALSE);
      if (align == -1)
        {
          g_set_error (error,
                       PANGO_LAYOUT_SERIALIZE_ERROR,
                       PANGO_LAYOUT_SERIALIZE_INVALID_VALUE,
                       "Could not parse \"alignment\" value: %s",
                       json_reader_get_string_value (reader));
          goto fail;
        }

      pango_layout_set_alignment (layout, align);
    }

  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "wrap"))
    {
      PangoWrapMode wrap = get_enum_value (PANGO_TYPE_WRAP_MODE,
                                           json_reader_get_string_value (reader),
                                           FALSE);

      if (wrap == -1)
        {
          g_set_error (error,
                       PANGO_LAYOUT_SERIALIZE_ERROR,
                       PANGO_LAYOUT_SERIALIZE_INVALID_VALUE,
                       "Could not parse \"wrap\" value: %s",
                       json_reader_get_string_value (reader));
          goto fail;
        }

      pango_layout_set_wrap (layout, wrap);
    }

  json_reader_end_member (reader);

  if (json_reader_read_member (reader, "ellipsize"))
    {
      PangoEllipsizeMode ellipsize = get_enum_value (PANGO_TYPE_ELLIPSIZE_MODE,
                                                     json_reader_get_string_value (reader),
                                                     FALSE);

      if (ellipsize == -1)
        {
          g_set_error (error,
                       PANGO_LAYOUT_SERIALIZE_ERROR,
                       PANGO_LAYOUT_SERIALIZE_INVALID_VALUE,
                       "Could not parse \"ellipsize\" value: %s",
                       json_reader_get_string_value (reader));
          goto fail;
        }

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
  g_object_unref (layout);
  return NULL;
}

/* }}} */
/* {{{ Public API */

/**
 * pango_layout_serialize:
 * @layout: a `PangoLayout`
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
pango_layout_serialize (PangoLayout *layout)
{
  JsonGenerator *generator;
  JsonNode *node;
  char *data;
  gsize size;

  node = layout_to_json (layout);

  generator = json_generator_new ();
  json_generator_set_pretty (generator, TRUE);
  json_generator_set_indent (generator, 2);

  json_generator_set_root (generator, node);
  data = json_generator_to_data (generator, &size);

  json_node_free (node);
  g_object_unref (generator);

  return g_bytes_new_take (data, size);
}

/**
 * pango_layout_write_to_file:
 * @layout: a `PangoLayout`
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
pango_layout_write_to_file (PangoLayout  *layout,
                            const char   *filename,
                            GError      **error)
{
  GBytes *bytes;
  gboolean result;

  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), FALSE);
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  bytes = pango_layout_serialize (layout);
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
pango_layout_deserialize (PangoContext  *context,
                          GBytes        *bytes,
                          GError       **error)
{
  JsonParser *parser;
  JsonNode *node;
  PangoLayout *layout;

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
  layout = json_to_layout (context, node, error);

  g_object_unref (parser);

  return layout;
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
