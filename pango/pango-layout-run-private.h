#pragma once

#include "config.h"

#include "pango-layout-run.h"
#include "pango-glyph-item.h"
#include "pango-item-private.h"


static inline PangoGlyphItem *
pango_layout_run_get_glyph_item (PangoLayoutRun *run)
{
  return (PangoGlyphItem *)run;
}
