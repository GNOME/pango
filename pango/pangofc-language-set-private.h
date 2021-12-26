#pragma once

#include "pango-language-set-private.h"
#include <fontconfig/fontconfig.h>

G_BEGIN_DECLS

#define PANGO_TYPE_FC_LANGUAGE_SET (pango_fc_language_set_get_type ())

G_DECLARE_FINAL_TYPE (PangoFcLanguageSet, pango_fc_language_set, PANGO, FC_LANGUAGE_SET, PangoLanguageSet)

struct _PangoFcLanguageSet
{
  PangoLanguageSet parent_instance;

  FcLangSet *langs;
};

struct _PangoFcLanguageSetClass
{
  PangoLanguageSetClass parent_class;
};

PangoFcLanguageSet *    pango_fc_language_set_new       (FcLangSet                      *langs);

guint                   pango_fc_language_set_hash      (const PangoFcLanguageSet       *self);
gboolean                pango_fc_language_set_equal     (const PangoFcLanguageSet       *a,
                                                         const PangoFcLanguageSet       *b);

G_END_DECLS
