/* pangox.c: Routines for handling X fonts
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

#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <X11/Xlib.h>
#include "pango-impl-utils.h"

#undef PANGO_DISABLE_DEPRECATED

#include "pangox.h"
#include "pangox-private.h"

#define PANGO_TYPE_X_FONT              (pango_x_font_get_type ())
#define PANGO_X_FONT(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_X_FONT, PangoXFont))
#define PANGO_X_FONT_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_X_FONT, PangoXFontClass))
#define PANGO_X_IS_FONT(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_X_FONT))
#define PANGO_X_IS_FONT_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), PANGO_TYPE_X_FONT))
#define PANGO_X_FONT_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_X_FONT, PangoXFontClass))

typedef struct _PangoXFontClass   PangoXFontClass;
typedef struct _PangoXMetricsInfo PangoXMetricsInfo;
typedef struct _PangoXContextInfo PangoXContextInfo;

struct _PangoXSubfontInfo
{
  char *xlfd;
  XFontStruct *font_struct;
  gboolean     is_1byte;
  int          range_byte1;
  int          range_byte2;
};

struct _PangoXMetricsInfo
{
  const char *sample_str;
  PangoFontMetrics *metrics;
};

struct _PangoXContextInfo
{
  PangoGetGCFunc  get_gc_func;
  PangoFreeGCFunc free_gc_func;
};

struct _PangoXFontClass
{
  PangoFontClass parent_class;
};

static PangoFontClass *parent_class;	/* Parent class structure for PangoXFont */

static void pango_x_font_class_init (PangoXFontClass *class);
static void pango_x_font_init       (PangoXFont      *xfont);
static void pango_x_font_dispose    (GObject         *object);
static void pango_x_font_finalize   (GObject         *object);

static PangoFontDescription *pango_x_font_describe          (PangoFont        *font);
static PangoCoverage *       pango_x_font_get_coverage      (PangoFont        *font,
							     PangoLanguage    *language);
static PangoEngineShape *    pango_x_font_find_shaper       (PangoFont        *font,
							     PangoLanguage    *language,
							     guint32           ch);
static void                  pango_x_font_get_glyph_extents (PangoFont        *font,
							     PangoGlyph        glyph,
							     PangoRectangle   *ink_rect,
							     PangoRectangle   *logical_rect);
static PangoFontMetrics *    pango_x_font_get_metrics       (PangoFont        *font,
							     PangoLanguage    *language);
static PangoFontMap *        pango_x_font_get_font_map      (PangoFont        *font);

static PangoXSubfontInfo * pango_x_find_subfont    (PangoFont          *font,
						    PangoXSubfont       subfont_index);
static XCharStruct *       pango_x_get_per_char    (PangoFont          *font,
						    PangoXSubfontInfo  *subfont,
						    guint16             char_index);
static gboolean            pango_x_find_glyph      (PangoFont          *font,
						    PangoGlyph          glyph,
						    PangoXSubfontInfo **subfont_return,
						    XCharStruct       **charstruct_return);
static XFontStruct *       pango_x_get_font_struct (PangoFont          *font,
						    PangoXSubfontInfo  *info);

static void     pango_x_get_item_properties (PangoItem      *item,
					     PangoUnderline *uline,
					     PangoAttrColor *fg_color,
					     gboolean       *fg_set,
					     PangoAttrColor *bg_color,
					     gboolean       *bg_set);

static inline PangoXSubfontInfo *
pango_x_find_subfont (PangoFont  *font,
		      PangoXSubfont subfont_index)
{
  PangoXFont *xfont = (PangoXFont *)font;

  if (subfont_index < 1 || subfont_index > xfont->n_subfonts)
    return NULL;

  return xfont->subfonts[subfont_index-1];
}

static void
pango_x_make_font_struct (PangoFont *font, PangoXSubfontInfo *info)
{
  PangoXFont *xfont = (PangoXFont *)font;
  PangoXFontCache *cache;

  cache = pango_x_font_map_get_font_cache (xfont->fontmap);

  info->font_struct = pango_x_font_cache_load (cache, info->xlfd);
  if (!info->font_struct)
    {
      g_warning ("Cannot load font for XLFD '%s\n", info->xlfd);

      /* Prevent a segfault, but probably not much more */
      info->font_struct = pango_x_font_cache_load (cache, "fixed");
    }

  info->is_1byte = (info->font_struct->min_byte1 == 0 && info->font_struct->max_byte1 == 0);
  info->range_byte1 = info->font_struct->max_byte1 - info->font_struct->min_byte1 + 1;
  info->range_byte2 = info->font_struct->max_char_or_byte2 - info->font_struct->min_char_or_byte2 + 1;
}

static inline XFontStruct *
pango_x_get_font_struct (PangoFont *font, PangoXSubfontInfo *info)
{
  if (!info->font_struct)
    pango_x_make_font_struct (font, info);

  return info->font_struct;
}

static void
free_context_info (PangoXContextInfo *info)
{
  g_slice_free (PangoXContextInfo, info);
}

static PangoXContextInfo *
get_context_info (PangoContext *context)
{
  PangoXContextInfo *info;
  static quark = 0;

  if (G_UNLIKELY (!quark))
    quark = g_quark_from_static_string ("pango-x-info");

  info =  g_object_get_qdata (G_OBJECT (context), quark);

  if (G_UNLIKELY (!info))
    {
      info = g_slice_new (PangoXContextInfo);
      info->get_gc_func = NULL;
      info->free_gc_func = NULL;
      g_object_set_qdata_full (G_OBJECT (context),
			       quark,
			       info, (GDestroyNotify)free_context_info);
    }

  return info;
}

/**
 * pango_x_get_context:
 * @display: an X display (As returned by XOpenDisplay().)
 *
 * Retrieves a #PangoContext appropriate for rendering with X fonts on the
 * given display.
 *
 * Return value: the new #PangoContext.
 *
 * Deprecated: 1.22: Use pango_x_font_map_for_display() followed by
 * pango_font_map_create_context() instead.
 **/
PangoContext *
pango_x_get_context (Display *display)
{
  return pango_font_map_create_context (pango_x_font_map_for_display (display));
}

/**
 * pango_x_context_set_funcs:
 * @context: a #PangoContext.
 * @get_gc_func: function called to create a new GC for a given color.
 * @free_gc_func: function called to free a GC created with @get_gc_func.
 *
 * Sets the functions that will be used to get GC's in various colors when
 * rendering layouts with this context.
 **/
void
pango_x_context_set_funcs  (PangoContext     *context,
			    PangoGetGCFunc    get_gc_func,
			    PangoFreeGCFunc   free_gc_func)
{
  PangoXContextInfo *info;

  g_return_if_fail (context != NULL);

  info = get_context_info (context);

  info->get_gc_func = get_gc_func;
  info->free_gc_func = free_gc_func;
}

