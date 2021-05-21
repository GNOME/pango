/* Pango
 * pangoxft-font.c: Routines for handling X fonts
 *
 * Copyright (C) 2000 Red Hat Software
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

#include <stdlib.h>

#include "pangofc-fontmap.h"
#include "pangoxft-private.h"
#include "pangofc-private.h"

#define PANGO_XFT_FONT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_XFT_FONT, PangoXftFontClass))
#define PANGO_XFT_IS_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_XFT_FONT))
#define PANGO_XFT_FONT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_XFT_FONT, PangoXftFontClass))

typedef struct _PangoXftFontClass    PangoXftFontClass;

struct _PangoXftFontClass
{
  PangoFcFontClass  parent_class;
};

static void pango_xft_font_finalize (GObject *object);

static void                  pango_xft_font_get_glyph_extents (PangoFont        *font,
							       PangoGlyph        glyph,
							       PangoRectangle   *ink_rect,
							       PangoRectangle   *logical_rect);

static FT_Face    pango_xft_font_real_lock_face         (PangoFcFont      *font);
static void       pango_xft_font_real_unlock_face       (PangoFcFont      *font);
static gboolean   pango_xft_font_real_has_char          (PangoFcFont      *font,
							 gunichar          wc);
static guint      pango_xft_font_real_get_glyph         (PangoFcFont      *font,
							 gunichar          wc);
static void       pango_xft_font_real_shutdown          (PangoFcFont      *font);

static XftFont *xft_font_get_font (PangoFont *font);

G_DEFINE_TYPE (PangoXftFont, pango_xft_font, PANGO_TYPE_FC_FONT)

static void
pango_xft_font_class_init (PangoXftFontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);
  PangoFcFontClass *fc_font_class = PANGO_FC_FONT_CLASS (class);

  object_class->finalize = pango_xft_font_finalize;

  font_class->get_glyph_extents = pango_xft_font_get_glyph_extents;

  fc_font_class->lock_face = pango_xft_font_real_lock_face;
  fc_font_class->unlock_face = pango_xft_font_real_unlock_face;
  fc_font_class->has_char = pango_xft_font_real_has_char;
  fc_font_class->get_glyph = pango_xft_font_real_get_glyph;
  fc_font_class->shutdown = pango_xft_font_real_shutdown;
}

static void
pango_xft_font_init (PangoXftFont *xftfont G_GNUC_UNUSED)
{
}

PangoXftFont *
_pango_xft_font_new (PangoXftFontMap *xftfontmap,
		     FcPattern	     *pattern)
{
  PangoFontMap *fontmap = PANGO_FONT_MAP (xftfontmap);
  PangoXftFont *xfont;

  g_return_val_if_fail (fontmap != NULL, NULL);
  g_return_val_if_fail (pattern != NULL, NULL);

  xfont = (PangoXftFont *)g_object_new (PANGO_TYPE_XFT_FONT,
					"pattern", pattern,
					"fontmap", fontmap,
					NULL);

  /* Hack to force hinting of vertical metrics; hinting off for
   * a Xft font just means to not hint outlines, but we still
   * want integer line spacing, underline positions, etc
   */
  PANGO_FC_FONT (xfont)->is_hinted = TRUE;

  xfont->xft_font = NULL;

  return xfont;
}

/**
 * _pango_xft_font_get_mini_font:
 * @xfont: a `PangoXftFont`
 *
 * Gets the font used for drawing the digits in the
 * missing-character hex squares
 *
 * Return value: the `PangoFont` used for the digits; this
 *  value is associated with the main font and will be freed
 *  along with the main font.
 */
