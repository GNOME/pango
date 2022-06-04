#pragma once

#include "pango-attributes.h"

G_BEGIN_DECLS

gboolean pango_attribute_affects_itemization    (PangoAttribute *attr,
                                                 gpointer        data);
gboolean pango_attribute_affects_break_or_shape (PangoAttribute *attr,
                                                 gpointer        data);


G_END_DECLS
