/* Pango
 * pangowin32-fontmap.c: Win32 font handling
 *
 * Copyright (C) 2000 Red Hat Software
 * Copyright (C) 2000 Tor Lillqvist
 * Copyright (C) 2001 Alexander Larsson
 * Copyright (C) 2007 Novell, Inc.
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <gio/gio.h>

#include "pango-fontmap.h"
#include "pango-impl-utils.h"
#include "pangowin32-private.h"

typedef struct _PangoWin32Family PangoWin32Family;
typedef PangoFontFamilyClass PangoWin32FamilyClass;

struct _PangoWin32Family
{
  PangoFontFamily parent_instance;

  char *family_name;
  GSList *faces;

  gboolean is_monospace;
};

#define PANGO_WIN32_TYPE_FAMILY              (pango_win32_family_get_type ())
#define PANGO_WIN32_FAMILY(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_WIN32_TYPE_FAMILY, PangoWin32Family))
#define PANGO_WIN32_IS_FAMILY (object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_WIN32_TYPE_FAMILY))
#define PANGO_WIN32_FAMILY_CLASS (klass)     (G_TYPE_CHECK_CLASS_CAST (klass), PANGO_WIN32_TYPE_FAMILY, PangoWin32Family))
#define PANGO_WIN32_IS_FAMILY_CLASS (klass)  (G_TYPE_CHECK_CLASS_CAST (klass), PANGO_WIN32_TYPE_FAMILY))
#define PANGO_WIN32_FAMILY_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS((object), PANGO_WIN32_FAMILY, PangoWin32FamilyClass))

#define PANGO_WIN32_TYPE_FACE              (pango_win32_face_get_type ())
#define PANGO_WIN32_FACE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_WIN32_TYPE_FACE, PangoWin32Face))
#define PANGO_WIN32_IS_FACE(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_WIN32_TYPE_FACE))
#define PANGO_WIN32_FACE_CLASS (klass)     (G_TYPE_CHECK_CLASS_CAST (klass), PANGO_WIN32_TYPE_FACE, PangoWin32Face))
#define PANGO_WIN32_IS_FACE_CLASS (klass)  (G_TYPE_CHECK_CLASS_CAST (klass), PANGO_WIN32_TYPE_FACE))
#define PANGO_WIN32_FACE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS((object), PANGO_WIN32_FACE, PangoWin32FaceClass))

static GType      pango_win32_face_get_type          (void);

static GType      pango_win32_family_get_type        (void);

static void       pango_win32_face_list_sizes        (PangoFontFace  *face,
                                                      int           **sizes,
                                                      int            *n_sizes);

static void       pango_win32_font_map_finalize      (GObject                      *object);
static PangoFont *pango_win32_font_map_load_font     (PangoFontMap                 *fontmap,
                                                      PangoContext                 *context,
                                                      const PangoFontDescription   *description);
static PangoFont *pango_win32_font_map_real_load_font  (PangoFontMap                 *fontmap,
                                                        PangoContext                 *context,
                                                        const PangoFontDescription   *description);
static PangoFontset *pango_win32_font_map_load_fontset (PangoFontMap               *fontmap,
                                                        PangoContext                 *context,
                                                        const PangoFontDescription   *desc,
                                                        PangoLanguage                *language);
static void       pango_win32_font_map_list_families (PangoFontMap                 *fontmap,
                                                      PangoFontFamily            ***families,
                                                      int                          *n_families);

static PangoFont *pango_win32_font_map_real_find_font (PangoWin32FontMap           *win32fontmap,
                                                       PangoContext                *context,
                                                       PangoWin32Face              *face,
                                                       const PangoFontDescription  *description);

static void       pango_win32_fontmap_cache_clear    (PangoWin32FontMap            *win32fontmap);

static PangoWin32Family *pango_win32_get_font_family (PangoWin32FontMap            *win32fontmap,
                                                      const char                   *family_name);

static const char *pango_win32_face_get_face_name    (PangoFontFace *face);

static PangoWin32FontMap *default_fontmap = NULL; /* MT-safe */

static void
pango_win32_font_map_ensure_families_list (PangoWin32FontMap *self)
{
  GPtrArray *array;
  
  if (self->families_list)
    return;

  array = g_hash_table_get_values_as_ptr_array (self->families);
  self->families_list = g_ptr_array_steal (array, NULL);
  g_ptr_array_unref (array);
}

static GType
pango_win32_font_map_get_item_type (GListModel *list)
{
  return PANGO_TYPE_FONT_FAMILY;
}

static guint
pango_win32_font_map_get_n_items (GListModel *list)
{
  PangoWin32FontMap *self = PANGO_WIN32_FONT_MAP (list);

  return g_hash_table_size (self->families);
}

static gpointer
pango_win32_font_map_get_item (GListModel *list,
                               guint       position)
{
  PangoWin32FontMap *self = PANGO_WIN32_FONT_MAP (list);
  gsize n;

  n = g_hash_table_size (self->families);
  if (position >= n)
    return NULL;

  pango_win32_font_map_ensure_families_list (self);
  return g_object_ref (self->families_list[position]);
}

static void
pango_win32_font_map_list_model_init (GListModelInterface *iface)
{
  iface->get_item_type = pango_win32_font_map_get_item_type;
  iface->get_n_items = pango_win32_font_map_get_n_items;
  iface->get_item = pango_win32_font_map_get_item;
}

G_DEFINE_TYPE_WITH_CODE (PangoWin32FontMap, pango_win32_font_map, PANGO_TYPE_FONT_MAP,
                         G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, pango_win32_font_map_list_model_init))

#define TOLOWER(c) \
  (((c) >= 'A' && (c) <= 'Z') ? (c) - 'A' + 'a' : (c))

static guint
case_insensitive_str_hash (const char *key)
{
  const char *p = key;
  guint h = TOLOWER (*p);

  if (h)
    {
      for (p += 1; *p != '\0'; p++)
        h = (h << 5) - h + TOLOWER (*p);
    }

  return h;
}

static gboolean
case_insensitive_str_equal (const char *key1,
                            const char *key2)
{
  while (*key1 && *key2 && TOLOWER (*key1) == TOLOWER (*key2))
    key1++, key2++;
  return (!*key1 && !*key2);
}

static guint
case_insensitive_wcs_hash (const wchar_t *key)
{
  const wchar_t *p = key;
  guint h = TOLOWER (*p);

  if (h)
    {
      for (p += 1; *p != '\0'; p++)
        h = (h << 5) - h + TOLOWER (*p);
    }

  return h;
}

static gboolean
case_insensitive_wcs_equal (const wchar_t *key1,
                            const wchar_t *key2)
{
  while (*key1 && *key2 && TOLOWER (*key1) == TOLOWER (*key2))
    key1++, key2++;
  return (!*key1 && !*key2);
}

