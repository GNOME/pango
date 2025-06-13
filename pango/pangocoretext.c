/* Pango
 * pangocoretext.c
 *
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

#include "pangocoretext.h"
#include "pangocoretext-private.h"
#include "pango-impl-utils.h"
#include <hb-coretext.h>
#include <hb-ot.h>

struct _PangoCoreTextFontPrivate
{
  gpointer context_key;

  CTFontRef font_ref;
  PangoCoreTextFontKey *key;

  PangoCoverage *coverage;

  PangoFontMap *fontmap;
};

G_DEFINE_TYPE_WITH_PRIVATE (PangoCoreTextFont, pango_core_text_font, PANGO_TYPE_FONT)

static void
pango_core_text_font_finalize (GObject *object)
{
  PangoCoreTextFont *ctfont = (PangoCoreTextFont *)object;
  PangoCoreTextFontPrivate *priv = ctfont->priv;

  if (priv->fontmap)
    g_object_remove_weak_pointer (G_OBJECT (priv->fontmap), (gpointer *) &priv->fontmap);

  if (priv->coverage)
    g_object_unref (priv->coverage);

  G_OBJECT_CLASS (pango_core_text_font_parent_class)->finalize (object);
}

static PangoFontDescription *
pango_core_text_font_describe (PangoFont *font)
{
  PangoCoreTextFont *ctfont = (PangoCoreTextFont *)font;
  PangoCoreTextFontPrivate *priv = ctfont->priv;
  CTFontDescriptorRef ctfontdesc;
  PangoFontDescription *desc;
  const char *variations;

  ctfontdesc = CTFontCopyFontDescriptor (priv->font_ref);
  desc = _pango_core_text_font_description_from_ct_font_descriptor (ctfontdesc);
  CFRelease (ctfontdesc);

  variations = pango_core_text_font_key_get_variations (priv->key);
  if (variations)
    pango_font_description_set_variations (desc, variations);

  if (pango_core_text_font_key_get_synthetic_small_caps (priv->key))
    pango_font_description_set_variant (desc, PANGO_VARIANT_SMALL_CAPS);

  return desc;
}

static PangoCoverage *
ct_font_descriptor_get_coverage (CTFontDescriptorRef desc)
{
  CFCharacterSetRef charset;
  CFIndex i, length;
  CFDataRef bitmap;
  const UInt8 *ptr, *plane_ptr;
  const UInt32 plane_size = 8192;
  PangoCoverage *coverage;
  CFStringRef font_name;

  coverage = pango_coverage_new ();

  charset = CTFontDescriptorCopyAttribute (desc, kCTFontCharacterSetAttribute);
  if (!charset)
    /* Return an empty coverage */
    return coverage;
  /* .AppleSymbols Fallback's CTFontDescriptor has a host of members
   * but the font appears to have no glyphs so return an empty
   * coverage.
   */
  font_name = (CFStringRef)CTFontDescriptorCopyAttribute(desc, kCTFontNameAttribute);
  if (CFStringCompare(font_name, CFSTR(".AppleSymbolsFB"), 0) == kCFCompareEqualTo)
    return coverage;

  bitmap = CFCharacterSetCreateBitmapRepresentation (kCFAllocatorDefault,
                                                     charset);
  ptr = CFDataGetBytePtr (bitmap);

  /* First handle the BMP plane. */
  length = MIN (CFDataGetLength (bitmap), plane_size);

  /* FIXME: can and should this be done more efficiently? */
  for (i = 0; i < length; i++)
    {
      int j;

      for (j = 0; j < 8; j++)
        if ((ptr[i] & (1 << j)) == (1 << j))
          pango_coverage_set (coverage, i * 8 + j, PANGO_COVERAGE_EXACT);
    }

  /* Next, handle the other planes. The plane number is encoded first as
   * a single byte. In the following 8192 bytes that plane's coverage bitmap
   * is stored.
   */
  plane_ptr = ptr + plane_size;
  while (plane_ptr - ptr < CFDataGetLength (bitmap))
    {
      const UInt8 plane_number = *plane_ptr;
      plane_ptr++;

      for (i = 0; i < plane_size; i++)
        {
          int j;

          for (j = 0; j < 8; j++)
            if ((plane_ptr[i] & (1 << j)) == (1 << j))
              pango_coverage_set (coverage, (plane_number * plane_size + i) * 8 + j,
                                  PANGO_COVERAGE_EXACT);
        }

      plane_ptr += plane_size;
    }

  CFRelease (bitmap);
  CFRelease (charset);

  return coverage;
}

static PangoCoverage *
pango_core_text_font_get_coverage (PangoFont     *font,
                                   PangoLanguage *language G_GNUC_UNUSED)
{
  PangoCoreTextFont *ctfont = (PangoCoreTextFont *)font;
  PangoCoreTextFontPrivate *priv = ctfont->priv;

  if (!priv->coverage)
    {
      CTFontDescriptorRef ctfontdesc;

      ctfontdesc = pango_core_text_font_key_get_ctfontdescriptor (priv->key);

      priv->coverage = ct_font_descriptor_get_coverage (ctfontdesc);
    }

  return g_object_ref (priv->coverage);
}