PangoFont *
_pango_xft_font_get_mini_font (PangoXftFont *xfont)
{
  PangoFcFont *fcfont = (PangoFcFont *)xfont;

  if (!fcfont || !fcfont->fontmap)
    return NULL;

  if (!xfont->mini_font)
    {
      Display *display;
      int screen;
      PangoFontDescription *desc = pango_font_description_new ();
      PangoContext *context;
      int i;
      int width = 0, height = 0;
      XGlyphInfo extents;
      XftFont *mini_xft;
      int new_size;

      _pango_xft_font_map_get_info (fcfont->fontmap, &display, &screen);

      context = pango_font_map_create_context (pango_xft_get_font_map (display, screen));
      pango_context_set_language (context, pango_language_from_string ("en"));

      pango_font_description_set_family_static (desc, "monospace");

      new_size = pango_font_description_get_size (fcfont->description) / 2;

      if (pango_font_description_get_size_is_absolute (fcfont->description))
	pango_font_description_set_absolute_size (desc, new_size);
      else
	pango_font_description_set_size (desc, new_size);

      xfont->mini_font = pango_font_map_load_font (fcfont->fontmap, context, desc);
      pango_font_description_free (desc);
      g_object_unref (context);

      if (!xfont->mini_font)
	return NULL;

      mini_xft = xft_font_get_font (xfont->mini_font);

      for (i = 0 ; i < 16 ; i++)
	{
	  char c = i < 10 ? '0' + i : 'A' + i - 10;
	  XftTextExtents8 (display, mini_xft, (FcChar8 *) &c, 1, &extents);
	  width = MAX (width, extents.width);
	  height = MAX (height, extents.height);
	}

      xfont->mini_width = PANGO_SCALE * width;
      xfont->mini_height = PANGO_SCALE * height;
      xfont->mini_pad = PANGO_SCALE * MIN (height / 2, MAX ((int)(2.2 * height + 27) / 28, 1));
    }

  return xfont->mini_font;
}

static void
pango_xft_font_finalize (GObject *object)
{
  PangoXftFont *xfont = (PangoXftFont *)object;
  PangoFcFont *fcfont = (PangoFcFont *)object;

  if (xfont->mini_font)
    g_object_unref (xfont->mini_font);

  if (xfont->xft_font)
    {
      Display *display;

      _pango_xft_font_map_get_info (fcfont->fontmap, &display, NULL);
      XftFontClose (display, xfont->xft_font);
    }

  if (xfont->glyph_info)
    g_hash_table_destroy (xfont->glyph_info);

  G_OBJECT_CLASS (pango_xft_font_parent_class)->finalize (object);
}

static void
get_glyph_extents_missing (PangoXftFont    *xfont,
			   PangoGlyph       glyph,
			   PangoRectangle  *ink_rect,
			   PangoRectangle  *logical_rect)

{
  PangoFont *font = PANGO_FONT (xfont);
  XftFont *xft_font = xft_font_get_font (font);
  gunichar ch;
  gint cols;
  
  ch = glyph & ~PANGO_GLYPH_UNKNOWN_FLAG;

  if (G_UNLIKELY (glyph == PANGO_GLYPH_INVALID_INPUT || ch > 0x10FFFF))
    cols = 1;
  else
    cols = ch > 0xffff ? 3 : 2;

  _pango_xft_font_get_mini_font (xfont);

  if (ink_rect)
    {
      ink_rect->x = 0;
      ink_rect->y = - PANGO_SCALE * xft_font->ascent + PANGO_SCALE * (((xft_font->ascent + xft_font->descent) - (xfont->mini_height * 2 + xfont->mini_pad * 5 + PANGO_SCALE / 2) / PANGO_SCALE) / 2);
      ink_rect->width = xfont->mini_width * cols + xfont->mini_pad * (2 * cols + 1);
      ink_rect->height = xfont->mini_height * 2 + xfont->mini_pad * 5;
    }

  if (logical_rect)
    {
      logical_rect->x = 0;
      logical_rect->y = - PANGO_SCALE * xft_font->ascent;
      logical_rect->width = xfont->mini_width * cols + xfont->mini_pad * (2 * cols + 2);
      logical_rect->height = (xft_font->ascent + xft_font->descent) * PANGO_SCALE;
    }
}