/* A hash function for LOGFONTWs that takes into consideration only
 * those fields that indicate a specific .ttf file is in use:
 * lfFaceName, lfItalic and lfWeight. Dunno how correct this is.
 */
static guint
logfontw_nosize_hash (const LOGFONTW *lfp)
{
  return case_insensitive_wcs_hash (lfp->lfFaceName) + (lfp->lfItalic != 0) + lfp->lfWeight;
}

/* Ditto comparison function */
static gboolean
logfontw_nosize_equal (const LOGFONTW *lfp1,
                       const LOGFONTW *lfp2)
{
  return (case_insensitive_wcs_equal (lfp1->lfFaceName, lfp2->lfFaceName) &&
          (lfp1->lfItalic != 0) == (lfp2->lfItalic != 0) &&
          lfp1->lfWeight == lfp2->lfWeight);
}

struct PangoAlias
{
  char *alias;
  int n_families;
  char **families;
  gboolean visible; /* Do we want/need this? */
};

static guint
alias_hash (struct PangoAlias *alias)
{
  return g_str_hash (alias->alias);
}

static gboolean
alias_equal (struct PangoAlias *alias1,
             struct PangoAlias *alias2)
{
  return g_str_equal (alias1->alias,
                      alias2->alias);
}

static void
alias_free (struct PangoAlias *alias)
{
  int i;
  g_free (alias->alias);

  for (i = 0; i < alias->n_families; i++)
    g_free (alias->families[i]);

  g_free (alias->families);

  g_slice_free (struct PangoAlias, alias);
}

static void
handle_alias_line (GString    *line_buffer,
                   char       **errstring,
                   GHashTable *ht_aliases)
{
  GString *tmp_buffer1;
  GString *tmp_buffer2;
  const char *pos;
  struct PangoAlias alias_key;
  struct PangoAlias *alias;
  gboolean append = FALSE;
  char **new_families;
  int n_new;
  int i;

  tmp_buffer1 = g_string_new (NULL);
  tmp_buffer2 = g_string_new (NULL);

  pos = line_buffer->str;
  if (!pango_skip_space (&pos))
    return;

  if (!pango_scan_string (&pos, tmp_buffer1) ||
      !pango_skip_space (&pos))
    {
      *errstring = g_strdup ("Line is not of the form KEY=VALUE or KEY+=VALUE");
      goto error;
    }

  if (*pos == '+')
    {
      append = TRUE;
      pos++;
    }

  if (*(pos++) != '=')
    {
      *errstring = g_strdup ("Line is not of the form KEY=VALUE or KEY+=VALUE");
      goto error;
    }

  if (!pango_scan_string (&pos, tmp_buffer2))
    {
      *errstring = g_strdup ("Error parsing value string");
      goto error;
    }
  if (pango_skip_space (&pos))
    {
      *errstring = g_strdup ("Junk after value string");
      goto error;
    }

  alias_key.alias = g_ascii_strdown (tmp_buffer1->str, -1);

  /* Remove any existing values */
  alias = g_hash_table_lookup (ht_aliases, &alias_key);

  if (!alias)
    {
      alias = g_slice_new0 (struct PangoAlias);
      alias->alias = alias_key.alias;

      g_hash_table_insert (ht_aliases, alias, alias);
    }
  else
    g_free (alias_key.alias);

  new_families = g_strsplit (tmp_buffer2->str, ",", -1);

  n_new = 0;
  while (new_families[n_new])
    n_new++;

  if (alias->families && append)
    {
      alias->families = g_realloc (alias->families,
                                   sizeof (char *) *(n_new + alias->n_families));
      for (i = 0; i < n_new; i++)
        alias->families[alias->n_families + i] = new_families[i];
      g_free (new_families);
      alias->n_families += n_new;
    }
  else
    {
      for (i = 0; i < alias->n_families; i++)
        g_free (alias->families[i]);
      g_free (alias->families);
      
      alias->families = new_families;
      alias->n_families = n_new;
    }

 error:
  
  g_string_free (tmp_buffer1, TRUE);
  g_string_free (tmp_buffer2, TRUE);
}

#ifdef HAVE_CAIRO_WIN32

static const char * const builtin_aliases[] = {
  "courier = \"courier new\"",
  /* It sucks to use the same GulimChe, MS Gothic, Sylfaen, Kartika,
   * Latha, Mangal and Raavi fonts for all three of sans, serif and
   * mono, but it isn't like there would be much choice. For most
   * non-Latin scripts that Windows includes any font at all for, it
   * has ony one. One solution is to install the free DejaVu fonts
   * that are popular on Linux. They are listed here first.
   */
  "sans = \"dejavu sans,tahoma,arial unicode ms,lucida sans unicode,browallia new,mingliu,simhei,gulimche,ms gothic,sylfaen,kartika,latha,mangal,raavi\"",
  "sans-serif = \"dejavu sans,tahoma,arial unicode ms,lucida sans unicode,browallia new,mingliu,simhei,gulimche,ms gothic,sylfaen,kartika,latha,mangal,raavi\"",
  "serif = \"dejavu serif,georgia,angsana new,mingliu,simsun,gulimche,ms gothic,sylfaen,kartika,latha,mangal,raavi\"",
 "mono = \"dejavu sans mono,courier new,lucida console,courier monothai,mingliu,simsun,gulimche,ms gothic,sylfaen,kartika,latha,mangal,raavi\"",
  "monospace = \"dejavu sans mono,courier new,lucida console,courier monothai,mingliu,simsun,gulimche,ms gothic,sylfaen,kartika,latha,mangal,raavi\"",
  "emoji = \"segoe ui emoji,segoe ui symbol,segoe ui\"",
  "cursive = \"comic sans ms\"",
  "fantasy = \"gabriola,impact\"",
  "system-ui = \"yu gothic ui,segoe ui,meiryo\"",
};

static void
read_builtin_aliases (GHashTable *ht_aliases)
{

  GString *line_buffer;
  char *errstring = NULL;
  int line;

  line_buffer = g_string_new (NULL);

  for (line = 0; line < G_N_ELEMENTS (builtin_aliases) && errstring == NULL; line++)
    {
      g_string_assign (line_buffer, builtin_aliases[line]);
      handle_alias_line (line_buffer, &errstring, ht_aliases);
    }

  if (errstring)
    {
      g_error ("error in built-in aliases:%d: %s\n", line, errstring);
      g_free (errstring);
    }

  g_string_free (line_buffer, TRUE);
}

#define MAX_VALUE_NAME 16383