static GType
pango_x_font_get_type (void)
{
  static GType object_type = 0;

  if (G_UNLIKELY (!object_type))
    {
      const GTypeInfo object_info =
      {
	sizeof (PangoXFontClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) pango_x_font_class_init,
	NULL,           /* class_finalize */
	NULL,           /* class_data */
	sizeof (PangoXFont),
	0,              /* n_preallocs */
	(GInstanceInitFunc) pango_x_font_init,
	NULL            /* value_table */
      };

      object_type = g_type_register_static (PANGO_TYPE_FONT,
					    I_("PangoXFont"),
					    &object_info, 0);
    }

  return object_type;
}

static void
pango_x_font_init (PangoXFont *xfont)
{
  xfont->subfonts_by_charset = g_hash_table_new (g_str_hash, g_str_equal);

  xfont->n_subfonts = 0;
  xfont->max_subfonts = 1;

  xfont->subfonts = g_new (PangoXSubfontInfo *, xfont->max_subfonts);

  xfont->metrics_by_lang = NULL;

  xfont->size = -1;
  xfont->xface = NULL;
}

static void
pango_x_font_class_init (PangoXFontClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontClass *font_class = PANGO_FONT_CLASS (class);

  parent_class = g_type_class_peek_parent (class);

  object_class->finalize = pango_x_font_finalize;
  object_class->dispose = pango_x_font_dispose;

  font_class->describe = pango_x_font_describe;
  font_class->get_coverage = pango_x_font_get_coverage;
  font_class->find_shaper = pango_x_font_find_shaper;
  font_class->get_glyph_extents = pango_x_font_get_glyph_extents;
  font_class->get_metrics = pango_x_font_get_metrics;
  font_class->get_font_map = pango_x_font_get_font_map;
}

PangoXFont *
pango_x_font_new (PangoFontMap *fontmap, const char *spec, int size)
{
  PangoXFont *result;

  g_return_val_if_fail (fontmap != NULL, NULL);
  g_return_val_if_fail (spec != NULL, NULL);

  result = g_object_new (PANGO_TYPE_X_FONT, NULL);

  g_assert (result->fontmap == NULL);
  result->fontmap = fontmap;
  g_object_add_weak_pointer (G_OBJECT (result->fontmap), (gpointer *) (gpointer) &result->fontmap);

  result->display = pango_x_fontmap_get_display (fontmap);

  result->fonts = g_strsplit(spec, ",", -1);
  for (result->n_fonts = 0; result->fonts[result->n_fonts]; result->n_fonts++)
    ; /* Nothing */

  result->size = size;

  return result;
}

/**
 * pango_x_load_font:
 * @display: the X display.
 * @spec:    a comma-separated list of XLFD's.
 *
 * Loads up a logical font based on a "fontset" style text
 * specification. This is not remotely useful (Pango API's generally
 * work in terms of #PangoFontDescription) and the result may not
 * work correctly in all circumstances. Use of this function should
 * be avoided.
 *
 * Returns: a new #PangoFont.
 */
PangoFont *
pango_x_load_font (Display    *display,
		   const char *spec)
{
  PangoXFont *result;

  g_return_val_if_fail (display != NULL, NULL);
  g_return_val_if_fail (spec != NULL, NULL);

  result = pango_x_font_new (pango_x_font_map_for_display (display), spec, -1);

  return (PangoFont *)result;
}


#define FLUSH						\
  G_STMT_START {					\
    if (charcount)					\
      {							\
	XDrawString16 (display, d, gc,			\
		       glyph_x0, glyph_y0,		\
		       xcharbuffer, charcount);		\
	charcount = 0;					\
      }							\
  } G_STMT_END


/**
 * pango_x_render:
 * @display: the X display.
 * @d:       the drawable on which to draw string.
 * @gc:      the graphics context.
 * @font:    the font in which to draw the string.
 * @glyphs:  the glyph string to draw.
 * @x:       the x position of start of string (in pixels).
 * @y:       the y position of baseline (in pixels).
 *
 * Renders a #PangoGlyphString onto an X drawable.
 */
