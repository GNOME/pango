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
  const char *command[] = { "diff", "-u", file, NULL, NULL };
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
  command[3] = tmpfile;

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
  g_string_append_printf (string, "[%d %d] ", attr->start_index, attr->end_index);
  switch (attr->klass->type)
    {
    case PANGO_ATTR_LANGUAGE:
      g_string_append_printf (string,"language %s\n", pango_language_to_string (((PangoAttrLanguage *)attr)->value));
      break;
    case PANGO_ATTR_FAMILY:
      g_string_append_printf (string,"family %s\n", ((PangoAttrString *)attr)->value);
      break;
    case PANGO_ATTR_STYLE:
      g_string_append_printf (string,"style %d\n", ((PangoAttrInt *)attr)->value);
      break;
    case PANGO_ATTR_WEIGHT:
      g_string_append_printf (string,"weight %d\n", ((PangoAttrInt *)attr)->value);
      break;
    case PANGO_ATTR_VARIANT:
      g_string_append_printf (string,"variant %d\n", ((PangoAttrInt *)attr)->value);
      break;
    case PANGO_ATTR_STRETCH:
      g_string_append_printf (string,"stretch %d\n", ((PangoAttrInt *)attr)->value);
      break;
    case PANGO_ATTR_SIZE:
      g_string_append_printf (string,"size %d\n", ((PangoAttrSize *)attr)->size);
      break;
    case PANGO_ATTR_FONT_DESC:
      g_string_append_printf (string,"font %s\n", pango_font_description_to_string (((PangoAttrFontDesc *)attr)->desc));
      break;
    case PANGO_ATTR_FOREGROUND:
      g_string_append_printf (string,"foreground %s\n", pango_color_to_string (&((PangoAttrColor *)attr)->color));
      break;
    case PANGO_ATTR_BACKGROUND:
      g_string_append_printf (string,"background %s\n", pango_color_to_string (&((PangoAttrColor *)attr)->color));
      break;
    case PANGO_ATTR_UNDERLINE:
      g_string_append_printf (string,"underline %d\n", ((PangoAttrInt *)attr)->value);
      break;
    case PANGO_ATTR_STRIKETHROUGH:
      g_string_append_printf (string,"strikethrough %d\n", ((PangoAttrInt *)attr)->value);
      break;
    case PANGO_ATTR_RISE:
      g_string_append_printf (string,"rise %d\n", ((PangoAttrInt *)attr)->value);
      break;
    case PANGO_ATTR_SHAPE:
      g_string_append_printf (string,"shape\n");
      break;
    case PANGO_ATTR_SCALE:
      g_string_append_printf (string,"scale %f\n", ((PangoAttrFloat *)attr)->value);
      break;
    case PANGO_ATTR_FALLBACK:
      g_string_append_printf (string,"fallback %d\n", ((PangoAttrInt *)attr)->value);
      break;
    case PANGO_ATTR_LETTER_SPACING:
      g_string_append_printf (string,"letter-spacing %d\n", ((PangoAttrInt *)attr)->value);
      break;
    case PANGO_ATTR_UNDERLINE_COLOR:
      g_string_append_printf (string,"underline-color %s\n", pango_color_to_string (&((PangoAttrColor *)attr)->color));
      break;
    case PANGO_ATTR_STRIKETHROUGH_COLOR:
      g_string_append_printf (string,"strikethrough-color %s\n", pango_color_to_string (&((PangoAttrColor *)attr)->color));
      break;
    case PANGO_ATTR_ABSOLUTE_SIZE:
      g_string_append_printf (string,"absolute-size %d\n", ((PangoAttrSize *)attr)->size);
      break;
    case PANGO_ATTR_GRAVITY:
      g_string_append_printf (string,"gravity %d\n", ((PangoAttrInt *)attr)->value);
      break;
    case PANGO_ATTR_GRAVITY_HINT:
      g_string_append_printf (string,"gravity-hint %d\n", ((PangoAttrInt *)attr)->value);
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

      g_string_append (string, "  ");
      print_attribute (attr, string);
    }
}

typedef struct
{
  guint ref_count;
  GSList *attributes;
  GSList *attributes_tail;
} AL;

GSList *
attr_list_to_list (PangoAttrList *attrs)
{
  return ((AL*)attrs)->attributes;
}
