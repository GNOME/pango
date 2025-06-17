/* Pango
 * pangocairo-coretextfont.c
 *
 * Copyright (C) 2000-2005 Red Hat Software
 * Copyright (C) 2005-2007 Imendio AB
 * Copyright (C) 2010  Kristian Rietveld  <kris@gtk.org>
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

#include "config.h"

#include <Carbon/Carbon.h>

#include "pango-impl-utils.h"
#include "pangocoretext-private.h"
#include "pangocairo.h"
#include "pangocairo-private.h"
#include "pangocairo-coretext.h"
#include "pangocairo-coretextfont.h"

struct _PangoCairoCoreTextFont
{
  PangoCoreTextFont font;
  PangoCairoFontPrivate cf_priv;
  int abs_size;
};

struct _PangoCairoCoreTextFontClass
{
  PangoCoreTextFontClass parent_class;
};



static cairo_font_face_t *pango_cairo_core_text_font_create_font_face           (PangoCairoFont *font);
static PangoFontMetrics  *pango_cairo_core_text_font_create_base_metrics_for_context (PangoCairoFont *font,
                                                                                      PangoContext    *context);

static void
cairo_font_iface_init (PangoCairoFontIface *iface)
{
  iface->create_font_face = pango_cairo_core_text_font_create_font_face;
  iface->create_base_metrics_for_context = pango_cairo_core_text_font_create_base_metrics_for_context;
  iface->cf_priv_offset = G_STRUCT_OFFSET (PangoCairoCoreTextFont, cf_priv);
}

G_DEFINE_TYPE_WITH_CODE (PangoCairoCoreTextFont, pango_cairo_core_text_font, PANGO_TYPE_CORE_TEXT_FONT,
                         G_IMPLEMENT_INTERFACE (PANGO_TYPE_CAIRO_FONT, cairo_font_iface_init))

/* we want get_glyph_extents extremely fast, so we use a small wrapper here
 * to avoid having to lookup the interface data like we do for get_metrics
 * in _pango_cairo_font_get_metrics(). */
static void
pango_cairo_core_text_font_get_glyph_extents (PangoFont      *font,
                                              PangoGlyph      glyph,
                                              PangoRectangle *ink_rect,
                                              PangoRectangle *logical_rect)
{
  PangoCairoCoreTextFont *cafont = (PangoCairoCoreTextFont *) (font);

  _pango_cairo_font_private_get_glyph_extents (&cafont->cf_priv,
					       glyph,
					       ink_rect,
					       logical_rect);
}

static cairo_font_face_t *
pango_cairo_core_text_font_create_font_face (PangoCairoFont *font)
{
  PangoCoreTextFont *ctfont = (PangoCoreTextFont *) (font);
  CTFontRef font_id;
  CGFontRef cgfont;
  cairo_font_face_t *cairo_face;

  font_id = pango_core_text_font_get_ctfont (ctfont);
  cgfont = CTFontCopyGraphicsFont (font_id, NULL);

  cairo_face = cairo_quartz_font_face_create_for_cgfont (cgfont);

  CFRelease (cgfont);

  return cairo_face;
}

static PangoFontMetrics *
pango_cairo_core_text_font_create_base_metrics_for_context (PangoCairoFont *font,
                                                            PangoContext   *context)
{
  PangoCoreTextFont *cfont = (PangoCoreTextFont *) font;
  PangoFontMetrics *metrics;
  CTFontRef ctfont;

  metrics = pango_font_metrics_new ();

  ctfont = pango_core_text_font_get_ctfont (cfont);

  metrics->ascent = CTFontGetAscent (ctfont) * PANGO_SCALE;
  metrics->descent = CTFontGetDescent (ctfont) * PANGO_SCALE;
  metrics->height = (CTFontGetAscent (ctfont) + CTFontGetDescent (ctfont) + CTFontGetLeading (ctfont)) * PANGO_SCALE;

  metrics->underline_position = CTFontGetUnderlinePosition (ctfont) * PANGO_SCALE;
  metrics->underline_thickness = CTFontGetUnderlineThickness (ctfont) * PANGO_SCALE;

  metrics->strikethrough_position = metrics->ascent / 3;
  metrics->strikethrough_thickness = CTFontGetUnderlineThickness (ctfont) * PANGO_SCALE;

  return metrics;
}

