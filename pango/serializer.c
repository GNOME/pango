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

typedef struct
{
  int indentation_level;
  GString *str;
} Printer;

static void
printer_init (Printer *self)
{
  self->indentation_level = 0;
  self->str = g_string_new (NULL);
}

#define IDENT_LEVEL 2 /* Spaces per level */
static int
get_indent (Printer *self)
{
  return self->indentation_level * IDENT_LEVEL;
}

static void
_indent (Printer *self)
{
  if (self->indentation_level > 0)
    g_string_append_printf (self->str, "%*s", get_indent (self), " ");
}
#undef IDENT_LEVEL

static void
start_node (Printer    *self,
            const char *node_name)
{
  g_string_append_printf (self->str, "%s {\n", node_name);
  self->indentation_level ++;
}

static void
end_node (Printer *self)
{
  self->indentation_level --;
  _indent (self);
  g_string_append (self->str, "}\n");
}

static void
print_string (GString    *str,
              const char *string,
              gboolean    multiline)
{
  gsize len;

  g_return_if_fail (str != NULL);
  g_return_if_fail (string != NULL);

  g_string_append_c (str, '"');

  do {
    len = strcspn (string, "\\\"\n\r\f");
    g_string_append_len (str, string, len);
    string += len;
    switch (*string)
      {
      case '\0':
        goto out;
      case '\n':
        if (multiline)
          g_string_append (str, "\\A\\\n");
        else
          g_string_append (str, "\\A ");
        break;
      case '\r':
        g_string_append (str, "\\D ");
        break;
      case '\f':
        g_string_append (str, "\\C ");
        break;
      case '\"':
        g_string_append (str, "\\\"");
        break;
      case '\\':
        g_string_append (str, "\\\\");
        break;
      default:
        g_assert_not_reached ();
        break;
      }
    string++;
  } while (*string);

out:
  g_string_append_c (str, '"');
}

static void
append_string_param (Printer    *p,
                     const char *param_name,
                     const char *value)
{
  _indent (p);
  g_string_append_printf (p->str, "%s: ", param_name);
  print_string (p->str, value, TRUE);
  g_string_append (p->str, ";\n");
}

static void
append_simple_string (Printer    *p,
                      const char *param_name,
                      const char *value)
{
  _indent (p);
  g_string_append_printf (p->str, "%s: ", param_name);
  g_string_append (p->str, value);
  g_string_append (p->str, ";\n");
}

static void
append_bool_param (Printer    *p,
                   const char *param_name,
                   gboolean    value)
{
  append_simple_string (p, param_name, value ? "true" : "false");
}

static void
append_enum_param (Printer    *p,
                   const char *param_name,
                   GType       type,
                   int         value)
{
  GEnumClass *enum_class;
  GEnumValue *enum_value;

  enum_class = g_type_class_ref (type);
  enum_value = g_enum_get_value (enum_class, value);

  if (enum_value)
    {
      append_simple_string (p, param_name, enum_value->value_nick);
    }
  else
    {
      
      char *v = g_strdup_printf ("%d", value);
      append_simple_string (p, param_name, v);
      g_free (v);
    }

  g_type_class_unref (enum_class);
}

static void
append_int_param (Printer    *p,
                  const char *param_name,
                  int         value)
{
  _indent (p);
  g_string_append_printf (p->str, "%s: %d;\n", param_name, value);
}

static void
append_float_param (Printer    *p,
                    const char *param_name,
                    float       value)
{
  char buf[G_ASCII_DTOSTR_BUF_SIZE];
  g_ascii_formatd (buf, G_ASCII_DTOSTR_BUF_SIZE, "%g", (double)value);
  append_simple_string (p, param_name, buf);
}

static void
append_font_description (Printer              *p,
                         const char           *param_name,
                         PangoFontDescription *desc)
{
  char *value = pango_font_description_to_string (desc);
  append_string_param (p, param_name, value);
  g_free (value);
}

static const char *
get_attr_value_nick (PangoAttrType attr_type)
{
  GEnumClass *enum_class;
  GEnumValue *enum_value;

  enum_class = g_type_class_ref (pango_attr_type_get_type ());
  enum_value = g_enum_get_value (enum_class, attr_type);
  g_type_class_unref (enum_class);

  return enum_value->value_nick;
}

