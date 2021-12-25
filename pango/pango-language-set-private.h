#pragma once

#include "pango-language.h"

G_BEGIN_DECLS

#define PANGO_TYPE_LANGUAGE_SET (pango_language_set_get_type ())

PANGO_AVAILABLE_IN_1_52
G_DECLARE_DERIVABLE_TYPE (PangoLanguageSet, pango_language_set, PANGO, LANGUAGE_SET, GObject)

struct _PangoLanguageSetClass
{
  GObjectClass parent_class;

  gboolean               (* matches_language)   (PangoLanguageSet       *set,
                                                 PangoLanguage          *language);

  PangoLanguage **      (* get_languages)       (PangoLanguageSet       *set);
};

gboolean                pango_language_set_matches_language     (PangoLanguageSet *set,
                                                                 PangoLanguage    *language);

PangoLanguage **        pango_language_set_get_languages        (PangoLanguageSet *set);

char *                  pango_language_set_to_string            (PangoLanguageSet *set);

G_END_DECLS