static PangoFontMap *
pango_core_text_font_get_font_map (PangoFont *font)
{
  PangoCoreTextFont *ctfont = (PangoCoreTextFont *)font;
  /* FIXME: Not thread safe! */
  return ctfont->priv->fontmap;
}

static hb_font_t *
pango_core_text_font_create_hb_font (PangoFont *font)
{
  PangoCoreTextFont *ctfont = (PangoCoreTextFont *)font;

  if (ctfont->priv->font_ref)
    {
      const PangoMatrix *matrix;
      hb_font_t *hb_font;
      double x_scale, y_scale;
      int size;
      const char *variations;

      matrix = pango_core_text_font_key_get_matrix (ctfont->priv->key);
      pango_matrix_get_font_scale_factors (matrix, &x_scale, &y_scale);
      size = pango_core_text_font_key_get_size (ctfont->priv->key);
      variations = pango_core_text_font_key_get_variations (ctfont->priv->key);

      hb_font = hb_coretext_font_create (ctfont->priv->font_ref);
      hb_font_set_scale (hb_font, size / x_scale, size / y_scale);

      if (variations)
        {
          hb_face_t *face;
          unsigned int n_axes;

          face = hb_font_get_face (hb_font);
          n_axes = hb_ot_var_get_axis_infos (face, 0, NULL, NULL);
          if (n_axes > 0)
            {
              hb_ot_var_axis_info_t *axes;
              float *coords;

              axes = g_newa (hb_ot_var_axis_info_t, n_axes);
              coords = g_newa (float, n_axes);

              hb_ot_var_get_axis_infos (face, 0, &n_axes, axes);
              for (unsigned int i = 0; i < n_axes; i++)
                coords[axes[i].axis_index] = axes[i].default_value;

              pango_parse_variations (variations, axes, n_axes, coords);

              hb_font_set_var_coords_design (hb_font, coords, n_axes);
            }
        }


      return hb_font;
    }

  return hb_font_get_empty ();
}

static void
pango_core_text_font_init (PangoCoreTextFont *ctfont)
{
  ctfont->priv = pango_core_text_font_get_instance_private (ctfont);
}

static void
pango_core_text_font_class_init (PangoCoreTextFontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);

  object_class->finalize = pango_core_text_font_finalize;

  font_class->describe = pango_core_text_font_describe;
  /* font_class->describe_absolute is left virtual for PangoCairoCoreTextFont. */
  font_class->get_coverage = pango_core_text_font_get_coverage;
  font_class->get_font_map = pango_core_text_font_get_font_map;
  font_class->create_hb_font = pango_core_text_font_create_hb_font;
}

void
_pango_core_text_font_set_font_map (PangoCoreTextFont    *font,
                                    PangoCoreTextFontMap *fontmap)
{
  PangoCoreTextFontPrivate *priv = font->priv;

  g_return_if_fail (priv->fontmap == NULL);

  priv->fontmap = fontmap;
  if (fontmap)
    g_object_add_weak_pointer (G_OBJECT (fontmap), (gpointer *) &priv->fontmap);
}

PangoCoreTextFace *
_pango_core_text_font_get_face (PangoCoreTextFont *font)
{
  PangoCoreTextFontPrivate *priv = font->priv;

  return pango_core_text_font_map_find_face (PANGO_CORE_TEXT_FONT_MAP (priv->fontmap), priv->key);
}

gpointer
_pango_core_text_font_get_context_key (PangoCoreTextFont *font)
{
  PangoCoreTextFontPrivate *priv = font->priv;

  return priv->context_key;
}

void
_pango_core_text_font_set_context_key (PangoCoreTextFont *font,
                                       gpointer        context_key)
{
  PangoCoreTextFontPrivate *priv = font->priv;

  priv->context_key = context_key;
}

void
_pango_core_text_font_set_font_key (PangoCoreTextFont    *font,
                                    PangoCoreTextFontKey *key)
{
  PangoCoreTextFontPrivate *priv = font->priv;

  priv->key = key;

  if (priv->coverage)
    {
      g_object_unref (priv->coverage);
      priv->coverage = NULL;
    }
}

PangoCoreTextFontKey *
_pango_core_text_font_get_font_key (PangoCoreTextFont *font)
{
  PangoCoreTextFontPrivate *priv = font->priv;

  return priv->key;
}

void
_pango_core_text_font_set_ctfont (PangoCoreTextFont *font,
                                  CTFontRef          font_ref)
{
  PangoCoreTextFontPrivate *priv = font->priv;

  priv->font_ref = font_ref;
}

/**
 * pango_core_text_font_get_ctfont:
 * @font: A `PangoCoreTextFont`
 *
 * Returns the CTFontRef of a font.
 *
 * Return value: the CTFontRef associated to @font.
 *
 * Since: 1.24
 */
CTFontRef
pango_core_text_font_get_ctfont (PangoCoreTextFont *font)
{
  PangoCoreTextFontPrivate *priv = font->priv;

  return priv->font_ref;
}
