#pragma once

#include "config.h"

#include "pango-layout-run.h"
#include "pango-glyph-item.h"
#include "pango-item-private.h"

struct _PangoLayoutRun
{
  PangoGlyphItem glyph_item;
};

static inline PangoGlyphItem *
pango_layout_run_get_glyph_item (PangoLayoutRun *run)
{
  return &run->glyph_item;
}
