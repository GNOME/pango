/* Pango
 * pangox.h:
 *
 * Copyright (C) 1999 Red Hat Software
 * Copyright (C) 2000 SuSE Linux Ltd
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

#ifndef __PANGOXFT_H__
#define __PANGOXFT_H__

#include <pango/pango-context.h>
#include <pango/pango-ot.h>
#include <pango/pangofc-font.h>

G_BEGIN_DECLS

#define _XFT_NO_COMPAT
#define _XFTCOMPAT_H_
#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#if defined(XftVersion) && XftVersion >= 20000
#else
#error "must have Xft version 2 or newer"
#endif

#ifndef PANGO_DISABLE_DEPRECATED
#define PANGO_RENDER_TYPE_XFT "PangoRenderXft"
#endif

typedef void (*PangoXftSubstituteFunc) (FcPattern *pattern,
                                        gpointer   data);

/* Calls for applications
 */
PangoFontMap *pango_xft_get_font_map     (Display *display,
					  int      screen);
PangoContext *pango_xft_get_context      (Display *display,
					  int      screen);
void          pango_xft_shutdown_display (Display *display,
					  int      screen);

void          pango_xft_render         (XftDraw          *draw,
					XftColor         *color,
					PangoFont        *font,
					PangoGlyphString *glyphs,
					gint              x,
					gint              y);
void          pango_xft_picture_render (Display          *display,
					Picture           src_picture,
					Picture           dest_picture,
					PangoFont        *font,
					PangoGlyphString *glyphs,
					gint              x,
					gint              y);

void pango_xft_set_default_substitute (Display                *display,
				       int                     screen,
				       PangoXftSubstituteFunc  func,
				       gpointer                data,
				       GDestroyNotify          notify);
void pango_xft_substitute_changed     (Display                *display,
				       int                     screen);

#define PANGO_TYPE_XFT_FONT              (pango_xft_font_get_type ())
#define PANGO_XFT_IS_FONT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_XFT_FONT))

GType      pango_xft_font_get_type (void);

/* For shape engines
 */

#ifdef PANGO_ENABLE_ENGINE
XftFont *     pango_xft_font_get_font          (PangoFont *font);
Display *     pango_xft_font_get_display       (PangoFont *font);
#ifndef PANGO_DISABLE_DEPRECATED
FT_Face       pango_xft_font_lock_face         (PangoFont *font);
void	      pango_xft_font_unlock_face       (PangoFont *font);
guint	      pango_xft_font_get_glyph	       (PangoFont *font,
						gunichar   wc);
gboolean      pango_xft_font_has_char          (PangoFont *font,
						gunichar   wc);
PangoGlyph    pango_xft_font_get_unknown_glyph (PangoFont *font,
						gunichar   wc);
#endif /* PANGO_DISABLE_DEPRECATED */
#endif /* PANGO_ENABLE_ENGINE */

G_END_DECLS

#endif /* __PANGOXFT_H__ */