static void
get_glyph_extents_xft (PangoFcFont      *fcfont,
		       PangoGlyph        glyph,
		       PangoRectangle   *ink_rect,
		       PangoRectangle   *logical_rect)
{
  XftFont *xft_font = xft_font_get_font ((PangoFont *)fcfont);
  XGlyphInfo extents;
  Display *display;
  FT_UInt ft_glyph = glyph;

  _pango_xft_font_map_get_info (fcfont->fontmap, &display, NULL);

  XftGlyphExtents (display, xft_font, &ft_glyph, 1, &extents);

  if (ink_rect)
    {
      ink_rect->x = - extents.x * PANGO_SCALE; /* Xft crack-rock sign choice */
      ink_rect->y = - extents.y * PANGO_SCALE; /*             "              */
      ink_rect->width = extents.width * PANGO_SCALE;
      ink_rect->height = extents.height * PANGO_SCALE;
    }

  if (logical_rect)
    {
      logical_rect->x = 0;
      logical_rect->y = - xft_font->ascent * PANGO_SCALE;
      logical_rect->width = extents.xOff * PANGO_SCALE;
      logical_rect->height = (xft_font->ascent + xft_font->descent) * PANGO_SCALE;
    }
}

typedef struct
{
  PangoRectangle ink_rect;
  PangoRectangle logical_rect;
} Extents;

static void
extents_free (Extents *ext)
{
  g_slice_free (Extents, ext);
}

static void
get_glyph_extents_raw (PangoXftFont     *xfont,
		       PangoGlyph        glyph,
		       PangoRectangle   *ink_rect,
		       PangoRectangle   *logical_rect)
{
  Extents *extents;

  if (!xfont->glyph_info)
    xfont->glyph_info = g_hash_table_new_full (NULL, NULL,
					       NULL, (GDestroyNotify)extents_free);

  extents = g_hash_table_lookup (xfont->glyph_info,
				 GUINT_TO_POINTER (glyph));

  if (!extents)
    {
      extents = g_slice_new (Extents);

      pango_fc_font_get_raw_extents (PANGO_FC_FONT (xfont),
				     glyph,
				     &extents->ink_rect,
				     &extents->logical_rect);

      g_hash_table_insert (xfont->glyph_info,
			   GUINT_TO_POINTER (glyph),
			   extents);
    }

  if (ink_rect)
    *ink_rect = extents->ink_rect;

  if (logical_rect)
    *logical_rect = extents->logical_rect;
}

static void
pango_xft_font_get_glyph_extents (PangoFont        *font,
				  PangoGlyph        glyph,
				  PangoRectangle   *ink_rect,
				  PangoRectangle   *logical_rect)
{
  PangoXftFont *xfont = (PangoXftFont *)font;
  PangoFcFont *fcfont = PANGO_FC_FONT (font);
  gboolean empty = FALSE;

  if (G_UNLIKELY (!fcfont->fontmap))	/* Display closed */
    {
      if (ink_rect)
	ink_rect->x = ink_rect->width = ink_rect->y = ink_rect->height = 0;
      if (logical_rect)
	logical_rect->x = logical_rect->width = logical_rect->y = logical_rect->height = 0;
      return;
    }

  if (glyph == PANGO_GLYPH_EMPTY)
    {
      glyph = pango_fc_font_get_glyph (fcfont, ' ');
      empty = TRUE;
    }

  if (glyph & PANGO_GLYPH_UNKNOWN_FLAG)
    {
      get_glyph_extents_missing (xfont, glyph, ink_rect, logical_rect);
    }
  else
    {
      if (!fcfont->is_transformed)
	get_glyph_extents_xft (fcfont, glyph, ink_rect, logical_rect);
      else
	get_glyph_extents_raw (xfont, glyph, ink_rect, logical_rect);
    }

  if (empty)
    {
      if (ink_rect)
	ink_rect->x = ink_rect->y = ink_rect->height = ink_rect->width = 0;
      if (logical_rect)
	logical_rect->x = logical_rect->width = 0;
      return;
    }
}

