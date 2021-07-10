/* Pango
 * test-common.c: Common test code
 *
 * Copyright (C) 2014 Red Hat, Inc
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

#include <glib.h>
#include <string.h>

#include <locale.h>

#ifdef G_OS_WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <pango/pangocairo.h>
#include "test-common.h"

char *
diff_with_file (const char  *file,
                char        *text,
                gssize       len,
                GError     **error)
{
  const char *command[] = { "diff", "-u", "-i", file, NULL, NULL };
  char *diff, *tmpfile;
  int fd;

  diff = NULL;

  if (len < 0)
    len = strlen (text);

  /* write the text buffer to a temporary file */
  fd = g_file_open_tmp (NULL, &tmpfile, error);
  if (fd < 0)
    return NULL;

  if (write (fd, text, len) != (int) len)
    {
      close (fd);
      g_set_error (error,
                   G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "Could not write data to temporary file '%s'", tmpfile);
      goto done;
    }
  close (fd);
  command[4] = tmpfile;

  /* run diff command */
  g_spawn_sync (NULL,
                (char **) command,
                NULL,
                G_SPAWN_SEARCH_PATH,
                NULL, NULL,
                &diff,
                NULL, NULL,
                error);

done:
  unlink (tmpfile);
  g_free (tmpfile);

  return diff;
}

gboolean
file_has_prefix (const char  *filename,
                 const char  *str,
                 GError     **error)
{
  char *contents;
  gsize len;
  gboolean ret;

  if (!g_file_get_contents (filename, &contents, &len, error))
    return FALSE;

  ret = g_str_has_prefix (contents, str);

  g_free (contents);

  return ret;
}

void
print_attribute (PangoAttribute *attr, GString *string)
{
  GEnumClass *class;
  GEnumValue *value;

  g_string_append_printf (string, "[%d,%d]", attr->start_index, attr->end_index);

  class = g_type_class_ref (pango_attr_type_get_type ());
  value = g_enum_get_value (class, attr->klass->type);
  g_string_append_printf (string, "%s=", value->value_nick);
  g_type_class_unref (class);

  switch (attr->klass->type)
    {
    case PANGO_ATTR_LANGUAGE:
      g_string_append (string, pango_language_to_string (((PangoAttrLanguage *)attr)->value));
      break;
    case PANGO_ATTR_FAMILY:
    case PANGO_ATTR_FONT_FEATURES:
      g_string_append (string, ((PangoAttrString *)attr)->value);
      break;
    case PANGO_ATTR_STYLE:
    case PANGO_ATTR_WEIGHT:
    case PANGO_ATTR_VARIANT:
    case PANGO_ATTR_STRETCH:
    case PANGO_ATTR_SIZE:
    case PANGO_ATTR_ABSOLUTE_SIZE:
    case PANGO_ATTR_UNDERLINE:
    case PANGO_ATTR_OVERLINE:
    case PANGO_ATTR_STRIKETHROUGH:
    case PANGO_ATTR_RISE:
    case PANGO_ATTR_FALLBACK:
    case PANGO_ATTR_LETTER_SPACING:
    case PANGO_ATTR_GRAVITY:
    case PANGO_ATTR_GRAVITY_HINT:
    case PANGO_ATTR_FOREGROUND_ALPHA:
    case PANGO_ATTR_BACKGROUND_ALPHA:
    case PANGO_ATTR_ALLOW_BREAKS:
    case PANGO_ATTR_INSERT_HYPHENS:
    case PANGO_ATTR_SHOW:
      g_string_append_printf (string, "%d", ((PangoAttrInt *)attr)->value);
      break;
    case PANGO_ATTR_FONT_DESC:
      {
        char *text = pango_font_description_to_string (((PangoAttrFontDesc *)attr)->desc);
        g_string_append (string, text);
        g_free (text);
      }
      break;
    case PANGO_ATTR_FOREGROUND:
    case PANGO_ATTR_BACKGROUND:
    case PANGO_ATTR_UNDERLINE_COLOR:
    case PANGO_ATTR_OVERLINE_COLOR:
    case PANGO_ATTR_STRIKETHROUGH_COLOR:
      {
        char *text = pango_color_to_string (&((PangoAttrColor *)attr)->color);
        g_string_append (string, text);
        g_free (text);
      }
      break;
    case PANGO_ATTR_SHAPE:
      g_string_append_printf (string, "shape");
      break;
    case PANGO_ATTR_SCALE:
      {
        char val[20];

        g_ascii_formatd (val, 20, "%f", ((PangoAttrFloat *)attr)->value);
        g_string_append (string, val);
      }
      break;
    default:
      g_assert_not_reached ();
      break;
    }
}

