#pragma once

#include <glib-object.h>
#include <pango/pango-types.h>
#include <pango/pango-item.h>
#include <pango/pango-glyph.h>
#include <pango/pango-layout.h>


PANGO_AVAILABLE_IN_ALL
PangoItem *             pango_layout_run_get_item     (PangoLayoutRun   *run);

PANGO_AVAILABLE_IN_ALL
PangoGlyphString *      pango_layout_run_get_glyphs   (PangoLayoutRun   *run);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_run_get_extents  (PangoLayoutRun   *run,
                                                       PangoLeadingTrim  trim,
                                                       PangoRectangle   *ink_rect,
                                                       PangoRectangle   *logical_rect);
