#pragma once

#include "config.h"

#include "pango-run.h"
#include "pango-glyph-item.h"
#include "pango-item-private.h"

struct _PangoRun
{
  PangoGlyphItem glyph_item;
};

static inline PangoGlyphItem *
pango_run_get_glyph_item (PangoRun *run)
{
  return &run->glyph_item;
}