static void
read_windows_fallbacks (GHashTable *ht_aliases)
{
  DWORD value_index;
  HKEY hkey;
  LSTATUS status;
  GString *line_buffer;

  /* https://docs.microsoft.com/en-us/globalization/input/font-technology */
  status = RegOpenKeyExW (HKEY_LOCAL_MACHINE,
                          L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontLink\\SystemLink",
                          0,
                          KEY_READ,
                          &hkey);
  if (status != ERROR_SUCCESS)
    return;

  line_buffer = g_string_new (NULL);
  status = ERROR_SUCCESS;
  for (value_index = 0; status != ERROR_NO_MORE_ITEMS; value_index++)
    {
      wchar_t name[MAX_VALUE_NAME];
      DWORD name_length = MAX_VALUE_NAME, value_length = 0;
      char *errstring = NULL;
      gchar *utf8_name;
      wchar_t *value_data, *entry;
      size_t entry_len;

      status = RegEnumValueW (hkey, value_index, name, &name_length,
                              NULL, NULL, NULL, NULL);

      if (status != ERROR_SUCCESS)
        continue;

      utf8_name = g_utf16_to_utf8 (name, -1, NULL, NULL, NULL);
      if (utf8_name == NULL)
        continue;
      g_string_truncate (line_buffer, 0);
      g_string_append_printf (line_buffer,
                              "\"%s\" = \"%s",
                              utf8_name,
                              utf8_name);
      g_free (utf8_name);

      status = RegGetValueW (hkey, NULL, name, RRF_RT_REG_MULTI_SZ,
                             NULL, NULL, &value_length);
      if (status != ERROR_SUCCESS)
        continue;

      value_data = g_malloc (value_length);
      status = RegGetValueW (hkey, NULL, name, RRF_RT_REG_MULTI_SZ, NULL,
                             value_data, &value_length);
      if (status != ERROR_SUCCESS)
        {
          g_free (value_data);
          continue;
        }

      entry = value_data;
      entry_len = wcslen (entry);
      while (entry_len > 0)
        {
          wchar_t *comma;
          gchar *entry_utf8;

          comma = wcsstr (entry, L",");
          /* The value after the first comma, as long as it isn't followed
           * by another comma with a font scale */
          if (comma && wcsstr (comma + 1, L",") == NULL)
            {
              g_string_append (line_buffer, ",");
              entry_utf8 = g_utf16_to_utf8 (comma + 1, -1, NULL, NULL, NULL);
              if (entry_utf8 != NULL)
                {
                  g_string_append (line_buffer, entry_utf8);
                  g_free (entry_utf8);
                }
            }

          entry += entry_len + 1;
          entry_len = wcslen (entry);
        }
      g_free (value_data);

      /* For some reason the default fallback list doesn't cover all of Unicode
       * and Windows has additional fonts for certain languages.
       * Some of them are listed in
       * SOFTWARE\Microsoft\Windows NT\CurrentVersion\FontMapperFamilyFallback
       * but I couldn't find any docs for it. Feel free to improve this */
      g_string_append (line_buffer,
                       ",gisha,leelawadee,arial unicode ms,browallia new,"
                       "mingliu,simhei,gulimche,ms gothic,sylfaen,kartika,"
                       "latha,mangal,raavi");

      g_string_append (line_buffer, "\"");

      handle_alias_line (line_buffer, &errstring, ht_aliases);
      if (errstring != NULL)
        {
          g_warning ("error in windows fallback: %s (%s)\n",
                     errstring, line_buffer->str);
          g_free (errstring);
          errstring = NULL;
        }
    }

  RegCloseKey (hkey);
  g_string_free (line_buffer, TRUE);
}

#endif

static void
lookup_aliases (GHashTable   *aliases_ht,
                const char   *fontname,
                char       ***families,
                int          *n_families)
{
  struct PangoAlias alias_key;
  struct PangoAlias *alias;

  alias_key.alias = g_ascii_strdown (fontname, -1);
  alias = g_hash_table_lookup (aliases_ht, &alias_key);
  g_free (alias_key.alias);

  if (alias)
    {
      *families = alias->families;
      *n_families = alias->n_families;
    }
  else
    {
      *families = NULL;
      *n_families = 0;
    }
}

static void
create_standard_family (PangoWin32FontMap *win32fontmap,
                        const char        *standard_family_name)
{
  int i;
  int n_aliases;
  char **aliases;
  PangoWin32FontMapClass *class = (PangoWin32FontMapClass*) G_OBJECT_GET_CLASS (win32fontmap);

  lookup_aliases (class->aliases, standard_family_name, &aliases, &n_aliases);
  for (i = 0; i < n_aliases; i++)
    {
      PangoWin32Family *existing_family = g_hash_table_lookup (win32fontmap->families, aliases[i]);

      if (existing_family)
        {
          PangoWin32Family *new_family = pango_win32_get_font_family (win32fontmap, standard_family_name);
          GSList *p = existing_family->faces;

          new_family->is_monospace = existing_family->is_monospace;

          while (p)
            {
              const PangoWin32Face *old_face = p->data;
              PangoWin32Face *new_face = g_object_new (PANGO_WIN32_TYPE_FACE, NULL);

              new_face->logfontw = old_face->logfontw;
              new_face->description = pango_font_description_copy_static (old_face->description);
              pango_font_description_set_family_static (new_face->description, standard_family_name);

              if (old_face->coverage != NULL)
                new_face->coverage = pango_coverage_ref (old_face->coverage);
              else
                new_face->coverage = NULL;

              new_face->face_name = NULL;

              new_face->is_synthetic = TRUE;

              new_face->cached_fonts = NULL;

              new_face->family = new_family;

              new_family->faces = g_slist_append (new_family->faces, new_face);

              p = p->next;
            }
          return;
        }
    }

  /* XXX What to do if none of the members of aliases for standard_family_name
   * exists on this machine?
   */
}

static void
pango_win32_font_map_init (PangoWin32FontMap *win32fontmap)
{
  HDC hdc = _pango_win32_get_display_dc ();

  win32fontmap->families =
    g_hash_table_new_full ((GHashFunc) case_insensitive_str_hash,
                           (GEqualFunc) case_insensitive_str_equal, NULL, g_object_unref);
  win32fontmap->fonts =
    g_hash_table_new_full ((GHashFunc) logfontw_nosize_hash,
                           (GEqualFunc) logfontw_nosize_equal,
                           NULL,
                           g_object_unref);

  win32fontmap->font_cache = pango_win32_font_cache_new ();
  win32fontmap->freed_fonts = g_queue_new ();

  pango_win32_dwrite_font_map_populate (win32fontmap);

  /* Create synthetic "Sans", "Sans-Serif", "Serif", "Monospace", "Cursive", "Fantasy" and "System-ui" families */
  create_standard_family (win32fontmap, "Sans");
  create_standard_family (win32fontmap, "Sans-Serif");
  create_standard_family (win32fontmap, "Serif");
  create_standard_family (win32fontmap, "Monospace");
  create_standard_family (win32fontmap, "Cursive");
  create_standard_family (win32fontmap, "Fantasy");
  create_standard_family (win32fontmap, "System-ui");

  win32fontmap->dpi = 96.0;
}

