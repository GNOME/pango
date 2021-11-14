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

#include "pango/css/gtkcss.h"
#include "pango/css/gtkcssdataurlprivate.h"
#include "pango/css/gtkcssparserprivate.h"
#include "pango/css/gtkcssserializerprivate.h"

/* {{{ Printer */

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

/* }}} */
/* {{{ Serialization */

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

static void
append_attributes (Printer       *p,
                   const char    *param_name,
                   PangoAttrList *attrs)
{
  char *str;

  str = pango_attr_list_to_string (attrs);
  append_string_param (p, param_name, str);
  g_free (str);
}

static void
append_tab_array (Printer       *p,
                  const char    *param_name,
                  PangoTabArray *tabs)
{
  char *str;

  str = pango_tab_array_to_string (tabs);
  append_string_param (p, param_name, str);
  g_free (str);
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

/* }}} */
/* {{{ Deserialization */

static PangoContext *parser_context; /* FIXME */

typedef struct
{
  const char *name;
  gboolean (* parse_func) (GtkCssParser *parser, gpointer result);
  void (* clear_func) (gpointer data);
  gpointer result;
} Declaration;

static guint
parse_declarations (GtkCssParser      *parser,
                    const Declaration *declarations,
                    guint              n_declarations)
{
  guint parsed = 0;
  guint i;

  g_assert (n_declarations < 8 * sizeof (guint));

  while (!gtk_css_parser_has_token (parser, GTK_CSS_TOKEN_EOF))
    {
      gtk_css_parser_start_semicolon_block (parser, GTK_CSS_TOKEN_OPEN_CURLY);

      for (i = 0; i < n_declarations; i++)
        {
          if (gtk_css_parser_try_ident (parser, declarations[i].name))
            {
              if (!gtk_css_parser_try_token (parser, GTK_CSS_TOKEN_COLON))
                {
                  gtk_css_parser_error_syntax (parser, "Expected ':' after variable declaration");
                }
              else
                {
                  if (parsed & (1 << i))
                    {
                      gtk_css_parser_warn_syntax (parser, "Variable \"%s\" defined multiple times", declarations[i].name);
                      /* Unset, just to be sure */
                      parsed &= ~(1 << i);
                      if (declarations[i].clear_func)
                        declarations[i].clear_func (declarations[i].result);
                    }
                  if (!declarations[i].parse_func (parser, declarations[i].result))
                    {
                      /* nothing to do */
                    }
                  else if (!gtk_css_parser_has_token (parser, GTK_CSS_TOKEN_EOF))
                    {
                      gtk_css_parser_error_syntax (parser, "Expected ';' at end of statement");
                      if (declarations[i].clear_func)
                        declarations[i].clear_func (declarations[i].result);
                    }
                  else
                    {
                      parsed |= (1 << i);
                    }
                }
              break;
            }
        }
      if (i == n_declarations)
        {
          if (gtk_css_parser_has_token (parser, GTK_CSS_TOKEN_IDENT))
            gtk_css_parser_error_syntax (parser, "No variable named \"%s\"",
                                         gtk_css_parser_get_token (parser)->string.string);
          else
            gtk_css_parser_error_syntax (parser, "Expected a variable name");
        }

      gtk_css_parser_end_block (parser);
    }

  return parsed;
}

static gboolean
parse_text (GtkCssParser *parser,
            gpointer      out_data)
{
  *(char **) out_data = gtk_css_parser_consume_string (parser);
  return TRUE;
}

static gboolean
parse_attributes (GtkCssParser *parser,
                  gpointer      out_data)
{
  char *string;
  PangoAttrList *attrs;

  string = gtk_css_parser_consume_string (parser);
  attrs = pango_attr_list_from_string (string);
  g_free (string);

  *(PangoAttrList **)out_data = attrs;

  return attrs != NULL;
}

static gboolean
parse_tabs (GtkCssParser *parser,
            gpointer      out_data)
{
  char *string;
  PangoTabArray *tabs;

  string = gtk_css_parser_consume_string (parser);
  tabs = pango_tab_array_from_string (string);
  g_free (string);

  *(PangoTabArray **)out_data = tabs;

  return tabs != NULL;
}

static gboolean
parse_font (GtkCssParser *parser,
            gpointer      out_data)
{
  char *string;
  PangoFontDescription *desc;

  string = gtk_css_parser_consume_string (parser);
  desc = pango_font_description_from_string (string);
  g_free (string);

  *(PangoFontDescription **) out_data = desc;

  return desc != NULL;
}

static gboolean
parse_enum_value (GtkCssParser *parser,
                  GType         enum_type,
                  gpointer      out_data)
{
  char *string;
  GEnumClass *enum_class;
  GEnumValue *enum_value;

  string = gtk_css_parser_consume_ident (parser);
  enum_class = g_type_class_ref (enum_type);
  enum_value = g_enum_get_value_by_nick (enum_class, string);
  g_type_class_unref (enum_class);
  g_free (string);

  if (enum_value)
    *(int *) out_data = enum_value->value;

  return enum_value != NULL;
}

static gboolean
parse_alignment (GtkCssParser *parser,
                 gpointer      out_data)
{
  return parse_enum_value (parser, PANGO_TYPE_ALIGNMENT, out_data);
}

static gboolean
parse_wrap (GtkCssParser *parser,
            gpointer      out_data)
{
  return parse_enum_value (parser, PANGO_TYPE_WRAP_MODE, out_data);
}

static gboolean
parse_ellipsize (GtkCssParser *parser,
                 gpointer      out_data)
{
  return parse_enum_value (parser, PANGO_TYPE_ELLIPSIZE_MODE, out_data);
}

static gboolean
parse_boolean (GtkCssParser *parser,
               gpointer      out_data)
{
  if (gtk_css_parser_try_ident (parser, "true"))
    *(gboolean *) out_data = TRUE;
  else if (gtk_css_parser_try_ident (parser, "false"))
    *(gboolean *) out_data = FALSE;
  else
    return FALSE;
  return TRUE;
}

static gboolean
parse_int (GtkCssParser *parser,
           gpointer      out_data)
{
  return gtk_css_parser_consume_integer (parser, (int *)out_data);
}

static gboolean
parse_double (GtkCssParser *parser,
              gpointer      out_data)
{
  return gtk_css_parser_consume_number (parser, (double *)out_data);
}

static PangoLayout *
parse_layout (GtkCssParser *parser)
{
  char *text = NULL;
  PangoAttrList *attrs = NULL;
  PangoFontDescription *desc = NULL;
  PangoTabArray *tabs = NULL;
  gboolean justify = FALSE;
  gboolean justify_last_line = FALSE;
  gboolean single_paragraph = FALSE;
  gboolean auto_dir = TRUE;
  PangoAlignment align = PANGO_ALIGN_LEFT;
  PangoWrapMode wrap = PANGO_WRAP_WORD;
  PangoEllipsizeMode ellipsize = PANGO_ELLIPSIZE_NONE;
  int width = -1;
  int height = -1;
  int indent = 0;
  int spacing = 0;
  double line_spacing = 0.;
  const Declaration declarations[] = {
    { "text", parse_text, g_free, &text },
    { "attributes", parse_attributes, (GDestroyNotify)pango_attr_list_unref, &attrs },
    { "font", parse_font, (GDestroyNotify)pango_font_description_free, &desc },
    { "tabs", parse_tabs, (GDestroyNotify)pango_tab_array_free, &tabs },
    { "alignment", parse_alignment, NULL, &align },
    { "wrap", parse_wrap, NULL, &wrap },
    { "ellipsize", parse_ellipsize, NULL, &ellipsize },
    { "justify", parse_boolean, NULL, &justify },
    { "justify-last-line", parse_boolean, NULL, &justify_last_line },
    { "single-paragraph", parse_boolean, NULL, &single_paragraph },
    { "auto-dir", parse_boolean, NULL, &auto_dir },
    { "width", parse_int, NULL, &width },
    { "height", parse_int, NULL, &height },
    { "indent", parse_int, NULL, &indent },
    { "spacing", parse_int, NULL, &spacing },
    { "line-spacing", parse_double, NULL, &line_spacing },
  };
  PangoLayout *layout;

  parse_declarations (parser, declarations, G_N_ELEMENTS (declarations));

  layout = pango_layout_new (parser_context);

  if (text)
    {
      pango_layout_set_text (layout, text, -1);
      g_free (text);
    }
  if (attrs)
    {
      pango_layout_set_attributes (layout, attrs);
      pango_attr_list_unref (attrs);
    }
  if (desc)
    {
      pango_layout_set_font_description (layout, desc);
      pango_font_description_free (desc);
    }
  if (tabs)
    {
      pango_layout_set_tabs (layout, tabs);
      pango_tab_array_free (tabs);
    }

  pango_layout_set_justify (layout, justify);
  pango_layout_set_justify_last_line (layout, justify_last_line);
  pango_layout_set_single_paragraph_mode (layout, single_paragraph);
  pango_layout_set_auto_dir (layout, auto_dir);
  pango_layout_set_alignment (layout, align);
  pango_layout_set_wrap (layout, wrap);
  pango_layout_set_ellipsize (layout,ellipsize);
  pango_layout_set_width (layout, width);
  pango_layout_set_height (layout, height);
  pango_layout_set_indent (layout, indent);
  pango_layout_set_spacing (layout, spacing);
  pango_layout_set_line_spacing (layout, line_spacing);

  return layout;
}

static PangoLayout *
parse_toplevel_layout (GtkCssParser *parser)
{
  PangoLayout *layout = NULL;

  gtk_css_parser_start_semicolon_block (parser, GTK_CSS_TOKEN_OPEN_CURLY);

  if (gtk_css_parser_try_ident (parser, "layout"))
    {

      if (!gtk_css_parser_has_token (parser, GTK_CSS_TOKEN_EOF))
        {
          gtk_css_parser_error_syntax (parser, "Expected '{' after node name");
          return FALSE;
        }

      gtk_css_parser_end_block_prelude (parser);

      layout = parse_layout (parser);

      if (layout)
        {
          if (!gtk_css_parser_has_token (parser, GTK_CSS_TOKEN_EOF))
            gtk_css_parser_error_syntax (parser, "Expected '}' at end of node definition");
        }
    }

  gtk_css_parser_end_block (parser);

  return layout;
}

/* }}} */
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

static void
parser_error (GtkCssParser         *parser,
              const GtkCssLocation *start,
              const GtkCssLocation *end,
              const GError         *error,
              gpointer              user_data)
{
  g_print ("from line %ld:%ld to %ld:%ld: %s\n",
           start->lines, start->line_chars,
           end->lines, end->line_chars,
           error->message);
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
  PangoLayout *layout = NULL;
  GtkCssParser *parser;

  parser = gtk_css_parser_new_for_bytes (bytes, NULL, parser_error, NULL, NULL);

  parser_context = context;
  layout = parse_toplevel_layout (parser);
  parser_context = NULL;

  gtk_css_parser_unref (parser);

  return layout;
}

/* }}} */

/* vim:set foldmethod=marker expandtab: */