void
pango_x_render  (Display           *display,
		 Drawable           d,
		 GC                 gc,
		 PangoFont         *font,
		 PangoGlyphString  *glyphs,
		 int                x,
		 int                y)
{
  Font old_fid = None;
  XFontStruct *fs;
  int i;
  int x_off = 0;

  /*
   * We collect the characters in this buffer as long as the font does not
   * change.  At that time, or when the buffer runs full, or at the end,
   * then we empty the buffer.
   */
  XChar2b xcharbuffer[1000];
  int glyph_x0 = 0, expected_x = 0; /* x/y initializations are to quiet GCC */
  int glyph_y0 = 0;
  int charcount = 0;

  g_return_if_fail (display != NULL);
  g_return_if_fail (glyphs != NULL);

  for (i=0; i<glyphs->num_glyphs; i++)
    {
      PangoGlyph glyph = glyphs->glyphs[i].glyph;
      int glyph_x = x + PANGO_PIXELS (x_off + glyphs->glyphs[i].geometry.x_offset);
      int glyph_y = y + PANGO_PIXELS (glyphs->glyphs[i].geometry.y_offset);

      /* Clip glyphs into the X coordinate range; we really
       * want to clip glyphs with an ink rect outside the
       * [0,32767] x [0,32767] rectangle but looking up
       * the ink rect here would be a noticeable speed hit.
       * This is close enough.
       */
      if (!(glyph != PANGO_GLYPH_EMPTY &&
	    glyph_x >= -16384 && glyph_x <= 32767 &&
	    glyph_y >= -16384 && glyph_y <= 32767))
	goto next_glyph;

      if (G_LIKELY ((glyph & PANGO_GLYPH_UNKNOWN_FLAG) == 0))
	{
	  guint16 index = PANGO_X_GLYPH_INDEX (glyph);
	  guint16 subfont_index = PANGO_X_GLYPH_SUBFONT (glyph);
	  PangoXSubfontInfo *subfont;

	  subfont = pango_x_find_subfont (font, subfont_index);
	  if (subfont)
	    {
	      fs = pango_x_get_font_struct (font, subfont);
	      if (!fs)
		continue;

	      if (fs->fid != old_fid)
		{
		  FLUSH;
		  XSetFont (display, gc, fs->fid);
		  old_fid = fs->fid;
		}

	      if (charcount == G_N_ELEMENTS (xcharbuffer) ||
		  (charcount > 0 && (glyph_y != glyph_y0 ||
				     glyph_x != expected_x)))
		FLUSH;

	      if (charcount == 0)
		{
		  glyph_x0 = glyph_x;
		  glyph_y0 = glyph_y;
		}
	      xcharbuffer[charcount].byte1 = index / 256;
	      xcharbuffer[charcount].byte2 = index % 256;

	      expected_x = glyph_x + XTextWidth16 (fs, &xcharbuffer[charcount], 1);

	      charcount++;
	    } else
	      goto unknown_glyph;
	} else {
	  PangoFontMetrics *metrics;
	  int x1, y1, x2, y2; /* rectangle the character should go inside. */
	  int baseline;
	  int stroke_thick;
	  gunichar wc;
	  gboolean invalid_input;

	unknown_glyph:
	  FLUSH;

	  if (font)
	    metrics = pango_font_get_metrics (font, NULL);
	  else
	    metrics = NULL;

	  if (metrics)
	    {
	      y1 = glyph_y - PANGO_PIXELS (metrics->ascent);
	      y2 = y1 + PANGO_PIXELS (metrics->ascent + metrics->descent);
	    }
	  else
	    {
	      y2 = glyph_y;
	      y1 = y2 - PANGO_UNKNOWN_GLYPH_HEIGHT;
	    }

	  x1 = glyph_x;
	  x2 = x1 + PANGO_PIXELS (glyphs->glyphs[i].geometry.width);

	  baseline = glyph_y;
	  stroke_thick = MAX ((int) (0.5 + 0.025 * (y2 - y1)), 1);

	  if (glyph & PANGO_GLYPH_UNKNOWN_FLAG)
	    wc = glyph & ~PANGO_GLYPH_UNKNOWN_FLAG;
	  else
	    wc = 0;
	  invalid_input = glyph == PANGO_GLYPH_INVALID_INPUT || wc > 0x10FFFF;

	  switch (wc)
	    {
	    case '\n':
	    case '\r':
	    case 0x2028: /* Line separator */
	    case 0x2029: /* Paragraph separator */
	      {
		/* Draw a carriage-return thingy */
		PangoRectangle up_stroke;
		PangoRectangle across_stroke;

		int hborder = (x2 - x1) * 0.1;
		int arrow_height = 0.25 * (y2 - y1);
		int top_border = (y2 - y1) * 0.25;

		int arrow_x, arrow_width, tmp_height;

		/* Draw the arrow-head */

		tmp_height = (stroke_thick % 2 == 0) ? 2 : 1; /* Starting height */
		arrow_height = 2 * ((1 + arrow_height - tmp_height) / 2) + tmp_height; /* Force symmetry */
		arrow_width = 2 + arrow_height - tmp_height;

		for (arrow_x = x1 + hborder; arrow_x < x1 + hborder + arrow_width; arrow_x++)
		  {
		    XDrawLine (display, d, gc,
			       arrow_x,
			       baseline - stroke_thick + (stroke_thick - tmp_height) / 2,
			       arrow_x,
			       baseline - stroke_thick + (stroke_thick - tmp_height) / 2 + tmp_height - 1);

		    if ((arrow_x - x1 - hborder) % 2 == 1)
		      tmp_height += 2;
		  }

		across_stroke.x = arrow_x;
		across_stroke.width = x2 - hborder - arrow_x - stroke_thick;
		across_stroke.y = baseline - stroke_thick;
		across_stroke.height = stroke_thick;

		XFillRectangle (display, d, gc,
				across_stroke.x, across_stroke.y,
				across_stroke.width, across_stroke.height);

		up_stroke.x = across_stroke.x + across_stroke.width;
		up_stroke.width = stroke_thick;
		up_stroke.y = y1 + top_border;
		up_stroke.height = baseline - up_stroke.y;

		XFillRectangle (display, d, gc,
				up_stroke.x, up_stroke.y,
				up_stroke.width, up_stroke.height);
	      }
	      break;

	    default:
	      {
		/* Perhaps we should draw the box-with-numbers as in the
		 * other backends, though we have no guarantee of having
		 * an appropriate size of font. Right now, we just
		 * draw an empty box. (To draw the box-with-numbers.
		 * the backends would have to be changed to use
		 * pango_x_font_get_unknown_glyph() rather than
		 * pango_x_get_unknown_glyph().
		 */

		int xspace = MAX ((int) (0.5 + 0.1 * (x2 - x1)), 1);
		int yspace = MAX ((int) (0.5 + 0.1 * (y2 - y1)), 1);

		x1 += xspace;
		x2 -= xspace;
		y1 += yspace;
		y2 -= yspace;

		XFillRectangle (display, d, gc,
				x1, y1,
				x2 - x1, stroke_thick);
		XFillRectangle (display, d, gc,
				x1, y1 + stroke_thick,
				stroke_thick, y2 - y1 - 2 * stroke_thick);
		XFillRectangle (display, d, gc,
				x2 - stroke_thick, y1 + stroke_thick,
				stroke_thick, y2 - y1 - 2 * stroke_thick);
		XFillRectangle (display, d, gc,
				x1, y2 - stroke_thick,
				x2 - x1, stroke_thick);
		if (invalid_input)
		  {
		    XDrawLine (display, d, gc,
			       x1, y1,
			       x2-1, y2-1);
		    XDrawLine (display, d, gc,
			       x2-1, y1,
			       x1, y2-1);
		  }

		break;
	      }
	    }

	  pango_font_metrics_unref (metrics);
	}

    next_glyph:
      x_off += glyphs->glyphs[i].geometry.width;
    }
  FLUSH;
}

#undef FLUSH

static void
pango_x_font_get_glyph_extents  (PangoFont      *font,
				 PangoGlyph      glyph,
				 PangoRectangle *ink_rect,
				 PangoRectangle *logical_rect)
{
  XCharStruct *cs;
  PangoXSubfontInfo *subfont;

  if (glyph == PANGO_GLYPH_EMPTY)
    {
      if (ink_rect)
	ink_rect->x = ink_rect->width = ink_rect->y = ink_rect->height = 0;
      if (logical_rect)
	logical_rect->x = logical_rect->width = logical_rect->y = logical_rect->height = 0;
      return;
    }
  if ((glyph & PANGO_GLYPH_UNKNOWN_FLAG) == 0 && pango_x_find_glyph (font, glyph, &subfont, &cs))
    {
      if (ink_rect)
	{
	  ink_rect->x = PANGO_SCALE * cs->lbearing;
	  ink_rect->width = PANGO_SCALE * (cs->rbearing - cs->lbearing);
	  ink_rect->y = PANGO_SCALE * -cs->ascent;
	  ink_rect->height = PANGO_SCALE * (cs->ascent + cs->descent);
	}
      if (logical_rect)
	{
	  logical_rect->x = 0;
	  logical_rect->width = PANGO_SCALE * cs->width;
	  logical_rect->y = - PANGO_SCALE * subfont->font_struct->ascent;
	  logical_rect->height = PANGO_SCALE * (subfont->font_struct->ascent + subfont->font_struct->descent);
	}
    }
  else
    {
      PangoFontMetrics *metrics;
      gunichar wc;
      gdouble width_factor;
      int w;

      if (glyph & PANGO_GLYPH_UNKNOWN_FLAG)
	wc = glyph & (~PANGO_GLYPH_UNKNOWN_FLAG);
      else
	wc = 0;

      switch (wc)
	{
	case '\n':
	case '\r':
	case 0x2028: /* Line separator */
	case 0x2029: /* Paragraph separator */
	  {
#define MAGIC_FACTOR 1.2

	    /* carriage-return thingy */
	    width_factor = MAGIC_FACTOR;
	    break;
	  }
	default:
	  {
	    /* Unknown glyph square */
	    width_factor = 1.0;
	  }
	}

      metrics = pango_font_get_metrics (font, NULL);

      if (metrics)
	{
	  w = metrics->approximate_char_width * width_factor;
	  w = PANGO_SCALE * PANGO_PIXELS (w);

	  if (ink_rect)
	    {
	      ink_rect->x = PANGO_SCALE;
	      ink_rect->width = w - 2 * PANGO_SCALE;
	      ink_rect->y = - (metrics->ascent - PANGO_SCALE);
	      ink_rect->height = metrics->ascent + metrics->descent - 2 * PANGO_SCALE;
	    }
	  if (logical_rect)
	    {
	      logical_rect->x = 0;
	      logical_rect->width = w;
	      logical_rect->y = - metrics->ascent;
	      logical_rect->height = metrics->ascent + metrics->descent;
	    }

	  pango_font_metrics_unref (metrics);
	}
      else
	{
	  if (ink_rect)
	    ink_rect->x = ink_rect->y = ink_rect->height = ink_rect->width = 0;
	  if (logical_rect)
	    logical_rect->x = logical_rect->y = logical_rect->height = logical_rect->width = 0;
	}
    }
}