static void
load_fallback_font (PangoXftFont *xfont)
{
  PangoFcFont *fcfont = PANGO_FC_FONT (xfont);
  Display *display;
  int screen;
  XftFont *xft_font;
  gboolean size_is_absolute;
  double size;

  _pango_xft_font_map_get_info (fcfont->fontmap, &display, &screen);

  size_is_absolute = pango_font_description_get_size_is_absolute (fcfont->description);
  size = pango_font_description_get_size (fcfont->description) / PANGO_SCALE;

  xft_font = XftFontOpen (display,  screen,
			  FC_FAMILY, FcTypeString, "sans",
			  size_is_absolute ? FC_PIXEL_SIZE : FC_SIZE, FcTypeDouble, size,
			  NULL);

  xfont->xft_font = xft_font;
}

static XftFont *
xft_font_get_font (PangoFont *font)
{
  PangoXftFont *xfont;
  PangoFcFont *fcfont;
  Display *display;
  int screen;

  xfont = (PangoXftFont *)font;
  fcfont = (PangoFcFont *)font;

  if (G_UNLIKELY (xfont->xft_font == NULL))
    {
      FcPattern *pattern = FcPatternDuplicate (fcfont->font_pattern);
      FcPatternDel (pattern, FC_SPACING);

      _pango_xft_font_map_get_info (fcfont->fontmap, &display, &screen);

      xfont->xft_font = XftFontOpenPattern (display, pattern);
      if (!xfont->xft_font)
	{
	  gchar *name = pango_font_description_to_string (fcfont->description);
	  g_warning ("Cannot open font file for font %s", name);
	  g_free (name);

	  load_fallback_font (xfont);
	}
    }

  return xfont->xft_font;
}

static FT_Face
pango_xft_font_real_lock_face (PangoFcFont *font)
{
  XftFont *xft_font = xft_font_get_font ((PangoFont *)font);

  return XftLockFace (xft_font);
}

static void
pango_xft_font_real_unlock_face (PangoFcFont *font)
{
  XftFont *xft_font = xft_font_get_font ((PangoFont *)font);

  XftUnlockFace (xft_font);
}

static gboolean
pango_xft_font_real_has_char (PangoFcFont *font,
			      gunichar     wc)
{
  XftFont *xft_font = xft_font_get_font ((PangoFont *)font);

  return XftCharExists (NULL, xft_font, wc);
}

static guint
pango_xft_font_real_get_glyph (PangoFcFont *font,
			       gunichar     wc)
{
  XftFont *xft_font = xft_font_get_font ((PangoFont *)font);

  return XftCharIndex (NULL, xft_font, wc);
}

static void
pango_xft_font_real_shutdown (PangoFcFont *fcfont)
{
  PangoXftFont *xfont = PANGO_XFT_FONT (fcfont);

  if (xfont->xft_font)
    {
      Display *display;

      _pango_xft_font_map_get_info (fcfont->fontmap, &display, NULL);
      XftFontClose (display, xfont->xft_font);
      xfont->xft_font = NULL;
    }
}

/**
 * pango_xft_font_get_font: (skip)
 * @font: (nullable): a `PangoFont`
 *
 * Returns the `XftFont` of a font.
 *
 * Return value: (nullable): the `XftFont` associated to @font
 */
XftFont *
pango_xft_font_get_font (PangoFont *font)
{
  if (G_UNLIKELY (!font))
    return NULL;

  return xft_font_get_font (font);
}

/**
 * pango_xft_font_get_display: (skip)
 * @font: (type PangoXftFont): a `PangoFont`
 *
 * Returns the X display of the `XftFont` of a font.
 *
 * Return value: (transfer none): the X display of the XftFont associated to @font.
 **/
