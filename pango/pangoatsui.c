/* Pango
 * pangatsui.c
 *
 * Copyright (C) 2005 Imendio AB
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include "pangoatsui-private.h"

G_DEFINE_TYPE (PangoATSUIFont, pango_atsui_font, PANGO_TYPE_FONT);

static void
pango_atsui_font_finalize (GObject *object)
{
  PangoATSUIFont *font = (PangoATSUIFont *)object;

  g_free (font->postscript_name);
  pango_font_description_free (font->desc);

  g_object_unref (font->fontmap);

  G_OBJECT_CLASS (pango_atsui_font_parent_class)->finalize (object);
}

static PangoFontDescription *
pango_atsui_font_describe (PangoFont *font)
{
  PangoATSUIFont *atsuifont = PANGO_ATSUI_FONT (font);

  return pango_font_description_copy (atsuifont->desc);
}

static PangoCoverage *
pango_atsui_font_get_coverage (PangoFont     *font,
 	                       PangoLanguage *language)
{
  PangoCoverage *coverage;
  int i;

  /* FIXME: Currently we say that all fonts have the
   * 255 first glyphs. This is not true.
   */
  coverage = pango_coverage_new ();

  for (i = 0; i < 256; i++)
    pango_coverage_set (coverage, i, PANGO_COVERAGE_EXACT);

  return coverage;
}

static PangoEngineShape *
pango_atsui_font_find_shaper (PangoFont     *font,
			      PangoLanguage *language,
			      guint32        ch)
{
  /* FIXME: Implement */
  return NULL;
}

static PangoFontMap *
pango_atsui_font_get_font_map (PangoFont *font)
{
  PangoATSUIFont *atsuifont = (PangoATSUIFont *)font;

  return atsuifont->fontmap;
}

static void
pango_atsui_font_init (PangoATSUIFont *font)
{
}

static void
pango_atsui_font_class_init (PangoATSUIFontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);

  object_class->finalize = pango_atsui_font_finalize; 

  font_class->describe = pango_atsui_font_describe;
  font_class->get_coverage = pango_atsui_font_get_coverage;
  font_class->find_shaper = pango_atsui_font_find_shaper;
  font_class->get_font_map = pango_atsui_font_get_font_map;
}




