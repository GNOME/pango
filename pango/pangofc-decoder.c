/* Pango
 * pangofc-decoder.c: Custom font encoder/decoders
 *
 * Copyright (C) 2004 Red Hat Software
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

#include "pangofc-decoder.h"

G_DEFINE_ABSTRACT_TYPE (PangoFcDecoder, pango_fc_decoder, G_TYPE_OBJECT)

static void
pango_fc_decoder_init (PangoFcDecoder *decoder);

static void
pango_fc_decoder_class_init (PangoFcDecoderClass *klass);

void
pango_fc_decoder_init (PangoFcDecoder *decoder)
{
}

void
pango_fc_decoder_class_init (PangoFcDecoderClass *klass)
{
}

/**
 * pango_fc_decoder_get_charset:
 * @decoder: A #PangoFcDecoder to use when querying the font for a
 * supported #FcCharSet.
 * @fcfont: The #PangoFcFont to query.
 *
 * Generates an #FcCharSet of supported characters for the fcfont
 * given.  The returned #FcCharSet should be a reference to an
 * internal value stored by the #PangoFcDecoder and will not be freed
 * by Pango.
 *
 * Since: 1.6
 *
 */

FcCharSet *
pango_fc_decoder_get_charset (PangoFcDecoder *decoder,
			      PangoFcFont    *fcfont)
{
  g_return_val_if_fail (PANGO_IS_FC_DECODER (decoder), NULL);

  return PANGO_FC_DECODER_GET_CLASS (decoder)->get_charset (fcfont);
}

/**
 * pango_fc_decoder_get_glyph:
 * @decoder: A #PangoFcDecoder to use when querying the font.
 * @fcfont: The #PangoFcFont to query.
 * @wc: The unicode code point that you would like to see converted to
 * a single #PangoGlyph.
 *
 * Generates a #PangoGlyph for the given unicode point with the custom
 * decoder.
 *
 * Since: 1.6
 *
 */

PangoGlyph
pango_fc_decoder_get_glyph   (PangoFcDecoder *decoder,
			      PangoFcFont    *fcfont,
			      guint32         wc)
{
  g_return_val_if_fail (PANGO_IS_FC_DECODER (decoder), 0);

  return PANGO_FC_DECODER_GET_CLASS (decoder)->get_glyph (fcfont, wc);
}