static void
pango_win32_font_map_fini (PangoWin32FontMap *win32fontmap)
{
  g_list_foreach (win32fontmap->freed_fonts->head, (GFunc)g_object_unref, NULL);
  g_queue_free (win32fontmap->freed_fonts);

  pango_win32_font_cache_free (win32fontmap->font_cache);

  g_hash_table_destroy (win32fontmap->fonts);
  g_hash_table_destroy (win32fontmap->families);
  g_clear_pointer (&win32fontmap->families_list, g_free);
}

static void
pango_win32_font_map_fontset_add_fonts (PangoFontMap          *fontmap,
                                        PangoContext          *context,
                                        PangoFontsetSimple *fonts,
                                        PangoFontDescription  *desc,
                                        const char            *family)
{
  /* Mostly use the "old" pango_font_map_fontset_add_fonts() */
  /* on Windows so that we can go through the .aliases file */
  /* to load the appropriate fontset for various texts */
  PangoFont *font;
  char **aliases;
  int n_aliases;
  int j;
  PangoWin32FontMap *win32fontmap = PANGO_WIN32_FONT_MAP (fontmap);
  PangoWin32FontMapClass *class = (PangoWin32FontMapClass*)G_OBJECT_GET_CLASS(win32fontmap);

  lookup_aliases (class->aliases, family, &aliases, &n_aliases);
  if (n_aliases)
  {
    for (j = 0; j < n_aliases; j++)
    {
      pango_font_description_set_family_static (desc, aliases[j]);
      font = pango_win32_font_map_load_font (fontmap, context, desc);
      if (font)
        pango_fontset_simple_append (fonts, font);
    }
  }
  else
  {
    pango_font_description_set_family_static (desc, family);
    font = pango_win32_font_map_load_font (fontmap, context, desc);
    if (font)
      pango_fontset_simple_append (fonts, font);
  }
}

static PangoFontFace *
pango_win32_font_map_get_face (PangoFontMap *fontmap,
                               PangoFont    *font)
{
  PangoWin32Font *win32font = PANGO_WIN32_FONT (font);

  return PANGO_FONT_FACE (win32font->win32face);
}

static guint
pango_win32_font_map_get_serial (PangoFontMap *fontmap)
{
  PangoWin32FontMap *win32fontmap = PANGO_WIN32_FONT_MAP (fontmap);

  return win32fontmap->serial;
}

static void
pango_win32_font_map_changed (PangoFontMap *fontmap)
{
  PangoWin32FontMap *win32fontmap = PANGO_WIN32_FONT_MAP (fontmap);

  win32fontmap->serial++;
  if (win32fontmap->serial == 0)
    win32fontmap->serial++;

  pango_win32_font_map_cache_clear (fontmap);
}

static void
pango_win32_font_map_class_init (PangoWin32FontMapClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontMapClass *fontmap_class = PANGO_FONT_MAP_CLASS (class);
  PangoFontMapClassPrivate *pclass;

  class->find_font = pango_win32_font_map_real_find_font;
  object_class->finalize = pango_win32_font_map_finalize;
  fontmap_class->load_font = pango_win32_font_map_real_load_font;
  /* we now need a load_fontset implementation for the Win32 backend */
  fontmap_class->load_fontset = pango_win32_font_map_load_fontset;
  fontmap_class->list_families = pango_win32_font_map_list_families;
  fontmap_class->shape_engine_type = PANGO_RENDER_TYPE_WIN32;
  fontmap_class->get_face = pango_win32_font_map_get_face;
  fontmap_class->get_serial = pango_win32_font_map_get_serial;
  fontmap_class->changed = pango_win32_font_map_changed;
  class->aliases = g_hash_table_new_full ((GHashFunc)alias_hash,
                                          (GEqualFunc)alias_equal,
                                          (GDestroyNotify)alias_free,
                                          NULL);

  pclass = g_type_class_get_private ((GTypeClass *) class, PANGO_TYPE_FONT_MAP);

  pclass->add_font_file = pango_win32_font_map_add_font_file;

#ifdef HAVE_CAIRO_WIN32
  read_windows_fallbacks (class->aliases);
  read_builtin_aliases (class->aliases);
#endif

  _pango_win32_get_display_dc ();
}

/**
 * pango_win32_font_map_for_display:
 *
 * Returns a `PangoWin32FontMap`.
 *
 * Font maps are cached and should not be freed. If
 * the font map is no longer needed, it can be released
 * with [func@Pango.win32_shutdown_display].
 *
 * Return value: a `PangoFontMap`
 **/
PangoFontMap *
pango_win32_font_map_for_display (void)
{
  if (g_once_init_enter ((gsize*)&default_fontmap))
    g_once_init_leave((gsize*)&default_fontmap, (gsize)g_object_new (PANGO_TYPE_WIN32_FONT_MAP, NULL));

  return PANGO_FONT_MAP (default_fontmap);
}

/**
 * pango_win32_shutdown_display:
 *
 * Free cached resources.
 **/
void
pango_win32_shutdown_display (void)
{
  if (default_fontmap)
    {
      pango_win32_fontmap_cache_clear (default_fontmap);
      g_object_unref (default_fontmap);

      default_fontmap = NULL;
    }
}

static inline void
pangowin32_release_com_obj (gpointer object)
{
  IUnknown *This = object;

  This->lpVtbl->Release (This);
}

static void
pango_win32_font_map_finalize (GObject *object)
{
  PangoWin32FontMap *win32fontmap = PANGO_WIN32_FONT_MAP (object);

  pango_win32_font_map_fini (win32fontmap);

  g_clear_pointer (&win32fontmap->font_set_builder1, pangowin32_release_com_obj);
  g_clear_pointer (&win32fontmap->font_set_builder, pangowin32_release_com_obj);

  G_OBJECT_CLASS (pango_win32_font_map_parent_class)->finalize (object);
}

/*
 * PangoWin32Family
 */
