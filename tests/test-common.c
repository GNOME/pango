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
#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

#include <locale.h>

#ifdef G_OS_WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <pango/pangocairo.h>
#include "test-common.h"

#include <hb-ot.h>

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

char *
diff_bytes (GBytes  *b1,
            GBytes  *b2,
            GError **error)
{
  const char *command[] = { "diff", "-u", "-i", NULL, NULL, NULL };
  char *diff, *tmpfile, *tmpfile2;
  int fd;
  const char *text;
  gsize len;

  /* write the text buffer to a temporary file */
  fd = g_file_open_tmp (NULL, &tmpfile, error);
  if (fd < 0)
    return NULL;

  text = (const char *) g_bytes_get_data (b1, &len);
  if (write (fd, text, len) != (int) len)
    {
      close (fd);
      g_set_error (error,
                   G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "Could not write data to temporary file '%s'", tmpfile);
      goto done;
    }
  close (fd);

  fd = g_file_open_tmp (NULL, &tmpfile2, error);
  if (fd < 0)
    return NULL;

  text = (const char *) g_bytes_get_data (b2, &len);
  if (write (fd, text, len) != (int) len)
    {
      close (fd);
      g_set_error (error,
                   G_FILE_ERROR, G_FILE_ERROR_FAILED,
                   "Could not write data to temporary file '%s'", tmpfile2);
      goto done;
    }
  close (fd);

  command[3] = tmpfile;
  command[4] = tmpfile2;

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
  unlink (tmpfile2);
  g_free (tmpfile2);

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
  PangoAttrList *l = pango_attr_list_new ();
  char *s;

  pango_attr_list_insert (l, pango_attribute_copy (attr));
  s = pango_attr_list_to_string (l);
  g_string_append (string, s);
  g_free (s);
  pango_attr_list_unref (l);
}

