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

G_DEFINE_TYPE (PangoCoreTextFont, pango_core_text_font, PANGO_TYPE_FONT);

struct _PangoCoreTextFontPrivate
{
  PangoCoreTextFace *face;
  gpointer context_key;

  CTFontRef font_ref;
  PangoCoreTextFontKey *key;

  PangoCoverage *coverage;

  PangoFontMap *fontmap;
};

static void
pango_core_text_font_finalize (GObject *object)
{
  PangoCoreTextFont *ctfont = (PangoCoreTextFont *)object;
  PangoCoreTextFontPrivate *priv = ctfont->priv;

  g_assert (priv->fontmap != NULL);
  g_object_remove_weak_pointer (G_OBJECT (priv->fontmap), (gpointer *) (gpointer) &priv->fontmap);
  priv->fontmap = NULL;

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
  const UInt8 *ptr;
  PangoCoverage *coverage;

  coverage = pango_coverage_new ();

  charset = CTFontDescriptorCopyAttribute (desc, kCTFontCharacterSetAttribute);
  if (!charset)
    /* Return an empty coverage */
    return coverage;

  bitmap = CFCharacterSetCreateBitmapRepresentation (kCFAllocatorDefault,
                                                     charset);

  /* We only handle the BMP plane */
  length = MIN (CFDataGetLength (bitmap), 8192);
  ptr = CFDataGetBytePtr (bitmap);

  /* FIXME: can and should this be done more efficiently? */
  for (i = 0; i < length; i++)
    {
      int j;

      for (j = 0; j < 8; j++)
        pango_coverage_set (coverage, i * 8 + j,
                            ((ptr[i] & (1 << j)) == (1 << j)) ?
                            PANGO_COVERAGE_EXACT : PANGO_COVERAGE_NONE);
    }

  CFRelease (bitmap);
  CFRelease (charset);

  return coverage;
}

static PangoCoverage *
pango_core_text_font_get_coverage (PangoFont     *font,
                                   PangoLanguage *language)
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

static PangoEngineShape *
pango_core_text_font_find_shaper (PangoFont     *font,
                                  PangoLanguage *language,
                                  guint32        ch)
{
  /* FIXME: Implement */
  return NULL;
}

static PangoFontMap *
pango_core_text_font_get_font_map (PangoFont *font)
{
  PangoCoreTextFont *ctfont = (PangoCoreTextFont *)font;

  return ctfont->priv->fontmap;
}

static void
pango_core_text_font_init (PangoCoreTextFont *ctfont)
{
  ctfont->priv = G_TYPE_INSTANCE_GET_PRIVATE (ctfont,
                                              PANGO_TYPE_CORE_TEXT_FONT,
                                              PangoCoreTextFontPrivate);
}

static void
pango_core_text_font_class_init (PangoCoreTextFontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);

  object_class->finalize = pango_core_text_font_finalize;

  font_class->describe = pango_core_text_font_describe;
  font_class->get_coverage = pango_core_text_font_get_coverage;
  font_class->find_shaper = pango_core_text_font_find_shaper;
  font_class->get_font_map = pango_core_text_font_get_font_map;

  g_type_class_add_private (object_class, sizeof (PangoCoreTextFontPrivate));
}

void
_pango_core_text_font_set_font_map (PangoCoreTextFont    *font,
                                    PangoCoreTextFontMap *fontmap)
{
  PangoCoreTextFontPrivate *priv = font->priv;

  g_assert (priv->fontmap == NULL);
  priv->fontmap = (PangoFontMap *) fontmap;
  g_object_add_weak_pointer (G_OBJECT (priv->fontmap), (gpointer *) (gpointer) &priv->fontmap);
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