static gboolean
get_int_prop (Atom         atom,
	      XFontStruct *fs,
	      int         *val)
{
  int i;

  *val = 0;

  i = 0;
  while (i < fs->n_properties)
    {
      if (fs->properties[i].name == atom)
	{
	  *val = fs->properties[i].card32;
	  return TRUE;
	}

      ++i;
    }

  return FALSE;
}

/* Call @func with each glyph resulting from shaping @string with each
 * glyph. This duplicates quite a bit of code from pango_itemize. This
 * function should die and we should simply add the ability to specify
 * particular fonts when itemizing.
 */
static void
itemize_string_foreach (PangoFont     *font,
			PangoLanguage *language,
			const char    *str,
			void         (*func) (PangoFont      *font,
					      PangoGlyphInfo *glyph_info,
					      gpointer        data),
			gpointer       data)
{
  const char *start, *p;
  PangoGlyphString *glyph_str = pango_glyph_string_new ();
  PangoEngineShape *shaper, *last_shaper;
  int last_level;
  int i;
  guint8 *embedding_levels;
  PangoDirection base_dir = PANGO_DIRECTION_LTR;
  gboolean finished = FALSE;

  embedding_levels = pango_log2vis_get_embedding_levels (str, -1, &base_dir);

  last_shaper = NULL;
  last_level = 0;

  i = 0;
  p = start = str;
  while (*p || !finished)
    {
      gunichar wc;

      if (*p)
	{
	  wc = g_utf8_get_char (p);
	  shaper = pango_font_find_shaper (font, language, wc);
	}
      else
	{
	  finished = TRUE;
	  shaper = NULL;
	}

      if (p > start &&
	  (finished ||
	   (shaper != last_shaper || last_level != embedding_levels[i])))
	{
	  PangoAnalysis analysis = { NULL };
	  int j;

	  analysis.shape_engine = last_shaper;
	  analysis.font = font;
	  analysis.language = language;
	  analysis.level = last_level;

	  pango_shape (start, p - start, &analysis, glyph_str);

	  for (j = 0; j < glyph_str->num_glyphs; j++)
	    (*func) (font, &glyph_str->glyphs[j], data);

	  start = p;
	}

      if (!finished)
	{
	  p = g_utf8_next_char (p);

	  last_shaper = shaper;
	  last_level = embedding_levels[i];
	  i++;
	}
    }

  pango_glyph_string_free (glyph_str);
  g_free (embedding_levels);
}

/* Get composite font metrics for all subfonts in list
 */
static void
get_font_metrics_from_subfonts (PangoFont        *font,
				GSList           *subfonts,
				PangoFontMetrics *metrics)
{
  PangoXFont *xfont = (PangoXFont *)font;
  GSList *tmp_list = subfonts;
  gboolean first = TRUE;
  int total_avg_widths = 0;
  int n_avg_widths = 0;
  Atom avg_width_atom;

  avg_width_atom = pango_x_fontmap_atom_from_name (xfont->fontmap,
						   "AVERAGE_WIDTH");

  metrics->ascent = 0;
  metrics->descent = 0;

  while (tmp_list)
    {
      PangoXSubfontInfo *subfont = pango_x_find_subfont (font, GPOINTER_TO_UINT (tmp_list->data));

      if (subfont)
	{
	  XFontStruct *fs = pango_x_get_font_struct (font, subfont);
	  gint avg_width = 0;

	  if (fs)
	    {
	      if (first)
		{
		  metrics->ascent = fs->ascent * PANGO_SCALE;
		  metrics->descent = fs->descent * PANGO_SCALE;
		  first = FALSE;
		}
	      else
		{
		  metrics->ascent = MAX (fs->ascent * PANGO_SCALE, metrics->ascent);
		  metrics->descent = MAX (fs->descent * PANGO_SCALE, metrics->descent);
		}

	      if (get_int_prop (avg_width_atom, fs, &avg_width))
		{
		  /* convert decipoints --> Pango units.
		   * Resolution is in (points * PANGO_SCALE) / pixel,
		   * avg_width in decipoints.
		   * We want pixels * PANGO_SCALE
		   */

		  /* Convert to points * PANGO_SCALE */
		  avg_width *= PANGO_SCALE / (double) 10.0;
		  /* Convert to pixels * PANGO_SCALE */
		  avg_width *= (PANGO_SCALE / PANGO_X_FONT_MAP (PANGO_X_FONT (font)->fontmap)->resolution);
		}
	      else
		{
		  avg_width = PANGO_SCALE * ((fs->min_bounds.width + fs->max_bounds.width) / 2);
		}
	    }

	  if (avg_width)
	    {
	      total_avg_widths += avg_width;
	      n_avg_widths += 1;
	    }
	}
      else
	g_warning ("Invalid subfont %d in get_font_metrics_from_subfonts", GPOINTER_TO_UINT (tmp_list->data));

      tmp_list = tmp_list->next;
    }

  /* This is pretty darn bogus. */
  if (n_avg_widths)
    metrics->approximate_char_width = total_avg_widths / n_avg_widths;
  else
    metrics->approximate_char_width = PANGO_UNKNOWN_GLYPH_WIDTH * PANGO_SCALE;
  if (metrics->ascent + metrics->descent == 0)
    {
      metrics->ascent = PANGO_UNKNOWN_GLYPH_HEIGHT * PANGO_SCALE;
      metrics->descent = 0;
    }
}