static void
pango_win32_family_list_faces (PangoFontFamily  *family,
                               PangoFontFace  ***faces,
                               int              *n_faces)
{
  PangoWin32Family *win32family = PANGO_WIN32_FAMILY (family);
  GSList *p;
  int n;

  p = win32family->faces;
  n = 0;
  while (p)
    {
      n++;
      p = p->next;
    }

  if (faces)
    {
      int i;

      *faces = g_new (PangoFontFace *, n);

      p = win32family->faces;
      i = 0;
      while (p)
        {
          (*faces)[i++] = p->data;
          p = p->next;
        }
    }
  if (n_faces)
    *n_faces = n;
}

static const char *
pango_win32_family_get_name (PangoFontFamily  *family)
{
  PangoWin32Family *win32family = PANGO_WIN32_FAMILY (family);

  return win32family->family_name;
}

static gboolean
pango_win32_family_is_monospace (PangoFontFamily *family)
{
  PangoWin32Family *win32family = PANGO_WIN32_FAMILY (family);

  return win32family->is_monospace;
}

G_DEFINE_TYPE (PangoWin32Family, pango_win32_family, PANGO_TYPE_FONT_FAMILY)

static void
pango_win32_family_finalize (GObject *object)
{
  PangoWin32Family *win32family = PANGO_WIN32_FAMILY (object);

  g_free (win32family->family_name);

  g_slist_free_full (win32family->faces, g_object_unref);

  G_OBJECT_CLASS (pango_win32_family_parent_class)->finalize (object);
}

static void
pango_win32_family_class_init (PangoFontFamilyClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = pango_win32_family_finalize;
  class->list_faces = pango_win32_family_list_faces;
  class->get_name = pango_win32_family_get_name;
  class->is_monospace = pango_win32_family_is_monospace;
}

static void
pango_win32_family_init (PangoWin32Family *family)
{
}

static void
pango_win32_font_map_list_families (PangoFontMap      *fontmap,
                                    PangoFontFamily ***families,
                                    int               *n_families)
{
  PangoWin32FontMap *self = (PangoWin32FontMap *) fontmap;
  gsize n;

  n = g_hash_table_size (self->families);

  if (n_families)
    *n_families = n;

  if (families)
    {
      pango_win32_font_map_ensure_families_list (self);
      *families = g_memdup (self->families_list, sizeof (PangoFontFamily **) * n);
    }
}

static PangoWin32Family *
pango_win32_get_font_family (PangoWin32FontMap *win32fontmap,
                             const char        *family_name)
{
  PangoWin32Family *win32family = g_hash_table_lookup (win32fontmap->families, family_name);
  if (!win32family)
    {
      win32family = g_object_new (PANGO_WIN32_TYPE_FAMILY, NULL);
      win32family->family_name = g_strdup (family_name);
      win32family->faces = NULL;

      g_hash_table_insert (win32fontmap->families, win32family->family_name, win32family);
      g_clear_pointer (&win32fontmap->families_list, g_free);
    }

  return win32family;
}

static gboolean
get_first_font (PangoFontset *fontset G_GNUC_UNUSED,
                PangoFont    *font,
                gpointer      data)
{
  *(PangoFont **)data = font;

  return TRUE;
}

static PangoFont *
pango_win32_font_map_real_load_font (PangoFontMap               *fontmap,
                                     PangoContext               *context,
                                     const PangoFontDescription *description)
{
  PangoLanguage *language;
  PangoFontset *fontset;
  PangoFont *font = NULL;

  if (context)
    language = pango_context_get_language (context);
  else
    language = NULL;

  fontset = pango_font_map_load_fontset (fontmap, context,
                                         description, language);

  if (fontset)
    {
      pango_fontset_foreach (fontset, get_first_font, &font);

      if (font)
        g_object_ref (font);

      g_object_unref (fontset);
    }

  return font;
}

static PangoFont *
pango_win32_font_map_load_font (PangoFontMap               *fontmap,
                                PangoContext               *context,
                                const PangoFontDescription *description)
{
  PangoWin32FontMap *win32fontmap = (PangoWin32FontMap *)fontmap;
  PangoWin32Face *best_match = NULL;
  PangoFont *result = NULL;
  GSList *tmp_list;
  const char *family;
  char **families;
  int i;

  g_return_val_if_fail (description != NULL, NULL);

  family = pango_font_description_get_family (description);
  families = g_strsplit (family ? family : "", ",", -1);

  for (i = 0; families[i] && !best_match; ++i)
    {
      PangoWin32Family *win32family;
      PING (("name=%s", families[i]));

      win32family = g_hash_table_lookup (win32fontmap->families, families[i]);
      if (win32family)
        {
          PangoFontDescription *new_desc;
          PangoFontDescription *best_desc = NULL;

          PING (("got win32family"));
          tmp_list = win32family->faces;

          while (tmp_list)
            {
              PangoWin32Face *face = tmp_list->data;
              new_desc = pango_font_face_describe (PANGO_FONT_FACE (face));
              pango_font_description_set_gravity (new_desc,
                                                  pango_font_description_get_gravity (description));

              if (pango_font_description_better_match (description,
                                                       best_desc,
                                                       new_desc))
                {
                  pango_font_description_free (best_desc);
                  best_desc = new_desc;
                  best_match = face;
                }
              else
                pango_font_description_free (new_desc);

              tmp_list = tmp_list->next;
            }

          if (best_desc != NULL)
            pango_font_description_free (best_desc);
        }
    }
  
  g_strfreev (families);

  if (best_match)
    result = PANGO_WIN32_FONT_MAP_GET_CLASS (win32fontmap)->find_font (win32fontmap, context,
                                                                       best_match,
                                                                       description);
      /* TODO: Handle the case that result == NULL. */
  else
    PING (("no best match!"));

  return result;
}

static PangoWin32Font *
pango_win32_font_neww (PangoFontMap   *fontmap,
                       const LOGFONTW *lfp,
                       int             size)
{
  PangoWin32Font *result;

  g_return_val_if_fail (fontmap != NULL, NULL);
  g_return_val_if_fail (lfp != NULL, NULL);

  result = (PangoWin32Font *)g_object_new (PANGO_TYPE_WIN32_FONT, NULL);

  if (G_UNLIKELY(result->fontmap))
    return result;

  if (fontmap)
    {
      result->fontmap = fontmap;
      g_object_add_weak_pointer (G_OBJECT (fontmap), (gpointer *) &result->fontmap);
    }

  result->size = size;
  _pango_win32_make_matching_logfontw (fontmap, lfp, size, &result->logfontw);
  result->is_hinted = pango_win32_dwrite_font_check_is_hinted (result);

  return result;
}

