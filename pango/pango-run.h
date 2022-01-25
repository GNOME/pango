#pragma once

#include <glib-object.h>
#include <pango/pango-types.h>
#include <pango/pango-item.h>
#include <pango/pango-glyph.h>

PANGO_AVAILABLE_IN_ALL
PangoItem *             pango_run_get_item     (PangoRun         *run);

PANGO_AVAILABLE_IN_ALL
PangoGlyphString *      pango_run_get_glyphs   (PangoRun         *run);

PANGO_AVAILABLE_IN_ALL
void                    pango_run_get_extents  (PangoRun         *run,
                                                PangoLeadingTrim  trim,
                                                PangoRectangle   *ink_rect,
                                                PangoRectangle   *logical_rect);