static void
get_subfonts_foreach (PangoFont      *font,
		      PangoGlyphInfo *glyph_info,
		      gpointer        data)
{
  GSList **subfonts = data;
  PangoGlyph glyph = glyph_info->glyph;
  PangoXSubfont subfont;

  if (glyph == PANGO_GLYPH_EMPTY)
    return;

  /* Use an arbitrary subfont for unknown glyphs...*/
  if (glyph & PANGO_GLYPH_UNKNOWN_FLAG)
    {
    if (((PangoXFont *)font)->n_subfonts > 0)
      glyph = PANGO_X_MAKE_GLYPH (1, 0);
    else
      return;
    }

  subfont = PANGO_X_GLYPH_SUBFONT (glyph);
  if (!g_slist_find (*subfonts, GUINT_TO_POINTER ((guint)subfont)))
    *subfonts = g_slist_prepend (*subfonts, GUINT_TO_POINTER ((guint)subfont));
}

/* Get composite font metrics for all subfonts resulting from shaping
 * string str with the given font
 */
static void
get_font_metrics_from_string (PangoFont        *font,
			      PangoLanguage    *language,
			      const char       *str,
			      PangoFontMetrics *metrics)
{
  GSList *subfonts = NULL;

  itemize_string_foreach (font, language, str, get_subfonts_foreach, &subfonts);
  get_font_metrics_from_subfonts (font, subfonts, metrics);
  g_slist_free (subfonts);
}

static void
average_width_foreach (PangoFont      *font,
		       PangoGlyphInfo *glyph_info,
		       gpointer        data)
{
  int *width = data;

  *width += glyph_info->geometry.width;
}

/* Get composite font metrics for all subfonts resulting from shaping
 * string str with the given font
 */
static gdouble
get_total_width_for_string (PangoFont        *font,
			    PangoLanguage    *language,
			    const char       *str)
{
  int width = 0;

  itemize_string_foreach (font, language, str, average_width_foreach, &width);

  return width;
}

static PangoFontMetrics *
pango_x_font_get_metrics (PangoFont        *font,
			  PangoLanguage    *language)
{
  PangoXMetricsInfo *info = NULL; /* Quiet gcc */
  PangoXFont *xfont = (PangoXFont *)font;
  GSList *tmp_list;

  const char *sample_str = pango_language_get_sample_string (language);

  tmp_list = xfont->metrics_by_lang;
  while (tmp_list)
    {
      info = tmp_list->data;

      if (info->sample_str == sample_str)    /* We _don't_ need strcmp */
	break;

      tmp_list = tmp_list->next;
    }

  if (!tmp_list)
    {
      PangoFontMetrics *metrics;

      info = g_slice_new0 (PangoXMetricsInfo);

      xfont->metrics_by_lang = g_slist_prepend (xfont->metrics_by_lang, info);

      info->sample_str = sample_str;
      metrics = pango_font_metrics_new ();

      get_font_metrics_from_string (font, language, sample_str, metrics);

      metrics->approximate_digit_width = get_total_width_for_string (font, language, "0123456789") / 10;

      info->metrics = metrics;
    }

  return pango_font_metrics_ref (info->metrics);
}

static PangoFontMap *
pango_x_font_get_font_map (PangoFont *font)
{
  PangoXFont *xfont = (PangoXFont *)font;

  return xfont->fontmap;
}

/* Compare the tail of a to b */
static gboolean
match_end (const char *a, const char *b)
{
  size_t len_a = strlen (a);
  size_t len_b = strlen (b);

  if (len_b > len_a)
    return FALSE;
  else
    return (strcmp (a + len_a - len_b, b) == 0);
}

/* Substitute in a charset into an XLFD. Return the
 * (g_malloc'd) new name, or %NULL if the XLFD cannot
 * match the charset
 */
static char *
name_for_charset (char *xlfd, char *charset)
{
  char *p;
  char *dash_charset = g_strconcat ("-", charset, NULL);
  char *result = NULL;
  int ndashes = 0;

  for (p = xlfd; *p; p++)
    if (*p == '-')
      ndashes++;

  if (ndashes == 14) /* Complete XLFD */
    {
      if (match_end (xlfd, "-*-*"))
	{
	  result = g_malloc (strlen (xlfd) - 4 + strlen (dash_charset) + 1);
	  strncpy (result, xlfd, strlen (xlfd) - 4);
	  strcpy (result + strlen (xlfd) - 4, dash_charset);
	}
      if (match_end (xlfd, dash_charset))
	result = g_strdup (xlfd);
    }
  else if (ndashes == 13)
    {
      if (match_end (xlfd, "-*"))
	{
	  result = g_malloc (strlen (xlfd) - 2 + strlen (dash_charset) + 1);
	  strncpy (result, xlfd, strlen (xlfd) - 2);
	  strcpy (result + strlen (xlfd) - 2, dash_charset);
	}
      if (match_end (xlfd, dash_charset))
	result = g_strdup (xlfd);
    }
  else
    {
      if (match_end (xlfd, "*"))
	{
	  result = g_malloc (strlen (xlfd) + strlen (dash_charset) + 1);
	  strcpy (result, xlfd);
	  strcpy (result + strlen (xlfd), dash_charset);
	}
      if (match_end (xlfd, dash_charset))
	result = g_strdup (xlfd);
    }

  g_free (dash_charset);
  return result;
}

static PangoXSubfont
pango_x_insert_subfont (PangoFont *font, const char *xlfd)
{
  PangoXFont *xfont = (PangoXFont *)font;
  PangoXSubfontInfo *info;

  info = g_slice_new (PangoXSubfontInfo);

  info->xlfd = g_strdup (xlfd);
  info->font_struct = NULL;

  xfont->n_subfonts++;

  if (xfont->n_subfonts > xfont->max_subfonts)
    {
      xfont->max_subfonts *= 2;
      xfont->subfonts = g_renew (PangoXSubfontInfo *, xfont->subfonts, xfont->max_subfonts);
    }

  xfont->subfonts[xfont->n_subfonts - 1] = info;

  return xfont->n_subfonts;
}

/**
 * pango_x_list_subfonts:
 * @font: a #PangoFont.
 * @charsets: the charsets to list subfonts for.
 * @n_charsets: the number of charsets in @charsets.
 * @subfont_ids: location to store a pointer to an array of subfont IDs for each found subfont;
 *               the result must be freed using g_free().
 * @subfont_charsets: location to store a pointer to an array of subfont IDs for each found subfont;
 *               the result must be freed using g_free().
 *
 * Lists the subfonts of a given font. The result is ordered first by charset,
 * and then within each charset, by the order of fonts in the font specification.
 *
 * Return value: length of the arrays stored in @subfont_ids and
 * @subfont_charsets.
 **/