static PangoFont *
pango_win32_font_map_real_find_font (PangoWin32FontMap          *win32fontmap,
                                     PangoContext               *context,
                                     PangoWin32Face             *face,
                                     const PangoFontDescription *description)
{
  PangoFontMap *fontmap = PANGO_FONT_MAP (win32fontmap);
  PangoWin32Font *win32font;
  GSList *tmp_list = face->cached_fonts;
  int size = pango_font_description_get_size (description);

  if (pango_font_description_get_size_is_absolute (description))
    size = (int) 0.5 + (size * 72 / win32fontmap->dpi);

  PING (("got best match:%S size=%d",face->logfontw.lfFaceName,size));

  while (tmp_list)
    {
      win32font = tmp_list->data;
      if (win32font->size == size)
        {
          PING (("size matches"));

          g_object_ref (win32font);
          if (win32font->in_cache)
            _pango_win32_fontmap_cache_remove (fontmap, win32font);

          return (PangoFont *)win32font;
        }

      tmp_list = tmp_list->next;
    }

  win32font = pango_win32_font_neww (fontmap, &face->logfontw, size);

  if (!win32font)
    return NULL;

  win32font->win32face = g_object_ref (face);
  face->cached_fonts = g_slist_prepend (face->cached_fonts, win32font);

  return (PangoFont *)win32font;
}

/**
 * pango_win32_font_description_from_logfont:
 * @lfp: a LOGFONTA
 *
 * Creates a `PangoFontDescription` that matches the specified LOGFONTA.
 *
 * The face name, italicness and weight fields in the LOGFONTA are used
 * to set up the resulting `PangoFontDescription`. If the face name in
 * the LOGFONTA contains non-ASCII characters the font is temporarily
 * loaded (using CreateFontIndirect()) and an ASCII (usually English)
 * name for it is looked up from the font name tables in the font
 * data. If that doesn't work, the face name is converted from the
 * system codepage to UTF-8 and that is used.
 *
 * Return value: the newly allocated `PangoFontDescription`, which
 *  should be freed using [method@Pango.FontDescription.free]
 *
 * Since: 1.12
 */
PangoFontDescription *
pango_win32_font_description_from_logfont (const LOGFONT *lfp)
{
  LOGFONTW lfw;
  PangoFontDescription *desc = NULL;
  gchar *facename_utf8 = g_locale_to_utf8 (lfp->lfFaceName, -1, NULL, NULL, NULL);
  wchar_t *facename_utf16 = g_utf8_to_utf16 (facename_utf8, -1, NULL, NULL, NULL);

  lfw.lfHeight = lfp->lfHeight;
  lfw.lfWidth = lfp->lfWidth;
  lfw.lfEscapement = lfp->lfEscapement;
  lfw.lfOrientation = lfp->lfOrientation;
  lfw.lfWeight = lfp->lfWeight;
  lfw.lfItalic = lfp->lfItalic;
  lfw.lfUnderline = lfp->lfUnderline;
  lfw.lfStrikeOut = lfp->lfStrikeOut;
  lfw.lfCharSet = lfp->lfCharSet;
  lfw.lfOutPrecision = lfp->lfOutPrecision;
  lfw.lfClipPrecision = lfp->lfClipPrecision;
  lfw.lfQuality = lfp->lfQuality;
  lfw.lfPitchAndFamily = lfp->lfPitchAndFamily;
  wcscpy (lfw.lfFaceName, facename_utf16);

  desc = pango_win32_font_description_from_logfontw_dwrite (&lfw);
  g_free (facename_utf16);
  g_free (facename_utf8);

  return desc;
}

/**
 * pango_win32_font_description_from_logfontw:
 * @lfp: a LOGFONTW
 *
 * Creates a `PangoFontDescription` that matches the specified LOGFONTW.
 *
 * The face name, italicness and weight fields in the LOGFONTW are used
 * to set up the resulting `PangoFontDescription`. If the face name in
 * the LOGFONTW contains non-ASCII characters the font is temporarily
 * loaded (using CreateFontIndirect()) and an ASCII (usually English)
 * name for it is looked up from the font name tables in the font
 * data. If that doesn't work, the face name is converted from UTF-16
 * to UTF-8 and that is used.
 *
 * Return value: the newly allocated `PangoFontDescription`, which
 *  should be freed using [method@Pango.FontDescription.free]
 *
 * Since: 1.16
 */
PangoFontDescription *
pango_win32_font_description_from_logfontw (const LOGFONTW *lfp)
{
  return pango_win32_font_description_from_logfontw_dwrite (lfp);
}

static char *
charset_name (int charset, char* num)
{
  switch (charset)
    {
#define CASE(x) case x##_CHARSET: return (char *) #x
      CASE (ANSI);
      CASE (DEFAULT);
      CASE (SYMBOL);
      CASE (SHIFTJIS);
      CASE (HANGUL);
      CASE (GB2312);
      CASE (CHINESEBIG5);
      CASE (GREEK);
      CASE (TURKISH);
      CASE (HEBREW);
      CASE (ARABIC);
      CASE (BALTIC);
      CASE (RUSSIAN);
      CASE (THAI);
      CASE (EASTEUROPE);
      CASE (OEM);
      CASE (JOHAB);
      CASE (VIETNAMESE);
      CASE (MAC);
#undef CASE
    default:
      sprintf (num, "%d", charset);
      return num;
    }
}

static char *
ff_name (int ff, char* num)
{
  switch (ff)
    {
#define CASE(x) case FF_##x: return (char *) #x
      CASE (DECORATIVE);
      CASE (DONTCARE);
      CASE (MODERN);
      CASE (ROMAN);
      CASE (SCRIPT);
      CASE (SWISS);
#undef CASE
    default:
      sprintf (num, "%d", ff);
      return num;
    }
}

