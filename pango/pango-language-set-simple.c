/* Pango
 *
 * Copyright (C) 2021 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gio/gio.h>

#include "pango-language-set-simple-private.h"

struct _PangoLanguageSetSimple
{
  PangoLanguageSet parent_instance;

  GHashTable *languages;
  PangoLanguage **list;
};

G_DEFINE_TYPE (PangoLanguageSetSimple, pango_language_set_simple, PANGO_TYPE_LANGUAGE_SET)

static void
pango_language_set_simple_init (PangoLanguageSetSimple *self)
{
  self->languages = g_hash_table_new (g_direct_hash, g_direct_equal);
}

static void
pango_language_set_simple_finalize (GObject *object)
{
  PangoLanguageSetSimple *self = PANGO_LANGUAGE_SET_SIMPLE (object);

  g_hash_table_unref (self->languages);
  g_free (self->list);

  G_OBJECT_CLASS (pango_language_set_simple_parent_class)->finalize (object);
}

static gboolean
pango_language_set_simple_matches_language (PangoLanguageSet *set,
                                            PangoLanguage    *language)
{
  PangoLanguageSetSimple *self = PANGO_LANGUAGE_SET_SIMPLE (set);
  const char *s;

  if (g_hash_table_contains (self->languages, language))
    return TRUE;

  if (language == pango_language_from_string ("c"))
    return TRUE;

  s = pango_language_to_string (language);
  if (strchr (s, '-'))
    {
      char buf[10];

      for (int i = 0; i < 10; i++)
        {
          buf[i] = s[i];
          if (buf[i] == '-')
            {
              buf[i] = '\0';
              break;
            }
        }
      buf[9] = '\0';

      if (g_hash_table_contains (self->languages, pango_language_from_string (buf)))
        return TRUE;
    }

  return FALSE;
}

static PangoLanguage **
pango_language_set_simple_get_languages (PangoLanguageSet *set)
{
  PangoLanguageSetSimple *self = PANGO_LANGUAGE_SET_SIMPLE (set);

  if (!self->list)
    self->list = (PangoLanguage **) g_hash_table_get_keys_as_array (self->languages, NULL);

  return self->list;
}

static void
pango_language_set_simple_class_init (PangoLanguageSetSimpleClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoLanguageSetClass *language_set_class = PANGO_LANGUAGE_SET_CLASS (class);

  object_class->finalize = pango_language_set_simple_finalize;
  language_set_class->matches_language = pango_language_set_simple_matches_language;
  language_set_class->get_languages = pango_language_set_simple_get_languages;
}

PangoLanguageSetSimple *
pango_language_set_simple_new (void)
{
  return g_object_new (PANGO_TYPE_LANGUAGE_SET_SIMPLE, NULL);
}

void
pango_language_set_simple_add_language (PangoLanguageSetSimple *self,
                                        PangoLanguage          *language)
{
  g_return_if_fail (self->list == NULL);

  g_hash_table_add (self->languages, language);
}