int
pango_x_list_subfonts (PangoFont        *font,
		       char            **charsets,
		       int               n_charsets,
		       PangoXSubfont   **subfont_ids,
		       int             **subfont_charsets)
{
  PangoXFont *xfont = (PangoXFont *)font;
  PangoXSubfont **subfont_lists;
  PangoFontMap *fontmap;
  int i, j;
  int n_subfonts = 0;

  g_return_val_if_fail (font != NULL, 0);
  g_return_val_if_fail (n_charsets == 0 || charsets != NULL, 0);

  fontmap = pango_x_font_map_for_display (xfont->display);

  subfont_lists = g_new (PangoXSubfont *, n_charsets);

  for (j=0; j<n_charsets; j++)
    {
      subfont_lists[j] = g_hash_table_lookup (xfont->subfonts_by_charset, charsets[j]);
      if (!subfont_lists[j])
	{
	  subfont_lists[j] = g_new (PangoXSubfont, xfont->n_fonts);

	  for (i = 0; i < xfont->n_fonts; i++)
	    {
	      PangoXSubfont subfont = 0;
	      char *xlfd;

	      if (xfont->size == -1)
		{
		  xlfd = name_for_charset (xfont->fonts[i], charsets[j]);

		  if (xlfd)
		    {
		      int count;
		      char **names = XListFonts (xfont->display, xlfd, 1, &count);
		      if (count > 0)
			subfont = pango_x_insert_subfont (font, names[0]);

		      XFreeFontNames (names);
		      g_free (xlfd);
		    }
		}
	      else
		{
		  xlfd = pango_x_make_matching_xlfd (fontmap, xfont->fonts[i], charsets[j], xfont->size);
		  if (xlfd)
		    {
		      subfont = pango_x_insert_subfont (font, xlfd);
		      g_free (xlfd);
		    }
		}

	      subfont_lists[j][i] = subfont;
	    }

	  g_hash_table_insert (xfont->subfonts_by_charset, g_strdup (charsets[j]), subfont_lists[j]);
	}

      for (i = 0; i < xfont->n_fonts; i++)
	if (subfont_lists[j][i])
	  n_subfonts++;
    }

  *subfont_ids = g_new (PangoXSubfont, n_subfonts);
  *subfont_charsets = g_new (int, n_subfonts);

  n_subfonts = 0;

  for (j=0; j<n_charsets; j++)
    for (i=0; i<xfont->n_fonts; i++)
      if (subfont_lists[j][i])
	{
	  (*subfont_ids)[n_subfonts] = subfont_lists[j][i];
	  (*subfont_charsets)[n_subfonts] = j;
	  n_subfonts++;
	}

  g_free (subfont_lists);

  return n_subfonts;
}

/**
 * pango_x_has_glyph:
 * @font: a #PangoFont which must be from the X backend.
 * @glyph: the index of a glyph in the font. (Formed
 *         using the #PANGO_X_MAKE_GLYPH macro)
 *
 * Checks if the given glyph is present in a X font.
 *
 * Return value: %TRUE if the glyph is present.
 **/
gboolean
pango_x_has_glyph (PangoFont  *font,
		   PangoGlyph  glyph)
{
  PangoXSubfontInfo *subfont;
  XCharStruct *cs;

  guint16 char_index = PANGO_X_GLYPH_INDEX (glyph);
  guint16 subfont_index = PANGO_X_GLYPH_SUBFONT (glyph);

  subfont = pango_x_find_subfont (font, subfont_index);
  if (!subfont)
    return FALSE;

  cs = pango_x_get_per_char (font, subfont, char_index);

  if (cs && (cs->lbearing != cs->rbearing || cs->width != 0))
    return TRUE;
  else
    return FALSE;
}

/**
 * pango_x_font_subfont_xlfd:
 * @font: a #PangoFont which must be from the X backend.
 * @subfont_id: the id of a subfont within the font.
 *
 * Determines the X Logical Font Description for the specified
 * subfont.
 *
 * Return value: A newly-allocated string containing the XLFD for the
 * subfont. This string must be freed with g_free().
 **/
char *
pango_x_font_subfont_xlfd (PangoFont     *font,
			   PangoXSubfont  subfont_id)
{
  PangoXSubfontInfo *subfont;

  g_return_val_if_fail (font != NULL, NULL);
  g_return_val_if_fail (PANGO_X_IS_FONT (font), NULL);

  subfont = pango_x_find_subfont (font, subfont_id);
  if (!subfont)
    {
      g_warning ("pango_x_font_subfont_xlfd: Invalid subfont_id specified");
      return NULL;
    }

  return g_strdup (subfont->xlfd);
}

static void
pango_x_font_dispose (GObject *object)
{
  PangoXFont *xfont = PANGO_X_FONT (object);

  /* If the font is not already in the freed-fonts cache, add it,
   * if it is already there, do nothing and the font will be
   * freed.
   */
  if (!xfont->in_cache && xfont->fontmap)
    pango_x_fontmap_cache_add (xfont->fontmap, xfont);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}


static void
subfonts_foreach (gpointer key, gpointer value, gpointer data)
{
  g_free (key);
  g_free (value);
}

static void
free_metrics_info (PangoXMetricsInfo *info)
{
  pango_font_metrics_unref (info->metrics);
  g_slice_free (PangoXMetricsInfo, info);
}

