#pragma once

#include "pango-fontset.h"

G_BEGIN_DECLS

#define PANGO_FONTSET_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FONTSET, PangoFontsetClass))
#define PANGO_IS_FONTSET_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_FONTSET))
#define PANGO_FONTSET_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FONTSET, PangoFontsetClass))


typedef struct _PangoFontsetClass   PangoFontsetClass;

struct _PangoFontset
{
  GObject parent_instance;
};

struct _PangoFontsetClass
{
  GObjectClass parent_class;

  PangoFont *       (*get_font)     (PangoFontset     *fontset,
                                     guint             wc);

  PangoFontMetrics *(*get_metrics)  (PangoFontset     *fontset);
  PangoLanguage *   (*get_language) (PangoFontset     *fontset);
  void              (*foreach)      (PangoFontset           *fontset,
                                     PangoFontsetForeachFunc func,
                                     gpointer                data);
};

G_END_DECLS
