/* Pango
 * pangatsui.c
 *
 * Copyright (C) 2005-2007 Imendio AB
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

#include "pangoatsui.h"
#include "pangoatsui-private.h"

G_DEFINE_TYPE (PangoATSUIFont, pango_atsui_font, PANGO_TYPE_FONT);

struct _PangoATSUIFontPrivate
{
  PangoATSUIFace *face;
  PangoFontDescription *desc;
  gpointer context_key;

  ATSUFontID font_id;

  PangoFontMap *fontmap;
};

static void
pango_atsui_font_finalize (GObject *object)
{
  PangoATSUIFont *afont = (PangoATSUIFont *)object;
  PangoATSUIFontPrivate *priv = afont->priv;

  pango_font_description_free (priv->desc);

  g_assert (priv->fontmap != NULL);
  g_object_remove_weak_pointer (G_OBJECT (priv->fontmap), (gpointer *) (gpointer) &priv->fontmap);
  priv->fontmap = NULL;

  G_OBJECT_CLASS (pango_atsui_font_parent_class)->finalize (object);
}

static PangoFontDescription *
pango_atsui_font_describe (PangoFont *font)
{
  PangoATSUIFont *afont = (PangoATSUIFont *)font;
  PangoATSUIFontPrivate *priv = afont->priv;

  return pango_font_description_copy (priv->desc);
}

static PangoCoverage *
pango_atsui_font_get_coverage (PangoFont     *font,
	                       PangoLanguage *language)
{
  PangoATSUIFont *afont = (PangoATSUIFont *)font;
  PangoATSUIFontPrivate *priv = afont->priv;

  return pango_coverage_ref (_pango_atsui_face_get_coverage (priv->face,
                                                             language));
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
  PangoATSUIFont *afont = (PangoATSUIFont *)font;

  return afont->priv->fontmap;
}

static void
pango_atsui_font_init (PangoATSUIFont *afont)
{
  afont->priv = G_TYPE_INSTANCE_GET_PRIVATE (afont,
                                             PANGO_TYPE_ATSUI_FONT,
                                             PangoATSUIFontPrivate);
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

  g_type_class_add_private (object_class, sizeof (PangoATSUIFontPrivate));
}

void
_pango_atsui_font_set_font_description (PangoATSUIFont             *font,
                                        const PangoFontDescription *desc)
{
  PangoATSUIFontPrivate *priv = font->priv;

  priv->desc = pango_font_description_copy (desc);
}

PangoFontDescription *
_pango_atsui_font_get_font_description (PangoATSUIFont *font)
{
  PangoATSUIFontPrivate *priv = font->priv;

  return priv->desc;
}

void
_pango_atsui_font_set_font_map (PangoATSUIFont    *font,
                                PangoATSUIFontMap *fontmap)
{
  PangoATSUIFontPrivate *priv = font->priv;

  g_assert (priv->fontmap == NULL);
  priv->fontmap = (PangoFontMap *) fontmap;
  g_object_add_weak_pointer (G_OBJECT (priv->fontmap), (gpointer *) (gpointer) &priv->fontmap);
}

void
_pango_atsui_font_set_face (PangoATSUIFont *afont, 
                            PangoATSUIFace *face)
{
  PangoATSUIFontPrivate *priv = afont->priv;

  priv->face = face;
}

PangoATSUIFace *
_pango_atsui_font_get_face (PangoATSUIFont *afont)
{
  PangoATSUIFontPrivate *priv = afont->priv;

  return priv->face;
}

gpointer
_pango_atsui_font_get_context_key (PangoATSUIFont *afont)
{
  PangoATSUIFontPrivate *priv = afont->priv;

  return priv->context_key;
}

void
_pango_atsui_font_set_context_key (PangoATSUIFont *afont,
                                   gpointer        context_key)
{
  PangoATSUIFontPrivate *priv = afont->priv;

  priv->context_key = context_key;
}

void
_pango_atsui_font_set_atsu_font_id (PangoATSUIFont *font,
                                    ATSUFontID      font_id)
{
  PangoATSUIFontPrivate *priv = font->priv;

  priv->font_id = font_id;
}

/**
 * pango_atsui_font_get_atsu_font_id:
 * @font: A #PangoATSUIFont
 *
 * Returns the ATSUFontID of a font.
 *
 * Return value: the ATSUFontID associated to @font.
 *
 * Since: 1.18
 */
ATSUFontID
pango_atsui_font_get_atsu_font_id (PangoATSUIFont *font)
{
  PangoATSUIFontPrivate *priv = font->priv;

  return priv->font_id;
}