void
print_attr_list (PangoAttrList *attrs, GString *string)
{
  PangoAttrIterator *iter;

  if (!attrs)
    return;

  iter = pango_attr_list_get_iterator (attrs);
  do {
    int start, end;
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

static void
add_file (PangoFontMap *map,
          const char   *path,
          const char   *name)
{
  char *fullname;
  hb_blob_t *blob;
  hb_face_t *hbface;
  PangoHbFace *face;

  fullname = g_build_filename (path, name, NULL);

  blob = hb_blob_create_from_file_or_fail (fullname);
  if (!blob)
    g_error ("No font data in %s", fullname);

  g_free (fullname);

  hbface = hb_face_create (blob, 0);
  hb_face_make_immutable (hbface);

  if (hb_ot_var_get_axis_count (hbface) == 0)
    {
      /* Add the default instance */
      face = pango_hb_face_new_from_hb_face (hbface, -1, NULL, NULL);
      pango_font_map_add_face (map, PANGO_FONT_FACE (face));
      return;
    }

  /* Add the variable face */
  face = pango_hb_face_new_from_hb_face (hbface, -2, "Variable", NULL);
  pango_font_map_add_face (map, PANGO_FONT_FACE (face));

  if (hb_ot_var_get_named_instance_count (hbface) > 0)
    {
      /* Add named instances */
      for (int i = 0; i < hb_ot_var_get_named_instance_count (hbface); i++)
        {
          face = pango_hb_face_new_from_hb_face (hbface, i, NULL, NULL);
          pango_font_map_add_face (map, PANGO_FONT_FACE (face));
        }
    }

  hb_face_destroy (hbface);
}

static void
add_generic_family (PangoFontMap *map,
                    const char   *path,
                    const char   *name)
{
  char *fullname;
  char *contents;
  gsize length;
  char *basename;
  char **families;
  GError *error = NULL;
  PangoGenericFamily *generic;

  g_assert (g_str_has_suffix (name, ".generic"));

  fullname = g_build_filename (path, name, NULL);

  if (!g_file_get_contents (fullname, &contents, &length, &error))
    g_error ("Failed to load %s: %s", fullname, error->message);

  g_free (fullname);

  basename = g_strdup (name);
  basename[strlen (name) - strlen (".generic")] = '\0';

  generic = pango_generic_family_new (basename);

  families = g_strsplit (contents, "\n", -1);
  for (int i = 0; families[i]; i++)
    {
      const char *name = families[i];
      PangoFontFamily *family;

      if (name[0] == '\0')
        continue;

      family = pango_font_map_get_family (map, name);
      if (!family)
        {
          g_warning ("no such family: %s", name);
          continue;
        }

      pango_generic_family_add_family (generic, family);
    }

  pango_font_map_add_family (map, PANGO_FONT_FAMILY (generic));

  g_strfreev (families);
  g_free (basename);
  g_free (contents);
}

static void
add_synthetic_faces (PangoFontMap *map,
                     const char   *path,
                     const char   *name)
{
  char *fullname;
  char *contents;
  GError *error = NULL;
  gsize length;
  char *basename;
  PangoFontFamily *family;
  gboolean make_italic;
  PangoMatrix italic_matrix = { 1, 0.2, 0, 1, 0, 0 };
  PangoHbFace *newface;
  GSList *newfaces, *l;

  g_assert (g_str_has_suffix (name, ".synthetic"));

  fullname = g_build_filename (path, name, NULL);

  if (!g_file_get_contents (fullname, &contents, &length, &error))
    g_error ("Failed to load %s: %s", fullname, error->message);

  g_free (fullname);

  basename = g_strdup (name);
  basename[strlen (name) - strlen (".synthetic")] = '\0';

  family = pango_font_map_get_family (map, basename);
  if (!family)
    g_error ("Family %s not found", basename);

  make_italic = strstr (contents, "Italic") != NULL;

  newfaces = NULL;


  for (int i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (family)); i++)
    {
      PangoHbFace *face = g_list_model_get_item (G_LIST_MODEL (family), i);

      if (make_italic)
        {
          char *name;
          PangoFontDescription *desc;

          name = g_strconcat (pango_font_face_get_name (PANGO_FONT_FACE (face)),
                              " Italic",
                              NULL);
          desc = pango_font_face_describe (PANGO_FONT_FACE (face));
          pango_font_description_set_style (desc, PANGO_STYLE_ITALIC);
          pango_font_description_unset_fields (desc, ~(PANGO_FONT_MASK_FAMILY|
                                                       PANGO_FONT_MASK_STYLE|
                                                       PANGO_FONT_MASK_STRETCH|
                                                       PANGO_FONT_MASK_WEIGHT));
          newface = pango_hb_face_new_synthetic (face, &italic_matrix, FALSE, name, desc);
          newfaces = g_slist_prepend (newfaces, newface);

          g_free (name);
          pango_font_description_free (desc);
        }

      g_object_unref (face);
    }


  for (l = newfaces; l; l = l->next)
    {
      newface = l->data;
      pango_font_map_add_face (map, PANGO_FONT_FACE (newface));
    }

  g_slist_free (newfaces);

  g_free (basename);
  g_free (contents);
}

void
install_fonts (void)
{
  PangoFontMap *map;
  GDir *dir;
  GError *error = NULL;
  GPtrArray *generic;
  GPtrArray *synthetic;
  char *path = NULL;

  map = pango_font_map_new ();

  path = g_build_filename (g_getenv ("G_TEST_SRCDIR"), "fonts", NULL);

  dir = g_dir_open (path, 0, &error);

  if (!dir)
    g_error ("Failed to install fonts: %s", error->message);

  generic = g_ptr_array_new_with_free_func (g_free);
  synthetic = g_ptr_array_new_with_free_func (g_free);

  while (TRUE)
    {
      const char *name = g_dir_read_name (dir);
      if (name == NULL)
        break;

      if (g_str_has_suffix (name, ".ttf") ||
          g_str_has_suffix (name, ".otf"))
        add_file (map, path, name);
      else if (g_str_has_suffix (name, ".generic"))
        g_ptr_array_add (generic, g_strdup (name));
      else if (g_str_has_suffix (name, ".synthetic"))
        g_ptr_array_add (synthetic, g_strdup (name));
    }

  /* Add generics and synthetics in a second path,
   * so all families are already added.
   */
  for (int i = 0; i < generic->len; i++)
    {
      const char *name = g_ptr_array_index (generic, i);
      add_generic_family (map, path, name);
    }
  g_ptr_array_free (generic, TRUE);

  for (int i = 0; i < synthetic->len; i++)
    {
      const char *name = g_ptr_array_index (synthetic, i);
      add_synthetic_faces (map, path, name);
    }
  g_ptr_array_free (synthetic, TRUE);


  pango_font_map_set_default (map);

  g_object_unref (map);
  g_dir_close (dir);

  g_free (path);
}

void
dump_fonts (void)
{
  PangoFontMap *map;

  map = pango_font_map_get_default ();

  for (int i = 0; i < g_list_model_get_n_items (G_LIST_MODEL (map)); i++)
    {
      PangoFontFamily *family = g_list_model_get_item (G_LIST_MODEL (map), i);

      if (PANGO_IS_GENERIC_FAMILY (family))
        {
          g_print ("%s (generic)\n", pango_font_family_get_name (family));
        }
      else
        {
          g_print ("%s\n", pango_font_family_get_name (family));

          for (int j = 0; j < g_list_model_get_n_items (G_LIST_MODEL (family)); j++)
            {
              PangoFontFace *face = g_list_model_get_item (G_LIST_MODEL (family), j);

              g_print ("\t%s %s%s\n",
                       pango_font_family_get_name (family),
                       pango_font_face_get_name (face),
                       pango_font_face_is_variable (face) ? " (variable)" : "");

              g_object_unref (face);
            }
        }

      g_object_unref (family);
    }
}