void
print_attr_list (PangoAttrList *attrs, GString *string)
{
  PangoAttrIterator *iter;

  if (!attrs)
    return;

  iter = pango_attr_list_get_iterator (attrs);
  do {
    gint start, end;
    GSList *list, *l;

    pango_attr_iterator_range (iter, &start, &end);
    g_string_append_printf (string, "range %d %d\n", start, end);
    list = pango_attr_iterator_get_attrs (iter);
    for (l = list; l; l = l->next)
      {
        PangoAttribute *attr = l->data;
        print_attribute (attr, string);
        g_string_append (string, "\n");
      }
    g_slist_free_full (list, (GDestroyNotify)pango_attribute_destroy);
  } while (pango_attr_iterator_next (iter));

  pango_attr_iterator_destroy (iter);
}

void
print_attributes (GSList *attrs, GString *string)
{
  GSList *l;

  for (l = attrs; l; l = l->next)
    {
      PangoAttribute *attr = l->data;

      print_attribute (attr, string);
      g_string_append (string, "\n");
    }
}

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

PangoAttribute *
attribute_from_string (const char *string)
{
  const char *s;
  char *p;
  PangoAttribute *attr;
  long long start, end;
  GEnumClass *class;
  int i;
  PangoColor color;
  int val;

  s = string;
  g_assert (*s == '[');

  s++;
  start = strtoll (s, &p, 10);
  g_assert (p > s);
  g_assert (*p == ',');
  s = p + 1;

  g_assert (start >= 0);

  end = strtoll (s, &p, 10);
  g_assert (p > s);
  g_assert (*p == ']');
  s = p + 1;

  if (end == -1)
    end = G_MAXUINT;

  g_assert (start >= 0);
  g_assert (end >= 0);

  class = g_type_class_ref (pango_attr_type_get_type ());

  for (i = 0; i < class->n_values; i++)
    {
      if (g_str_has_prefix (s, class->values[i].value_nick))
        break;
    }

  g_assert (i < class->n_values);

  s += strlen (class->values[i].value_nick);
  g_assert (*s == '=');
  s++;

  switch (class->values[i].value)
    {
    case PANGO_ATTR_LANGUAGE:
      attr = pango_attr_language_new (pango_language_from_string (s));
      break;
    case PANGO_ATTR_FAMILY:
      attr = pango_attr_family_new (s);
      break;
    case PANGO_ATTR_FONT_FEATURES:
      attr = pango_attr_font_features_new (s);
      break;
    case PANGO_ATTR_STYLE:
      {
        if (!pango_parse_enum (PANGO_TYPE_STYLE, s, &val, FALSE, NULL))
          val = strtol (s, &p, 10);
      attr = pango_attr_style_new (val);
      }
      break;
    case PANGO_ATTR_WEIGHT:
      {
        if (!pango_parse_enum (PANGO_TYPE_WEIGHT, s, &val, FALSE, NULL))
          val = strtol (s, &p, 10);
        attr = pango_attr_weight_new (val);
      }
      break;
    case PANGO_ATTR_VARIANT:
      {
        if (!pango_parse_enum (PANGO_TYPE_VARIANT, s, &val, FALSE, NULL))
          val = strtol (s, &p, 10);
        attr = pango_attr_variant_new (val);
      }
      break;
    case PANGO_ATTR_STRETCH:
      {
        if (!pango_parse_enum (PANGO_TYPE_STRETCH, s, &val, FALSE, NULL))
          val = strtol (s, &p, 10);
        attr = pango_attr_stretch_new (val);
      }
      break;
    case PANGO_ATTR_SIZE:
      attr = pango_attr_size_new (strtol (s, &p, 10));
      break;
    case PANGO_ATTR_ABSOLUTE_SIZE:
      attr = pango_attr_size_new_absolute (strtol (s, &p, 10));
      break;
    case PANGO_ATTR_UNDERLINE:
      {
        if (!pango_parse_enum (PANGO_TYPE_UNDERLINE, s, &val, FALSE, NULL))
          val = strtol (s, &p, 10);
        attr = pango_attr_underline_new (val);
      }
      break;
    case PANGO_ATTR_OVERLINE:
      {
        if (!pango_parse_enum (PANGO_TYPE_OVERLINE, s, &val, FALSE, NULL))
          val = strtol (s, &p, 10);
        attr = pango_attr_overline_new (val);
      }
      break;
    case PANGO_ATTR_STRIKETHROUGH:
      attr = pango_attr_strikethrough_new (strtol (s, &p, 10));
      break;
    case PANGO_ATTR_RISE:
      attr = pango_attr_rise_new (strtol (s, &p, 10));
      break;
    case PANGO_ATTR_FALLBACK:
      attr = pango_attr_fallback_new (strtol (s, &p, 10));
      break;
    case PANGO_ATTR_LETTER_SPACING:
      attr = pango_attr_letter_spacing_new (strtol (s, &p, 10));
      break;
    case PANGO_ATTR_GRAVITY:
      {
        if (!pango_parse_enum (PANGO_TYPE_GRAVITY, s, &val, FALSE, NULL))
          val = strtol (s, &p, 10);
        attr = pango_attr_gravity_new (val);
      }
      break;
    case PANGO_ATTR_GRAVITY_HINT:
      {
        if (!pango_parse_enum (PANGO_TYPE_GRAVITY_HINT, s, &val, FALSE, NULL))
          val = strtol (s, &p, 10);
        attr = pango_attr_gravity_hint_new (val);
      }
      break;
    case PANGO_ATTR_FOREGROUND_ALPHA:
      attr = pango_attr_foreground_alpha_new (strtol (s, &p, 10));
      break;
    case PANGO_ATTR_BACKGROUND_ALPHA:
      attr = pango_attr_background_alpha_new (strtol (s, &p, 10));
      break;
    case PANGO_ATTR_ALLOW_BREAKS:
      attr = pango_attr_allow_breaks_new (strtol (s, &p, 10));
      break;
    case PANGO_ATTR_INSERT_HYPHENS:
      attr = pango_attr_insert_hyphens_new (strtol (s, &p, 10));
      break;
    case PANGO_ATTR_SHOW:
      attr = pango_attr_show_new (strtol (s, &p, 10));
      break;
    case PANGO_ATTR_FONT_DESC:
      {
        PangoFontDescription *desc = pango_font_description_from_string (s);
        attr = pango_attr_font_desc_new (desc);
        pango_font_description_free (desc);
      }
      break;
    case PANGO_ATTR_FOREGROUND:
      {
        pango_color_parse (&color, s);
        attr = pango_attr_foreground_new (color.red, color.green, color.blue);
      }
      break;
    case PANGO_ATTR_BACKGROUND:
      {
        pango_color_parse (&color, s);
        attr = pango_attr_background_new (color.red, color.green, color.blue);
      }
      break;
    case PANGO_ATTR_UNDERLINE_COLOR:
      {
        pango_color_parse (&color, s);
        attr = pango_attr_underline_color_new (color.red, color.green, color.blue);
      }
      break;
    case PANGO_ATTR_OVERLINE_COLOR:
      {
        pango_color_parse (&color, s);
        attr = pango_attr_overline_color_new (color.red, color.green, color.blue);
      }
      break;
    case PANGO_ATTR_STRIKETHROUGH_COLOR:
      {
        pango_color_parse (&color, s);
        attr = pango_attr_strikethrough_color_new (color.red, color.green, color.blue);
      }
      break;
    case PANGO_ATTR_SHAPE:
      {
        PangoRectangle rect = { 0, };
        attr = pango_attr_shape_new (&rect, &rect);
      }
      break;
    case PANGO_ATTR_SCALE:
      attr = pango_attr_scale_new (strtod (s, &p));
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  g_type_class_unref (class);

  attr->start_index = start;
  attr->end_index = end;

  return attr;
}

G_GNUC_END_IGNORE_DEPRECATIONS

PangoAttrList *
attributes_from_string (const char *string)
{
  PangoAttrList *attrs;
  char **lines;

  attrs = pango_attr_list_new ();

  lines = g_strsplit (string, "\n", 0);
  for (int i = 0; lines[i]; i++)
    {
      PangoAttribute *attr;

      if (lines[i][0] == '\0')
        continue;

      attr = attribute_from_string (lines[i]);
      g_assert (attr);
      pango_attr_list_insert (attrs, attr);
    }
  g_strfreev (lines);

  return attrs;
}

const char *
get_script_name (GUnicodeScript s)
{
  GEnumClass *class;
  GEnumValue *value;
  const char *nick;

  class = g_type_class_ref (g_unicode_script_get_type ());
  value = g_enum_get_value (class, s);
  nick = value->value_nick;
  g_type_class_unref (class);
  return nick;
}