void
pango_win32_insert_font (PangoWin32FontMap *win32fontmap,
                         LOGFONTW          *lfp,
                         gpointer           dwrite_font,
                         gboolean           is_synthetic)
{
  PangoFontDescription *description;
  PangoWin32Family *win32family;
  PangoWin32Face *win32face;

  char tmp_for_charset_name[10];
  char tmp_for_ff_name[10];

  PING (("face=%S,charset=%s,it=%s,wt=%ld,ht=%ld,ff=%s%s",
         lfp->lfFaceName,
         charset_name (lfp->lfCharSet, tmp_for_charset_name),
         lfp->lfItalic ? "yes" : "no",
         lfp->lfWeight,
         lfp->lfHeight,
         ff_name (lfp->lfPitchAndFamily & 0xF0, tmp_for_ff_name),
         is_synthetic ? " synthetic" : ""));

  /* Ignore Symbol fonts (which don't have any Unicode mapping
   * table). We could also be fancy and use the PostScript glyph name
   * table for such if present, and build a Unicode map by mapping
   * each PostScript glyph name to Unicode character. Oh well.
   */
  if (lfp->lfCharSet == SYMBOL_CHARSET)
    return;

  if (g_hash_table_lookup (win32fontmap->fonts, lfp))
    {
      PING (("already have it"));
      return;
    }

  PING (("not found"));

  if (dwrite_font == NULL)
    {
      dwrite_font = pango_win32_logfontw_get_dwrite_font (lfp);
      description = pango_win32_font_description_from_logfontw (lfp);
    }
  else
    {
      description = pango_win32_font_description_from_dwrite_font (dwrite_font);
    }

  /* In some cases, extracting a name for a font can fail; such fonts
   * aren't usable for us
   */
  if (!pango_font_description_get_family (description))
    {
      pangowin32_release_com_obj (dwrite_font);
      pango_font_description_free (description);
      return;
    }

  win32face = g_object_new (PANGO_WIN32_TYPE_FACE, NULL);

  PING (("win32face created: %p for %S", win32face, lfp->lfFaceName));

  win32face->logfontw = *lfp;
  win32face->dwrite_font = dwrite_font;
  win32face->description = description;

  win32face->coverage = NULL;

  win32face->face_name = NULL;

  win32face->is_synthetic = is_synthetic;

  win32face->cached_fonts = NULL;

  win32face->family = win32family =
    pango_win32_get_font_family (win32fontmap,
                                 pango_font_description_get_family (win32face->description));

  if (!pango_win32_dwrite_font_is_monospace (dwrite_font, &win32family->is_monospace))
    {
      if ((lfp->lfPitchAndFamily & 0xF0) == FF_MODERN)
        win32family->is_monospace = TRUE;
    }

  win32family->faces = g_slist_append (win32family->faces, win32face);

  PING (("name=%s, length(faces)=%d",
        win32family->family_name, g_slist_length (win32family->faces)));

  g_hash_table_insert (win32fontmap->fonts, &win32face->logfontw, g_object_ref (win32face));
}

/* Given a LOGFONTW and size, make a matching LOGFONTW corresponding to
 * an installed font.
 */
void
_pango_win32_make_matching_logfontw (PangoFontMap   *fontmap,
                                     const LOGFONTW *lfp,
                                     int             size,
                                     LOGFONTW       *out)
{
  PangoWin32FontMap *win32fontmap;
  gpointer match;

  PING (("lfp.face=%S,wt=%ld,ht=%ld,size:%d",
        lfp->lfFaceName, lfp->lfWeight, lfp->lfHeight, size));
  win32fontmap = PANGO_WIN32_FONT_MAP (fontmap);

  if (!g_hash_table_lookup_extended (win32fontmap->fonts, lfp, &match, NULL))
    {
      PING (("not found"));
      return;
    }

  /* OK, we have a match; let's modify it to fit this size */

  *out = * (LOGFONTW *) match;
  out->lfHeight = -(int) ((double) size * win32fontmap->dpi / (72 * PANGO_SCALE) + 0.5);
  out->lfWidth = 0;
}

static PangoFontDescription *
pango_win32_face_describe (PangoFontFace *face)
{
  PangoWin32Face *win32face = PANGO_WIN32_FACE (face);

  return pango_font_description_copy (win32face->description);
}

static const char *
pango_win32_face_get_face_name (PangoFontFace *face)
{
  PangoWin32Face *win32face = PANGO_WIN32_FACE (face);

  if (!win32face->face_name)
    {
      PangoFontDescription *desc = pango_font_face_describe (face);

      pango_font_description_unset_fields (desc,
                                           PANGO_FONT_MASK_FAMILY | PANGO_FONT_MASK_SIZE);

      win32face->face_name = pango_font_description_to_string (desc);
      pango_font_description_free (desc);
    }

  return win32face->face_name;
}

static gboolean
pango_win32_face_is_synthesized (PangoFontFace *face)
{
  PangoWin32Face *win32face = PANGO_WIN32_FACE (face);

  return win32face->is_synthetic;
}

static PangoFontFamily *
pango_win32_face_get_family (PangoFontFace *face)
{
  PangoWin32Face *win32face = PANGO_WIN32_FACE (face);

  return PANGO_FONT_FAMILY (win32face->family);
}

G_DEFINE_TYPE (PangoWin32Face, pango_win32_face, PANGO_TYPE_FONT_FACE)

static void
pango_win32_face_finalize (GObject *object)
{
  PangoWin32Face *win32face = PANGO_WIN32_FACE (object);

  g_clear_pointer (&win32face->dwrite_font, pangowin32_release_com_obj);

  pango_font_description_free (win32face->description);

  if (win32face->coverage != NULL)
    g_object_unref (win32face->coverage);

  g_free (win32face->face_name);

  g_assert (win32face->cached_fonts == NULL);

  G_OBJECT_CLASS (pango_win32_family_parent_class)->finalize (object);
}

static void
pango_win32_face_class_init (PangoFontFaceClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->finalize = pango_win32_face_finalize;
  class->describe = pango_win32_face_describe;
  class->get_face_name = pango_win32_face_get_face_name;
  class->list_sizes = pango_win32_face_list_sizes;
  class->is_synthesized = pango_win32_face_is_synthesized;
  class->get_family = pango_win32_face_get_family;
}

static void
pango_win32_face_init (PangoWin32Face *face)
{
}

static void
pango_win32_face_list_sizes (PangoFontFace  *face,
                             int           **sizes,
                             int            *n_sizes)
{
  /*
   * for scalable fonts it's simple, and currently we only have such
   * see : pango_win32_enum_proc(), TRUETYPE_FONTTYPE
   */
  if (sizes)
    *sizes = NULL;
  *n_sizes = 0;
}

/**
 * pango_win32_font_map_get_font_cache:
 * @font_map: a `PangoWin32FontMap`
 *
 * Obtains the font cache associated with the given font map.
 *
 * Return value: the `PangoWin32FontCache` of @font_map.
 */
PangoWin32FontCache *
pango_win32_font_map_get_font_cache (PangoFontMap *font_map)
{
  if (G_UNLIKELY (!font_map))
    return NULL;

  g_return_val_if_fail (PANGO_WIN32_IS_FONT_MAP (font_map), NULL);

  return PANGO_WIN32_FONT_MAP (font_map)->font_cache;
}

void
_pango_win32_fontmap_cache_remove (PangoFontMap   *fontmap,
                                   PangoWin32Font *win32font)
{
  PangoWin32FontMap *win32fontmap = PANGO_WIN32_FONT_MAP (fontmap);
  GList *link = g_queue_find (win32fontmap->freed_fonts, win32font);

  if (link)
    g_queue_delete_link (win32fontmap->freed_fonts, link);
  win32font->in_cache = FALSE;

  g_object_unref (win32font);
}