Display *
pango_xft_font_get_display (PangoFont *font)
{
  PangoFcFont *fcfont;
  Display *display;

  g_return_val_if_fail (PANGO_XFT_IS_FONT (font), NULL);

  fcfont = PANGO_FC_FONT (font);
  _pango_xft_font_map_get_info (fcfont->fontmap, &display, NULL);

  return display;
}

/**
 * pango_xft_font_get_unknown_glyph:
 * @font: (type PangoXftFont): a `PangoFont`
 * @wc: the Unicode character for which a glyph is needed.
 *
 * Returns the index of a glyph suitable for drawing @wc as an
 * unknown character.
 *
 * Use PANGO_GET_UNKNOWN_GLYPH() instead.
 *
 * Return value: a glyph index into @font.
 **/
PangoGlyph
pango_xft_font_get_unknown_glyph (PangoFont *font,
				  gunichar   wc)
{
  g_return_val_if_fail (PANGO_XFT_IS_FONT (font), PANGO_GLYPH_EMPTY);

  return PANGO_GET_UNKNOWN_GLYPH (wc);
}

/**
 * pango_xft_font_lock_face: (skip)
 * @font: (type PangoXftFont): a `PangoFont`
 *
 * Gets the FreeType `FT_Face` associated with a font.
 *
 * This face will be kept around until you call pango_xft_font_unlock_face().
 *
 * Use pango_fc_font_lock_face() instead.
 *
 * Return value: the FreeType `FT_Face` associated with @font.
 *
 * Since: 1.2
 **/
FT_Face
pango_xft_font_lock_face (PangoFont *font)
{
  g_return_val_if_fail (PANGO_XFT_IS_FONT (font), NULL);

  return pango_fc_font_lock_face (PANGO_FC_FONT (font));
}

/**
 * pango_xft_font_unlock_face: (skip)
 * @font: (type PangoXftFont): a `PangoFont`
 *
 * Releases a font previously obtained with
 * pango_xft_font_lock_face().
 *
 * Use pango_fc_font_unlock_face() instead.
 *
 * Since: 1.2
 **/
void
pango_xft_font_unlock_face (PangoFont *font)
{
  g_return_if_fail (PANGO_XFT_IS_FONT (font));

  pango_fc_font_unlock_face (PANGO_FC_FONT (font));
}

/**
 * pango_xft_font_get_glyph:
 * @font: (type PangoXftFont): a `PangoFont` for the Xft backend
 * @wc: Unicode codepoint to look up
 *
 * Gets the glyph index for a given Unicode character
 * for @font.
 *
 * If you only want to determine whether the font has
 * the glyph, use pango_xft_font_has_char().
 *
 * Use pango_fc_font_get_glyph() instead.
 *
 * Return value: the glyph index, or 0, if the Unicode
 *  character does not exist in the font.
 *
 * Since: 1.2
 **/
guint
pango_xft_font_get_glyph (PangoFont *font,
			  gunichar   wc)
{
  g_return_val_if_fail (PANGO_XFT_IS_FONT (font), 0);

  return pango_fc_font_get_glyph (PANGO_FC_FONT (font), wc);
}

/**
 * pango_xft_font_has_char:
 * @font: (type PangoXftFont): a `PangoFont` for the Xft backend
 * @wc: Unicode codepoint to look up
 *
 * Determines whether @font has a glyph for the codepoint @wc.
 *
 * Use pango_fc_font_has_char() instead.
 *
 * Return value: %TRUE if @font has the requested codepoint.
 *
 * Since: 1.2
 **/
gboolean
pango_xft_font_has_char (PangoFont *font,
			 gunichar   wc)
{
  g_return_val_if_fail (PANGO_XFT_IS_FONT (font), 0);

  return pango_fc_font_has_char (PANGO_FC_FONT (font), wc);
}
