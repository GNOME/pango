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

/**
 * SECTION:coretext-fonts
 * @short_description:Font handling with CoreText fonts
 * @title:CoreText Fonts
 *
 * The macros and functions in this section are used to access fonts natively on
 * OS X using the CoreText text rendering subsystem.
 */
#include "config.h"

#include "pangocoretext.h"
#include "pangocoretext-private.h"

struct _PangoCoreTextFontPrivate
{
  PangoCoreTextFace *face;
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
  PangoCoreTextFontMap* fontmap = g_weak_ref_get ((GWeakRef *)&priv->fontmap);
  if (fontmap)
    {
      g_weak_ref_clear ((GWeakRef *)&priv->fontmap);
      g_object_unref (fontmap);
    }

  if (priv->coverage)
    pango_coverage_unref (priv->coverage);

  G_OBJECT_CLASS (pango_core_text_font_parent_class)->finalize (object);
}

static PangoFontDescription *
pango_core_text_font_describe (PangoFont *font)
{
  PangoCoreTextFont *ctfont = (PangoCoreTextFont *)font;
  PangoCoreTextFontPrivate *priv = ctfont->priv;
  CTFontDescriptorRef ctfontdesc;

  ctfontdesc = pango_core_text_font_key_get_ctfontdescriptor (priv->key);

  return _pango_core_text_font_description_from_ct_font_descriptor (ctfontdesc);
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

  coverage = pango_coverage_new ();

  charset = CTFontDescriptorCopyAttribute (desc, kCTFontCharacterSetAttribute);
  if (!charset)
    /* Return an empty coverage */
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

  return pango_coverage_ref (priv->coverage);
}

/* Wrap shaper in PangoEngineShape to pass it through old API,
 * from times when there were modules and engines. */
typedef PangoEngineShape      PangoCoreTextShapeEngine;
typedef PangoEngineShapeClass PangoCoreTextShapeEngineClass;
static GType pango_core_text_shape_engine_get_type (void) G_GNUC_CONST;
G_DEFINE_TYPE (PangoCoreTextShapeEngine, pango_core_text_shape_engine, PANGO_TYPE_ENGINE_SHAPE);
static void
_pango_core_text_shape_engine_shape (PangoEngineShape    *engine G_GNUC_UNUSED,
				 PangoFont           *font,
				 const char          *item_text,
				 unsigned int         item_length,
				 const PangoAnalysis *analysis,
				 PangoGlyphString    *glyphs,
				 const char          *paragraph_text,
				 unsigned int         paragraph_length)
{
  _pango_core_text_shape (font, item_text, item_length, analysis, glyphs,
		      paragraph_text, paragraph_length);
}
static void
pango_core_text_shape_engine_class_init (PangoEngineShapeClass *class)
{
  class->script_shape = _pango_core_text_shape_engine_shape;
}
static void
pango_core_text_shape_engine_init (PangoEngineShape *object)
{
}

static PangoEngineShape *
pango_core_text_font_find_shaper (PangoFont     *font,
                                  PangoLanguage *language G_GNUC_UNUSED,
                                  guint32        ch)
{
  static PangoEngineShape *shaper;
  if (g_once_init_enter (&shaper))
    g_once_init_leave (&shaper, g_object_new (pango_core_text_shape_engine_get_type(), NULL));
  return shaper;
}

static PangoFontMap *
pango_core_text_font_get_font_map (PangoFont *font)
{
  PangoCoreTextFont *ctfont = (PangoCoreTextFont *)font;
  /* FIXME: Not thread safe! */
  return ctfont->priv->fontmap;
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
  font_class->find_shaper = pango_core_text_font_find_shaper;
  font_class->get_font_map = pango_core_text_font_get_font_map;
}

void
_pango_core_text_font_set_font_map (PangoCoreTextFont    *font,
                                    PangoCoreTextFontMap *fontmap)
{
  PangoCoreTextFontPrivate *priv = font->priv;

  g_return_if_fail (priv->fontmap == NULL);
  g_weak_ref_set((GWeakRef *) &priv->fontmap, fontmap);
}

void
_pango_core_text_font_set_face (PangoCoreTextFont *ctfont,
                                PangoCoreTextFace *ctface)
{
  PangoCoreTextFontPrivate *priv = ctfont->priv;

  priv->face = ctface;
}

PangoCoreTextFace *
_pango_core_text_font_get_face (PangoCoreTextFont *font)
{
  PangoCoreTextFontPrivate *priv = font->priv;

  return priv->face;
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
      pango_coverage_unref (priv->coverage);
      priv->coverage = NULL;
    }
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
 * @font: A #PangoCoreTextFont
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