static void
pango_win32_fontmap_cache_clear (PangoWin32FontMap *win32fontmap)
{
  g_list_foreach (win32fontmap->freed_fonts->head, (GFunc)g_object_unref, NULL);
  g_queue_free (win32fontmap->freed_fonts);
  win32fontmap->freed_fonts = g_queue_new ();
}

static PangoFontset *
pango_win32_font_map_load_fontset (PangoFontMap                 *fontmap,
                                   PangoContext                 *context,
                                   const PangoFontDescription   *desc,
                                   PangoLanguage                *language)
{
  const unsigned int MAX_WARNED_FONTS_CACHE = 50;
  static GHashTable *warned_fonts = NULL; /* MT-safe */
  G_LOCK_DEFINE_STATIC (warned_fonts);
  PangoFontDescription *tmp_desc;
  const char *family;
  char **families;
  int i;
  PangoFontsetSimple *fonts;

  g_return_val_if_fail (fontmap != NULL, NULL);

  family = pango_font_description_get_family (desc);
  families = g_strsplit (family ? family : "", ",", -1);

  tmp_desc = pango_font_description_copy_static (desc);

  fonts = pango_fontset_simple_new (language);

  for (i = 0; families[i]; i++)
    pango_win32_font_map_fontset_add_fonts (fontmap,
                                            context,
                                            fonts,
                                            tmp_desc,
                                            families[i]);

  g_strfreev (families);

  if (!warned_fonts)
    warned_fonts = g_hash_table_new (g_str_hash, g_str_equal);

  /* The font description was completely unloadable, try with
   * family == "Sans"
   */
  if (pango_fontset_simple_size (fonts) == 0)
    {
      char *ctmp1, *ctmp2;

      pango_font_description_set_family_static (tmp_desc,
                                                pango_font_description_get_family (desc));

      ctmp1 = pango_font_description_to_string (desc);
      pango_font_description_set_family_static (tmp_desc, "Sans");

      G_LOCK (warned_fonts);
      if (!g_hash_table_lookup (warned_fonts, ctmp1))
        {
          if (g_hash_table_size (warned_fonts) >= MAX_WARNED_FONTS_CACHE)
            g_hash_table_remove_all (warned_fonts);

          g_hash_table_insert (warned_fonts, g_strdup (ctmp1), GINT_TO_POINTER (1));

          ctmp2 = pango_font_description_to_string (tmp_desc);
          g_printerr ("couldn't load font \"%s\", falling back to \"%s\", "
                      "expect ugly output.", ctmp1, ctmp2);
          g_free (ctmp2);
        }
      G_UNLOCK (warned_fonts);
      g_free (ctmp1);

      pango_win32_font_map_fontset_add_fonts (fontmap,
                                              context,
                                              fonts,
                                              tmp_desc,
                                              "Sans");
    }

  /* We couldn't try with Sans and the specified style. Try Sans Normal */
  if (pango_fontset_simple_size (fonts) == 0)
    {
      char *ctmp1, *ctmp2;

      pango_font_description_set_family_static (tmp_desc, "Sans");
      ctmp1 = pango_font_description_to_string (tmp_desc);
      pango_font_description_set_style (tmp_desc, PANGO_STYLE_NORMAL);
      pango_font_description_set_weight (tmp_desc, PANGO_WEIGHT_NORMAL);
      pango_font_description_set_variant (tmp_desc, PANGO_VARIANT_NORMAL);
      pango_font_description_set_stretch (tmp_desc, PANGO_STRETCH_NORMAL);

      G_LOCK (warned_fonts);
      if (!g_hash_table_lookup (warned_fonts, ctmp1))
        {
          if (g_hash_table_size (warned_fonts) >= MAX_WARNED_FONTS_CACHE)
            g_hash_table_remove_all (warned_fonts);

          g_hash_table_insert (warned_fonts, g_strdup (ctmp1), GINT_TO_POINTER (1));

          ctmp2 = pango_font_description_to_string (tmp_desc);

          g_printerr ("couldn't load font \"%s\", falling back to \"%s\", "
                      "expect ugly output.", ctmp1, ctmp2);
          g_free (ctmp2);
        }
      G_UNLOCK (warned_fonts);
      g_free (ctmp1);

      pango_win32_font_map_fontset_add_fonts (fontmap,
                                              context,
                                              fonts,
                                              tmp_desc,
                                              "Sans");
    }

  pango_font_description_free (tmp_desc);

  /* Everything failed, we are screwed, there is no way to continue,
   * but lets just not crash here.
   */
  if (pango_fontset_simple_size (fonts) == 0)
      g_warning ("All font fallbacks failed!!!!");

  return PANGO_FONTSET (fonts);
}

/*<private>
 * pango_win32_font_map_cache_clear:
 * @font_map: a `PangoWin32FontMap`
 *
 * Clear all cached information and fontsets for this font map.
 *
 * This should be called whenever the application wishes to add an
 * `IDirectWriteFontSet` for the @font_map; this is automatically called
 * when using the `pango_win32_font_map_add_font_file()` function.
 *
 * Since: 1.52
 */
void
pango_win32_font_map_cache_clear (PangoFontMap *font_map)
{
  PangoWin32FontMap *win32fontmap;
  int removed, added;

  g_return_if_fail (PANGO_WIN32_IS_FONT_MAP (font_map));

  win32fontmap = PANGO_WIN32_FONT_MAP (font_map);

  removed = g_list_model_get_n_items (G_LIST_MODEL (font_map));
  pango_win32_font_map_fini (win32fontmap);
  pango_win32_font_map_init (win32fontmap);
  added = g_list_model_get_n_items (G_LIST_MODEL (font_map));

  g_list_model_items_changed (G_LIST_MODEL (font_map), 0, removed, added);

  if (removed != added)
    g_object_notify (G_OBJECT (font_map), "n-items");
}

/**
 * pango_win32_font_map_add_font_file:
 * @font_map: a `PangoWin32FontMap`
 * @font_file_path: Path to the actual font file
 * @error: return location for an error
 *
 * Loads a font file with one or more fonts into the `PangoWin32FontMap` specified
 * by the path. The font file must be in a format that is supported by the system's
 * DirectWrite APIs.
 *
 * Return value: TRUE if the font file is successfully loaded into the `PangoWin32FontMap`;
 *   otherwise FALSE.
 *
 * Since: 1.52
 * Deprecated: 1.56: Use pango_font_map_add_font_file instead
 */
gboolean
pango_win32_font_map_add_font_file (PangoFontMap  *font_map,
                                    const char    *font_file_path,
                                    GError       **error)
{
  g_return_val_if_fail (PANGO_WIN32_IS_FONT_MAP (font_map), FALSE);

  return pango_win32_dwrite_add_font_file (font_map, font_file_path, error);
}
