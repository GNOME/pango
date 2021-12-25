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

#include "pango-language-set-private.h"


G_DEFINE_ABSTRACT_TYPE (PangoLanguageSet, pango_language_set, G_TYPE_OBJECT)

static void
pango_language_set_init (PangoLanguageSet *set)
{
}

static gboolean
pango_language_set_default_matches_language (PangoLanguageSet *set,
                                             PangoLanguage    *language)
{
  return FALSE;
}

static PangoLanguage **
pango_language_set_default_get_languages (PangoLanguageSet *set)
{
  return NULL;
}

static void
pango_language_set_class_init (PangoLanguageSetClass *class)
{
  PangoLanguageSetClass *language_set_class = PANGO_LANGUAGE_SET_CLASS (class);

  language_set_class->matches_language = pango_language_set_default_matches_language;
  language_set_class->get_languages = pango_language_set_default_get_languages;
}

gboolean
pango_language_set_matches_language (PangoLanguageSet *set,
                                     PangoLanguage    *language)
{
  return PANGO_LANGUAGE_SET_GET_CLASS (set)->matches_language (set, language);
}

PangoLanguage **
pango_language_set_get_languages (PangoLanguageSet *set)
{
  return PANGO_LANGUAGE_SET_GET_CLASS (set)->get_languages (set);
}

char *
pango_language_set_to_string (PangoLanguageSet *set)
{
  PangoLanguage **languages;
  GString *s;

  s = g_string_new ("");

  languages = pango_language_set_get_languages (set);
  for (int i = 0; languages[i]; i++)
    {
      if (s->len > 0)
        g_string_append (s, "|");
      g_string_append (s, pango_language_to_string (languages[i]));
    }

  return g_string_free (s, FALSE);
}