static void
attr_print (GString        *str,
            PangoAttribute *attr)
{
  PangoAttrString *string;
  PangoAttrLanguage *lang;
  PangoAttrInt *integer;
  PangoAttrFloat *flt;
  PangoAttrFontDesc *font;
  PangoAttrColor *color;
  PangoAttrShape *shape;
  PangoAttrSize *size;
  PangoAttrFontFeatures *features;

  g_string_append_printf (str, "%u %u ", attr->start_index, attr->end_index);

  g_string_append (str, get_attr_value_nick (attr->klass->type));

  if ((string = pango_attribute_as_string (attr)) != NULL)
    g_string_append_printf (str, " %s", string->value);
  else if ((lang = pango_attribute_as_language (attr)) != NULL)
    g_string_append_printf (str, " %s", pango_language_to_string (lang->value));
  else if ((integer = pango_attribute_as_int (attr)) != NULL)
    g_string_append_printf (str, " %d", integer->value);
  else if ((flt = pango_attribute_as_float (attr)) != NULL)
    {
      char buf[20];
      g_ascii_formatd (buf, 20, " %f", flt->value);
      g_string_append_printf (str, " %s", buf);
    }
  else if ((font = pango_attribute_as_font_desc (attr)) != NULL)
    {
      char *s = pango_font_description_to_string (font->desc);
      g_string_append_printf (str, " \"%s\"", s);
      g_free (s);
    }
  else if ((color = pango_attribute_as_color (attr)) != NULL)
    {
      char *s = pango_color_to_string (&color->color);
      g_string_append_printf (str, " %s", s);
      g_free (s);
    }
  else if ((shape = pango_attribute_as_shape (attr)) != NULL)
    g_string_append (str, "shape"); /* FIXME */
  else if ((size = pango_attribute_as_size (attr)) != NULL)
    g_string_append_printf (str, " %d", size->size);
  else if ((features = pango_attribute_as_font_features (attr)) != NULL)
    g_string_append_printf (str, " \"%s\"", features->features);
  else
    g_assert_not_reached ();
}

static void
append_attributes (Printer       *p,
                   const char    *param_name,
                   PangoAttrList *attrs)
{
  GSList *attributes;
  GString *s;

  s = g_string_new ("");

  attributes = pango_attr_list_get_attributes (attrs);
  for (GSList *l = attributes; l; l = l->next)
    {
      PangoAttribute *attr = l->data;
      attr_print (s, attr);
      if (l->next)
        g_string_append_printf (s, ",\n%*s", (int)(get_indent (p) + strlen (param_name) + 2), "");
    }

  g_slist_free_full (attributes, (GDestroyNotify)pango_attribute_destroy);

  append_simple_string (p, param_name, s->str);
  g_string_free (s, TRUE);
}

static void
append_tab_array (Printer       *p,
                  const char    *param_name,
                  PangoTabArray *tabs)
{
  int *locations;
  GString *s;
  const char *unit;

  if (pango_tab_array_get_positions_in_pixels (tabs))
    unit = "px";
  else
    unit = "";

  pango_tab_array_get_tabs (tabs, NULL, &locations);

  s = g_string_new ("");
  for (int i = 0; i < pango_tab_array_get_size (tabs); i++)
    {
      if (s->len > 0)
        g_string_append_c (s, ' ');
      g_string_append_printf (s, "%d%s", locations[i], unit);
    }

  append_simple_string (p, "positions", s->str);

  g_string_free (s, TRUE);
  g_free (locations);
}

static void
layout_print (Printer     *p,
              PangoLayout *layout)
{
  start_node (p, "layout");

  if (layout->text)
    append_string_param (p, "text", layout->text);
  if (layout->attrs)
    append_attributes (p, "attributes", layout->attrs);
  if (layout->font_desc)
    append_font_description (p, "font", layout->font_desc);
  if (layout->tabs)
    append_tab_array (p, "tabs", layout->tabs);
  if (layout->justify)
    append_bool_param (p, "justify", layout->justify);
  if (layout->justify_last_line)
    append_bool_param (p, "justify-last-line", layout->justify_last_line);
  if (layout->single_paragraph)
    append_bool_param (p, "single-paragraph", layout->single_paragraph);
  if (!layout->auto_dir)
    append_bool_param (p, "auto-dir", layout->auto_dir);
  if (layout->alignment != PANGO_ALIGN_LEFT)
    append_enum_param (p, "alignment", PANGO_TYPE_ALIGNMENT, layout->alignment);
  if (layout->wrap != PANGO_WRAP_WORD)
    append_enum_param (p, "wrap", PANGO_TYPE_WRAP_MODE, layout->wrap);
  if (layout->ellipsize != PANGO_ELLIPSIZE_NONE)
    append_enum_param (p, "ellipsize", PANGO_TYPE_ELLIPSIZE_MODE, layout->ellipsize);
  if (layout->width != -1)
    append_int_param (p, "width", layout->width);
  if (layout->height != -1)
    append_int_param (p, "height", layout->height);
  if (layout->indent != 0)
    append_int_param (p, "indent", layout->indent);
  if (layout->spacing != 0)
    append_int_param (p, "spacing", layout->spacing);
  if (layout->line_spacing != 0.)
    append_float_param (p, "line-spacing", layout->line_spacing);

  end_node (p);
}

/* {{{ Public API */

/**
 * pango_layout_serialize:
 * @layout: a `PangoLayout`
 *
 * Serializes the @layout for later deserialization via [method@Pango.Layout.deserialize].
 *
 * There are no guarantees about the format of the output accross different
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
  Printer p;

  printer_init (&p);
  layout_print (&p, layout);

  return g_string_free_to_bytes (p.str);
}

/**
 * pango_layout_deserialize:
 * @context: a `PangoContext`
 * @bytes: the bytes containing the data
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
pango_layout_deserialize (PangoContext *context,
                          GBytes       *bytes)
{
  return NULL;
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
