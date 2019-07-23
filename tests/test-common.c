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
      g_string_append_printf (string, "%s", pango_language_to_string (((PangoAttrLanguage *)attr)->value));
      break;
    case PANGO_ATTR_FAMILY:
    case PANGO_ATTR_FONT_FEATURES:
      g_string_append_printf (string, "%s", ((PangoAttrString *)attr)->value);
      break;
    case PANGO_ATTR_STYLE:
    case PANGO_ATTR_WEIGHT:
    case PANGO_ATTR_VARIANT:
    case PANGO_ATTR_STRETCH:
    case PANGO_ATTR_SIZE:
    case PANGO_ATTR_ABSOLUTE_SIZE:
    case PANGO_ATTR_UNDERLINE:
    case PANGO_ATTR_STRIKETHROUGH:
    case PANGO_ATTR_RISE:
    case PANGO_ATTR_FALLBACK:
    case PANGO_ATTR_LETTER_SPACING:
    case PANGO_ATTR_GRAVITY:
    case PANGO_ATTR_GRAVITY_HINT:
    case PANGO_ATTR_FOREGROUND_ALPHA:
    case PANGO_ATTR_BACKGROUND_ALPHA:
    case PANGO_ATTR_ALLOW_BREAKS:
      g_string_append_printf (string, "%d", ((PangoAttrInt *)attr)->value);
      break;
    case PANGO_ATTR_FONT_DESC:
      g_string_append_printf (string, "%s", pango_font_description_to_string (((PangoAttrFontDesc *)attr)->desc));
      break;
    case PANGO_ATTR_FOREGROUND:
    case PANGO_ATTR_BACKGROUND:
    case PANGO_ATTR_UNDERLINE_COLOR:
    case PANGO_ATTR_STRIKETHROUGH_COLOR:
      g_string_append_printf (string, "%s", pango_color_to_string (&((PangoAttrColor *)attr)->color));
      break;
    case PANGO_ATTR_SHAPE:
      g_string_append_printf (string, "shape");
      break;
    case PANGO_ATTR_SCALE:
      g_string_append_printf (string,"%f", ((PangoAttrFloat *)attr)->value);
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