static void
pango_x_font_finalize (GObject *object)
{
  PangoXFont *xfont = (PangoXFont *)object;
  PangoXFontCache *cache = pango_x_font_map_get_font_cache (xfont->fontmap);

  int i;

  for (i=0; i<xfont->n_subfonts; i++)
    {
      PangoXSubfontInfo *info = xfont->subfonts[i];

      g_free (info->xlfd);

      if (info->font_struct)
	pango_x_font_cache_unload (cache, info->font_struct);

      g_slice_free (PangoXSubfontInfo, info);
    }

  g_free (xfont->subfonts);

  g_hash_table_foreach (xfont->subfonts_by_charset, subfonts_foreach, NULL);
  g_hash_table_destroy (xfont->subfonts_by_charset);

  g_slist_foreach (xfont->metrics_by_lang, (GFunc)free_metrics_info, NULL);
  g_slist_free (xfont->metrics_by_lang);

  if (xfont->xface)
    pango_x_face_remove (xfont->xface, (PangoFont *)xfont);

  g_assert (xfont->fontmap != NULL);
  g_object_remove_weak_pointer (G_OBJECT (xfont->fontmap), (gpointer *) (gpointer) &xfont->fontmap);
  xfont->fontmap = NULL;

  g_strfreev (xfont->fonts);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static PangoFontDescription *
pango_x_font_describe (PangoFont *font)
{
  /* FIXME: this doesn't work for fonts from pango_x_font_load()
   */
  PangoXFont *xfont = (PangoXFont *)font;

  if (xfont->xface)
    {
      PangoFontDescription *desc = pango_font_face_describe (PANGO_FONT_FACE (xfont->xface));
      pango_font_description_set_size (desc, xfont->size);

      return desc;
    }
  else
    return NULL;
}

PangoMap *
pango_x_get_shaper_map (PangoLanguage *language)
{
  static guint engine_type_id = 0;
  static guint render_type_id = 0;

  if (engine_type_id == 0)
    {
      engine_type_id = g_quark_from_static_string (PANGO_ENGINE_TYPE_SHAPE);
      render_type_id = g_quark_from_static_string (PANGO_RENDER_TYPE_X);
    }

  return pango_find_map (language, engine_type_id, render_type_id);
}

static PangoCoverage *
pango_x_font_get_coverage (PangoFont     *font,
			   PangoLanguage *language)
{
  PangoXFont *xfont = (PangoXFont *)font;

  return pango_x_face_get_coverage (xfont->xface, font, language);
}

static PangoEngineShape *
pango_x_font_find_shaper (PangoFont     *font,
			  PangoLanguage *language,
			  guint32        ch)
{
  PangoMap *shape_map = NULL;
  PangoScript script;

  shape_map = pango_x_get_shaper_map (language);
  script = pango_script_for_unichar (ch);
  return (PangoEngineShape *)pango_map_get_engine (shape_map, script);
}

/* Utility functions */

static XCharStruct *
pango_x_get_per_char (PangoFont         *font,
		      PangoXSubfontInfo *subfont,
		      guint16            char_index)
{
  XFontStruct *fs;

  int index;
  int byte1;
  int byte2;

  fs = pango_x_get_font_struct (font, subfont);
  if (!fs)
    return NULL;

  if (subfont->is_1byte)
    {
      index = (int)char_index - fs->min_char_or_byte2;
      if (index < 0 || index >= subfont->range_byte2)
	return NULL;
    }
  else
    {
      byte1 = (int)(char_index / 256) - fs->min_byte1;
      if (byte1 < 0 || byte1 >= subfont->range_byte1)
	return NULL;

      byte2 = (int)(char_index % 256) - fs->min_char_or_byte2;
      if (byte2 < 0 || byte2 >= subfont->range_byte2)
	return NULL;

      index = byte1 * subfont->range_byte2 + byte2;
    }

  if (fs->per_char)
    return &fs->per_char[index];
  else
    return &fs->min_bounds;
}

static gboolean
pango_x_find_glyph (PangoFont *font,
		    PangoGlyph glyph,
		    PangoXSubfontInfo **subfont_return,
		    XCharStruct **charstruct_return)
{
  PangoXSubfontInfo *subfont;
  XCharStruct *cs;

  guint16 char_index = PANGO_X_GLYPH_INDEX (glyph);
  guint16 subfont_index = PANGO_X_GLYPH_SUBFONT (glyph);

  subfont = pango_x_find_subfont (font, subfont_index);
  if (!subfont)
    return FALSE;

  cs = pango_x_get_per_char (font, subfont, char_index);

  if (cs && (cs->lbearing != cs->rbearing || cs->width != 0))
    {
      if (subfont_return)
	*subfont_return = subfont;

      if (charstruct_return)
	*charstruct_return = cs;

      return TRUE;
    }
  else
    return FALSE;
}

/**
 * pango_x_get_unknown_glyph:
 * @font: a #PangoFont.
 *
 * Returns the index of a glyph suitable for drawing unknown characters;
 * you should generally use PANGO_GET_UNKNOWN_GLYPH() instead,
 * since that may return a glyph that provides a better representation
 * of a particular char. (E.g., by showing hex digits, or a glyph
 * representative of a certain Unicode range.)
 *
 * Return value: a glyph index into @font.
 **/
PangoGlyph
pango_x_get_unknown_glyph (PangoFont *font)
{
  return PANGO_GET_UNKNOWN_GLYPH (0);
}

/**
 * pango_x_render_layout_line:
 * @display:   the X display.
 * @drawable:  the drawable on which to draw.
 * @gc:        GC to use for uncolored drawing.
 * @line:      a #PangoLayoutLine.
 * @x:         the x position of start of string (in pixels).
 * @y:         the y position of baseline (in pixels).
 *
 * Renders a #PangoLayoutLine onto an X drawable.
 */
void
pango_x_render_layout_line (Display          *display,
			    Drawable          drawable,
			    GC                gc,
			    PangoLayoutLine  *line,
			    int               x,
			    int               y)
{
  GSList *tmp_list = line->runs;
  PangoRectangle overall_rect;
  PangoRectangle logical_rect;
  PangoRectangle ink_rect;
  PangoContext *context = pango_layout_get_context (line->layout);
  PangoXContextInfo *info = get_context_info (context);

  int x_off = 0;

  pango_layout_line_get_extents (line,NULL, &overall_rect);

  while (tmp_list)
    {
      PangoUnderline uline = PANGO_UNDERLINE_NONE;
      PangoLayoutRun *run = tmp_list->data;
      PangoAttrColor fg_color, bg_color;
      gboolean fg_set, bg_set;
      GC fg_gc;

      tmp_list = tmp_list->next;

      pango_x_get_item_properties (run->item, &uline, &fg_color, &fg_set, &bg_color, &bg_set);

      if (fg_set && info->get_gc_func)
	fg_gc = info->get_gc_func (context, &fg_color.color, gc);
      else
	fg_gc = gc;

      if (uline == PANGO_UNDERLINE_NONE)
	pango_glyph_string_extents (run->glyphs, run->item->analysis.font,
				    NULL, &logical_rect);
      else
	pango_glyph_string_extents (run->glyphs, run->item->analysis.font,
				    &ink_rect, &logical_rect);

      if (bg_set && info->get_gc_func)
	{
	  GC bg_gc = info->get_gc_func (context, &bg_color.color, gc);

	  XFillRectangle (display, drawable, bg_gc,
			  x + (x_off + logical_rect.x) / PANGO_SCALE,
			  y + overall_rect.y / PANGO_SCALE,
			  logical_rect.width / PANGO_SCALE,
			  overall_rect.height / PANGO_SCALE);

	  if (info->free_gc_func)
	    info->free_gc_func (context, bg_gc);
	}

      pango_x_render (display, drawable, fg_gc, run->item->analysis.font, run->glyphs,
		      x + x_off / PANGO_SCALE, y);

      switch (uline)
	{
	case PANGO_UNDERLINE_NONE:
	  break;
	case PANGO_UNDERLINE_DOUBLE:
	  XDrawLine (display, drawable, fg_gc,
		     x + (x_off + ink_rect.x) / PANGO_SCALE - 1, y + 4,
		     x + (x_off + ink_rect.x + ink_rect.width) / PANGO_SCALE, y + 4);
	  /* Fall through */
	case PANGO_UNDERLINE_SINGLE:
	  XDrawLine (display, drawable, fg_gc,
		     x + (x_off + ink_rect.x) / PANGO_SCALE - 1, y + 2,
		     x + (x_off + ink_rect.x + ink_rect.width) / PANGO_SCALE, y + 2);
	  break;
	case PANGO_UNDERLINE_ERROR:
	  {
	    int point_x;
	    int counter = 0;
	    int end_x = x + (x_off + ink_rect.x + ink_rect.width) / PANGO_SCALE;

	    for (point_x = x + PANGO_PIXELS (x_off + ink_rect.x) - 1;
		 point_x <= end_x;
		 point_x += 2)
	      {
		if (counter)
		  XDrawLine (display, drawable, gc,
			     point_x, y + 2, MIN (point_x + 1, end_x), y + 2);
		else
		  XDrawLine (display, drawable, gc,
			     point_x, y + 3, MIN (point_x + 1, end_x), y + 3);

		counter = (counter + 1) % 2;
	      }
	  }
	  break;
	case PANGO_UNDERLINE_LOW:
	  XDrawLine (display, drawable, fg_gc,
		     x + (x_off + ink_rect.x) / PANGO_SCALE - 1, y + (ink_rect.y + ink_rect.height) / PANGO_SCALE + 2,
		     x + (x_off + ink_rect.x + ink_rect.width) / PANGO_SCALE, y + (ink_rect.y + ink_rect.height) / PANGO_SCALE + 2);
	  break;
	}

      if (fg_set && info->get_gc_func && info->free_gc_func)
	info->free_gc_func (context, fg_gc);

      x_off += logical_rect.width;
    }
}

/**
 * pango_x_render_layout:
 * @display:   the X display.
 * @drawable:  the drawable on which to draw.
 * @gc:        GC to use for uncolored drawing.
 * @layout:    a #PangoLayout.
 * @x:         the x position of the left of the layout (in pixels).
 * @y:         the y position of the top of the layout (in pixels).
 *
 * Renders a #PangoLayout onto an X drawable.
 */
void
pango_x_render_layout (Display         *display,
		       Drawable         drawable,
		       GC               gc,
		       PangoLayout     *layout,
		       int              x,
		       int              y)
{
  PangoLayoutIter *iter;

  g_return_if_fail (display != NULL);
  g_return_if_fail (PANGO_IS_LAYOUT (layout));

  iter = pango_layout_get_iter (layout);

  do
    {
      PangoRectangle   logical_rect;
      PangoLayoutLine *line;
      int              baseline;

      line = pango_layout_iter_get_line_readonly (iter);

      pango_layout_iter_get_line_extents (iter, NULL, &logical_rect);
      baseline = pango_layout_iter_get_baseline (iter);

      pango_x_render_layout_line (display, drawable, gc,
				  line,
				  x + PANGO_PIXELS (logical_rect.x),
				  y + PANGO_PIXELS (baseline));
    }
  while (pango_layout_iter_next_line (iter));

  pango_layout_iter_free (iter);
}

/* This utility function is duplicated here and in pango-layout.c; should it be
 * public? Trouble is - what is the appropriate set of properties?
 */
static void
pango_x_get_item_properties (PangoItem      *item,
			     PangoUnderline *uline,
			     PangoAttrColor *fg_color,
			     gboolean       *fg_set,
			     PangoAttrColor *bg_color,
			     gboolean       *bg_set)
{
  GSList *tmp_list = item->analysis.extra_attrs;

  if (fg_set)
    *fg_set = FALSE;

  if (bg_set)
    *bg_set = FALSE;

  while (tmp_list)
    {
      PangoAttribute *attr = tmp_list->data;

      switch (attr->klass->type)
	{
	case PANGO_ATTR_UNDERLINE:
	  if (uline)
	    *uline = ((PangoAttrInt *)attr)->value;
	  break;

	case PANGO_ATTR_FOREGROUND:
	  if (fg_color)
	    *fg_color = *((PangoAttrColor *)attr);
	  if (fg_set)
	    *fg_set = TRUE;

	  break;

	case PANGO_ATTR_BACKGROUND:
	  if (bg_color)
	    *bg_color = *((PangoAttrColor *)attr);
	  if (bg_set)
	    *bg_set = TRUE;

	  break;

	default:
	  break;
	}
      tmp_list = tmp_list->next;
    }
}

/**
 * pango_x_apply_ligatures:
 * @font: unused
 * @subfont: unused
 * @glyphs: unused
 * @n_glyphs: unused
 * @clusters: unused
 *
 * Previously did subfont-specific ligation. Now a no-op.
 *
 * Return value: %FALSE, always.
 */
gboolean
pango_x_apply_ligatures (PangoFont     *font,
			 PangoXSubfont  subfont_id,
			 gunichar     **glyphs,
			 int           *n_glyphs,
			 int           **clusters)
{
  return FALSE;
}

/**
 * pango_x_find_first_subfont:
 * @font: A #PangoFont.
 * @rfont: A pointer to a #PangoXSubfont.
 * @charsets: An array of charsets.
 * @n_charsets: The number of charsets in @charsets.
 *
 * Looks for subfonts with the @charset charset,
 * in @font, and puts the first one in *@rfont.
 *
 * Return value: %TRUE if *@rfont now contains a font.
 */
gboolean
pango_x_find_first_subfont (PangoFont      *font,
			    char          **charsets,
			    int             n_charsets,
			    PangoXSubfont  *rfont)
{
  int n_subfonts;
  gboolean result = FALSE;
  PangoXSubfont *subfonts;
  int *subfont_charsets;

  g_return_val_if_fail (font, 0);
  g_return_val_if_fail (charsets, 0);
  g_return_val_if_fail (rfont, 0);

  n_subfonts = pango_x_list_subfonts (font, charsets, n_charsets,
				      &subfonts, &subfont_charsets);

  if (n_subfonts > 0)
    {
      *rfont = subfonts[0];
      result = TRUE;
    }

  g_free (subfonts);
  g_free (subfont_charsets);
  return result;
}

/**
 * pango_x_fallback_shape:
 * @font: A #PangoFont.
 * @glyphs: A pointer to a #PangoGlyphString.
 * @text: UTF-8 string.
 * @n_chars: Number of UTF-8 seqs in @text.
 *
 * This is a simple fallback shaper, that can be used
 * if no subfont that supports a given script is found.
 * For every character in @text, it puts the unknown glyph.
 */
void
pango_x_fallback_shape (PangoFont        *font,
			PangoGlyphString *glyphs,
			const char       *text,
			int               n_chars)
{
  PangoGlyph unknown_glyph = pango_x_get_unknown_glyph (font);
  PangoRectangle logical_rect;
  const char *p;
  int i;

  g_return_if_fail (font);
  g_return_if_fail (glyphs);
  g_return_if_fail (text);
  g_return_if_fail (n_chars >= 0);

  pango_font_get_glyph_extents (font, unknown_glyph, NULL, &logical_rect);
  pango_glyph_string_set_size (glyphs, n_chars);
  p = text;
  for (i = 0; i < n_chars; i++)
    {
      glyphs->glyphs[i].glyph = unknown_glyph;

      glyphs->glyphs[i].geometry.x_offset = 0;
      glyphs->glyphs[i].geometry.y_offset = 0;
      glyphs->glyphs[i].geometry.width = logical_rect.width;

      glyphs->log_clusters[i] = p - text;

      p = g_utf8_next_char (p);
    }
}

/**
 * pango_x_font_get_unknown_glyph:
 * @font: a #PangoFont.
 * @wc: the Unicode character for which a glyph is needed.
 *
 * Returns the index of a glyph suitable for drawing @wc as an
 * unknown character.
 *
 * Use PANGO_GET_UNKNOWN_GLYPH() instead.
 *
 * Return value: a glyph index into @font.
 */
PangoGlyph
pango_x_font_get_unknown_glyph (PangoFont *font,
				gunichar   wc)
{
  return PANGO_GET_UNKNOWN_GLYPH (wc);
}
