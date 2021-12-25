#pragma once

#include "pango-language-set-private.h"

G_BEGIN_DECLS

#define PANGO_TYPE_LANGUAGE_SET_SIMPLE (pango_language_set_simple_get_type ())

PANGO_AVAILABLE_IN_1_52
G_DECLARE_FINAL_TYPE (PangoLanguageSetSimple, pango_language_set_simple, PANGO, LANGUAGE_SET_SIMPLE, PangoLanguageSet)

PANGO_AVAILABLE_IN_1_52
PangoLanguageSetSimple *pango_language_set_simple_new           (void);

PANGO_AVAILABLE_IN_1_52
void                    pango_language_set_simple_add_language  (PangoLanguageSetSimple *self,
                                                                 PangoLanguage          *language);

G_END_DECLS
