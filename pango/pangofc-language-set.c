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

#include "pangofc-language-set-private.h"


G_DEFINE_TYPE (PangoFcLanguageSet, pango_fc_language_set, PANGO_TYPE_LANGUAGE_SET)

static void
pango_fc_language_set_init (PangoFcLanguageSet *self)
{
}

static void
pango_fc_language_set_finalize (GObject *object)
{
  PangoFcLanguageSet *self = PANGO_FC_LANGUAGE_SET (object);

  FcLangSetDestroy (self->langs);

  G_OBJECT_CLASS (pango_fc_language_set_parent_class)->finalize (object);
}

static gboolean
pango_fc_language_set_matches_language (PangoLanguageSet *set,
                                        PangoLanguage    *language)
{
  PangoFcLanguageSet *self = PANGO_FC_LANGUAGE_SET (set);
  const char *s;

  if (language == pango_language_from_string ("c"))
    return TRUE;

  if (FcLangSetHasLang (self->langs, (FcChar8 *) pango_language_to_string (language)) != FcLangDifferentLang)
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

      if (FcLangSetHasLang (self->langs, (FcChar8 *) buf) != FcLangDifferentLang)
        return TRUE;
    }

  return FALSE;
}

static PangoLanguage **
pango_fc_language_set_get_languages (PangoLanguageSet *set)
{
  PangoFcLanguageSet *self = PANGO_FC_LANGUAGE_SET (set);
  FcStrSet *strset;
  FcStrList *list;
  FcChar8 *s;
  GPtrArray *langs;

  langs = g_ptr_array_new ();

  strset = FcLangSetGetLangs (self->langs);
  list = FcStrListCreate (strset);

  FcStrListFirst (list);
  while ((s = FcStrListNext (list)))
    g_ptr_array_add (langs, pango_language_from_string ((const char *)s));

  FcStrListDone (list);
  FcStrSetDestroy (strset);

  g_ptr_array_add (langs, NULL);

  return (PangoLanguage **) g_ptr_array_free (langs, FALSE);
}

static void
pango_fc_language_set_class_init (PangoFcLanguageSetClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoLanguageSetClass *language_set_class = PANGO_LANGUAGE_SET_CLASS (class);

  object_class->finalize = pango_fc_language_set_finalize;

  language_set_class->matches_language = pango_fc_language_set_matches_language;
  language_set_class->get_languages = pango_fc_language_set_get_languages;
}

PangoFcLanguageSet *
pango_fc_language_set_new (FcLangSet *langs)
{
  PangoFcLanguageSet *self;

  self = g_object_new (PANGO_TYPE_FC_LANGUAGE_SET, NULL);

  self->langs = FcLangSetCopy (langs);

  return self;
}

guint
pango_fc_language_set_hash (const PangoFcLanguageSet *self)
{
  return (guint) FcLangSetHash (self->langs);
}

gboolean
pango_fc_language_set_equal (const PangoFcLanguageSet *a,
                             const PangoFcLanguageSet *b)
{
  return FcLangSetEqual (a->langs, b->langs);
}

/* vim:set foldmethod=marker expandtab: */