static PangoFontDescription *
pango_cairo_core_text_font_describe_absolute (PangoFont *font)
{
  PangoCairoCoreTextFont *cafont = (PangoCairoCoreTextFont *)font;
  PangoFontDescription *desc = pango_font_describe (font);

  pango_font_description_set_absolute_size (desc, cafont->abs_size);

  return desc;
}

static void
pango_cairo_core_text_font_finalize (GObject *object)
{
  PangoCairoCoreTextFont *cafont = (PangoCairoCoreTextFont *) object;

  _pango_cairo_font_private_finalize (&cafont->cf_priv);

  G_OBJECT_CLASS (pango_cairo_core_text_font_parent_class)->finalize (object);
}

static void
pango_cairo_core_text_font_class_init (PangoCairoCoreTextFontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);

  object_class->finalize = pango_cairo_core_text_font_finalize;
  /* font_class->describe defined by parent class PangoCoreTextFont. */
  font_class->get_glyph_extents = pango_cairo_core_text_font_get_glyph_extents;
  font_class->get_metrics = _pango_cairo_font_get_metrics;
  font_class->describe_absolute = pango_cairo_core_text_font_describe_absolute;
}

static void
pango_cairo_core_text_font_init (PangoCairoCoreTextFont *cafont G_GNUC_UNUSED)
{
}

PangoCoreTextFont *
_pango_cairo_core_text_font_new (PangoCairoCoreTextFontMap *cafontmap,
                                 PangoCoreTextFontKey      *key)
{
  gboolean synthesize_italic = FALSE;
  PangoCairoCoreTextFont *cafont;
  PangoCoreTextFont *cfont;
  CTFontRef font_ref;
  CTFontDescriptorRef ctdescriptor;
  CGFontRef font_id;
  double size;
  cairo_matrix_t font_matrix;
  cairo_font_options_t *options;
  const char *variations;

  size = pango_units_to_double (pango_core_text_font_key_get_size (key));

  size /= pango_matrix_get_font_scale_factor (pango_core_text_font_key_get_matrix (key));

  ctdescriptor = pango_core_text_font_key_get_ctfontdescriptor (key);
  font_ref = CTFontCreateWithFontDescriptor (ctdescriptor, size, NULL);

  if (pango_core_text_font_key_get_synthetic_italic (key))
    synthesize_italic = TRUE;

  font_id = CTFontCopyGraphicsFont (font_ref, NULL);
  if (!font_id)
    return NULL;

  cafont = g_object_new (PANGO_TYPE_CAIRO_CORE_TEXT_FONT, NULL);
  cfont = PANGO_CORE_TEXT_FONT (cafont);

  cafont->abs_size = pango_core_text_font_key_get_size (key);

  _pango_core_text_font_set_ctfont (cfont, font_ref);

  if (synthesize_italic)
    cairo_matrix_init (&font_matrix,
                       1, 0,
                       -0.25, 1,
                       0, 0);
  else
    cairo_matrix_init_identity (&font_matrix);
 
  cairo_matrix_scale (&font_matrix, size, size);

  options = pango_core_text_font_key_get_context_key (key);
  if (options)
    options = cairo_font_options_copy (options);
  else
    options = cairo_font_options_create ();

  variations = pango_core_text_font_key_get_variations (key);
  if (variations)
    cairo_font_options_set_variations (options, variations);

  _pango_cairo_font_private_initialize (&cafont->cf_priv,
					(PangoCairoFont *) cafont,
                                        pango_core_text_font_key_get_gravity (key),
                                        options,
                                        pango_core_text_font_key_get_matrix (key),
					&font_matrix);

  cairo_font_options_destroy (options);

  return cfont;
}
