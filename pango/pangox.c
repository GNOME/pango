/* Pango
 * gscriptx.c: Routines for handling X fonts
 *
 * Copyright (C) 1999 Red Hat Software
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

#include <X11/Xlib.h>
#include "pangox.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

typedef struct _PangoXFont PangoXFont;
typedef struct _PangoXCFont PangoXCFont;
typedef struct _PangoXCharsetInfo PangoXCharsetInfo;

struct _PangoXCFont {
  gchar *xlfd;
  XFontStruct *font_struct;
};

struct _PangoXCharsetInfo {
  PangoXCharset charset;
  gchar *name;
  GSList *cfonts;
};

struct _PangoXFont {
  PangoFont font;
  Display *display;

  char **fonts;
  
  GHashTable *charsets_by_name;
  PangoXCharsetInfo **charsets;

  gint n_charsets;
  gint max_charsets;
};

static XCharStruct *pango_x_get_per_char  (PangoXCFont   *cfont,
					   guint16        glyph_index);
static PangoXCFont *pango_x_find_cfont    (PangoXFont    *font,
					   PangoXCharset  charset,
					   guint16        index,
					   XCharStruct  **per_char_return);
static void         pango_x_font_destroy  (PangoFont     *font);

PangoFontClass pango_x_font_class = {
  pango_x_font_destroy
};

/**
 * pango_x_load_font:
 * @display: the X display
 * @spec:    a comma-separated list of XLFD's
 *
 * Load up a logical font based on a "fontset" style
 * text specification.
 *
 * Returns a new #PangoFont
 */
PangoFont *
pango_x_load_font (Display *display,
		   gchar    *spec)
{
  PangoXFont *result;

  g_return_val_if_fail (display != NULL, NULL);
  g_return_val_if_fail (spec != NULL, NULL);
  
  result = g_new (PangoXFont, 1);

  result->display = display;

  pango_font_init (&result->font);
  result->font.klass = &pango_x_font_class;

  result->fonts = g_strsplit(spec, ",", -1);

  result->charsets_by_name = g_hash_table_new (g_str_hash, g_str_equal);
  result->charsets = g_new (PangoXCharsetInfo *, 1);

  result->n_charsets = 0;
  result->max_charsets = 1;

  return (PangoFont *)result;
}
 
/**
 * pango_x_render:
 * @display: the X display
 * @d:       the drawable on which to draw string
 * @gc:      the graphics context
 * @glyphs:  the glyph string to draw
 * @x:       the x position of start of string
 * @y:       the y position of baseline
 *
 * Render a PangoGlyphString onto an X drawable
 */
void 
pango_x_render  (Display           *display,
		 Drawable           d,
		 GC                 gc,
		 PangoFont         *font,
		 PangoGlyphString  *glyphs,
		 gint               x, 
		 gint               y)
{
  /* Slow initial implementation. For speed, it should really
   * collect the characters into runs, and draw multiple
   * characters with each XDrawString16 call.
   */
  PangoXFont *xfont = (PangoXFont *)font;
  Font old_fid = None;
  XFontStruct *fs;
  int i;

  g_return_if_fail (display != NULL);
  g_return_if_fail (glyphs != NULL);
  
  for (i=0; i<glyphs->num_glyphs; i++)
    {
      guint16 index = PANGO_X_GLYPH_INDEX (glyphs->glyphs[i].glyph);
      guint16 charset = PANGO_X_GLYPH_CHARSET (glyphs->glyphs[i].glyph);
      PangoXCFont *cfont;
      
      XChar2b c;

      if (charset < 1 || charset > xfont->n_charsets)
	{
	  g_warning ("Glyph string contains invalid charset %d", charset);
	  continue;
	}

      cfont = pango_x_find_cfont (xfont, charset, index, NULL);
      
      if (cfont)
	{
	  c.byte1 = index / 256;
	  c.byte2 = index % 256;

	  fs = cfont->font_struct;
	  
	  if (fs->fid != old_fid)
	    {
	      XSetFont (display, gc, fs->fid);
	      old_fid = fs->fid;
	    }
	  
	  XDrawString16 (display, d, gc,
			 x + glyphs->geometry[i].x_offset / 72,
			 y + glyphs->geometry[i].y_offset / 72,
			 &c, 1);

	  x += glyphs->geometry[i].width / 72;
	}
    }
}

/**
 * pango_x_glyph_extents:
 * @glyph:    the glyph to measure
 * @lbearing: left bearing of glyph (result)
 * @rbearing: right bearing of glyph (result)
 * @width:    width of glyph (result)
 * @ascent:   ascent of glyph (result)
 * @descent:  descent of glyph (result)
 * @logical_ascent: The vertical distance from the baseline to the
 *                  bottom of the line above.
 * @logical_descent: The vertical distance from the baseline to the
 *                   top of the line below.
 *
 * Compute the measurements of a single glyph in pixels.
 */
void 
pango_x_glyph_extents (PangoFont       *font,
		       PangoGlyphIndex  glyph,
		       gint            *lbearing, 
		       gint            *rbearing,
		       gint            *width, 
		       gint            *ascent, 
		       gint            *descent,
		       gint            *logical_ascent,
		       gint            *logical_descent)
{
  PangoXFont *xfont = (PangoXFont *)font;
  PangoXCFont *cfont;

  XCharStruct *cs;

  guint16 index = PANGO_X_GLYPH_INDEX (glyph);
  guint16 charset = PANGO_X_GLYPH_CHARSET (glyph);
      
  if (charset < 1 || charset > xfont->n_charsets)
    {
      g_warning ("Glyph string contains invalid charset %d\n", charset);
      return;
    }

  cfont = pango_x_find_cfont (xfont, charset, index, &cs);

  if (cfont)
    {
      if (lbearing)
	*lbearing = cs->lbearing;
      if (rbearing)
	*rbearing = cs->rbearing;
      if (width)
	*width = cs->width;
      if (ascent)
	*ascent = cs->ascent;
      if (descent)
	*descent = cs->descent;
      if (logical_ascent)
	*logical_ascent = cfont->font_struct->ascent;
      if (logical_descent)
	*logical_descent = cfont->font_struct->descent;
    }
  else
    {
      if (lbearing)
	*lbearing = 0;
      if (rbearing)
	*rbearing = 0;
      if (width)
	*width = 0;
      if (ascent)
	*ascent = 0;
      if (descent)
	*descent = 0;
      if (logical_ascent)
	*logical_ascent = 0;
      if (logical_descent)
	*logical_descent = 0;
    }
}

/**
 * pango_x_extents:
 * @glyphs:   the glyph string to measure
 * @lbearing: left bearing of string (result)
 * @rbearing: right bearing of string (result)
 * @width:    width of string (result)
 * @ascent:   ascent of string (result)
 * @descent:  descent of string (result)
 * @logical_ascent: The vertical distance from the baseline to the
 *                  bottom of the line above.
 * @logical_descent: The vertical distance from the baseline to the
 *                   top of the line below.
 *
 * Compute the measurements of a glyph string in pixels.
 * The number of parameters here is clunky - it might be
 * nicer to use structures as in XmbTextExtents.
 */
void 
pango_x_extents (PangoFont        *font,
		 PangoGlyphString *glyphs,
		 gint             *lbearing, 
		 gint             *rbearing,
		 gint             *width, 
		 gint             *ascent, 
		 gint             *descent,
		 gint             *logical_ascent,
		 gint             *logical_descent)
{
  PangoXFont *xfont = (PangoXFont *)font;

  int i;

  int t_lbearing = 0;
  int t_rbearing = 0;
  int t_ascent = 0;
  int t_descent = 0;
  int t_logical_ascent = 0;
  int t_logical_descent = 0;
  int t_width = 0;

  g_return_if_fail (font != NULL);
  g_return_if_fail (glyphs != NULL);
  
  for (i=0; i<glyphs->num_glyphs; i++)
    {
      XCharStruct *cs;
      
      guint16 index = PANGO_X_GLYPH_INDEX (glyphs->glyphs[i].glyph);
      guint16 charset = PANGO_X_GLYPH_CHARSET (glyphs->glyphs[i].glyph);
      
      PangoGlyphGeometry *geometry = &glyphs->geometry[i];
      PangoXCFont *cfont = pango_x_find_cfont (xfont, charset, index, &cs);

      if (cfont)
	{
	  if (i == 0)
	    {
	      t_lbearing = cs->lbearing - geometry->x_offset / 72;
	      t_rbearing = cs->rbearing + geometry->x_offset / 72;
	      t_ascent = cs->ascent + geometry->y_offset / 72;
	      t_descent = cs->descent - geometry->y_offset / 72;

	      t_logical_ascent = cfont->font_struct->ascent + geometry->y_offset / 72;
	      t_logical_descent = cfont->font_struct->descent - geometry->y_offset / 72;
	    }
	  else
	    {
	      t_lbearing = MAX (t_lbearing,
				cs->lbearing - geometry->x_offset / 72 - t_width);
	      t_rbearing = MAX (t_rbearing,
				t_width + cs->rbearing + geometry->x_offset / 72);
	      t_ascent = MAX (t_ascent, cs->ascent + geometry->y_offset / 72);
	      t_descent = MAX (t_descent, cs->descent - geometry->y_offset / 72);
	      t_logical_ascent = MAX (t_logical_ascent, cfont->font_struct->ascent + geometry->y_offset / 72);
	      t_logical_descent = MAX (t_logical_descent, cfont->font_struct->descent - geometry->y_offset / 72);
	    }
	}
      
      t_width += geometry->width / 72;
    }

  if (lbearing)
    *lbearing = t_lbearing;
  if (rbearing)
    *rbearing = t_rbearing;
  if (width)
    *width = t_width;
  if (ascent)
    *ascent = t_ascent;
  if (descent)
    *descent = t_descent;
  if (logical_ascent)
    *logical_ascent = t_logical_ascent;
  if (logical_descent)
    *logical_descent = t_logical_descent;
}

/* Compare the tail of a to b */
static gboolean
match_end (char *a, char *b)
{
  size_t len_a = strlen (a);
  size_t len_b = strlen (b);

  if (len_b > len_a)
    return FALSE;
  else
    return (strcmp (a + len_a - len_b, b) == 0);
}

/* Substitute in a charset into an XLFD. Return the
 * (g_malloc'd) new name, or NULL if the XLFD cannot
 * match the charset
 */
static gchar *
name_for_charset (char *xlfd, char *charset)
{
  char *p;
  char *dash_charset = g_strconcat ("-", charset, NULL);
  char *result = NULL;
  gint ndashes = 0;

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

/**
 * pango_x_find_charset:
 * @font: a #PangoFont
 * @charset: string name of charset
 * 
 * Look up the character set ID for a character set in the given font.
 * If a character set ID has not yet been assigned, a new ID will be assigned.
 * 
 * Return value: Character ID for character set, if there is a match in this
 *               font, otherwise 0.
 **/
PangoXCharset
pango_x_find_charset (PangoFont *font,
		      gchar     *charset)
{
  PangoXFont *xfont = (PangoXFont *)font;
  PangoXCharsetInfo *info;
  int i;

  g_return_val_if_fail (font != NULL, 0);
  g_return_val_if_fail (charset != NULL, 0);

  info = g_hash_table_lookup (xfont->charsets_by_name, charset);

  if (!info)
    {
      info = g_new (PangoXCharsetInfo, 1);

      info->cfonts = NULL;

      for (i = 0; xfont->fonts[i]; i++)
	{
	  char *xlfd = name_for_charset (xfont->fonts[i], charset);

	  if (xlfd)
	    {
	      int count;
	      char **names = XListFonts (xfont->display, xlfd, 1, &count);
	      if (count > 0)
		{
		  PangoXCFont *cfont = g_new (PangoXCFont, 1);

		  cfont->xlfd = g_strdup (names[0]);
		  cfont->font_struct = NULL;

		  info->cfonts = g_slist_append (info->cfonts, cfont);
		}

	      XFreeFontNames (names);
	      g_free (xlfd);
	    }
	}

      if (info->cfonts)
	{
	  info->name = g_strdup (charset);
	  g_hash_table_insert (xfont->charsets_by_name, info->name, info);

	  xfont->n_charsets++;

	  if (xfont->n_charsets > xfont->max_charsets)
	    {
	      xfont->max_charsets *= 2;
	      xfont->charsets = g_renew (PangoXCharsetInfo *, xfont->charsets, xfont->max_charsets);
	    }
	  
	  info->charset = xfont->n_charsets;
	  xfont->charsets[xfont->n_charsets - 1] = info;
	}
      else
	{
	  g_free (info);
	  info = NULL;
	}
    }

  if (info)
    return info->charset;
  else
    return 0;
}

gboolean
pango_x_has_glyph (PangoFont       *font,
		   PangoGlyphIndex  glyph)
{
  PangoXFont *xfont = (PangoXFont *)font;
  
  guint16 index, charset;
  
  g_return_val_if_fail (font != NULL, FALSE);
  
  index = PANGO_X_GLYPH_INDEX (glyph);
  charset = PANGO_X_GLYPH_CHARSET (glyph);

  if (charset < 1 || charset > xfont->n_charsets)
    {
      g_warning ("Glyph string contains invalid charset %d", charset);
      return FALSE;
    }
  
  return (pango_x_find_cfont (xfont, charset, index, NULL) != NULL);
}

static void
pango_x_font_destroy (PangoFont *font)
{
  PangoXFont *xfont = (PangoXFont *)font;
  int i;

  g_hash_table_destroy (xfont->charsets_by_name);

  for (i=0; i<xfont->n_charsets; i++)
    {
      GSList *cfonts = xfont->charsets[i]->cfonts;

      while (cfonts)
	{
	  PangoXCFont *cfont = cfonts->data;

	  g_free (cfont->xlfd);
	  if (cfont->font_struct)
	    XFreeFont (xfont->display, cfont->font_struct);

	  g_free (cfont);
	  
	  cfonts = cfonts->next;
	}

      g_slist_free (xfont->charsets[i]->cfonts);

      g_free (xfont->charsets[i]->name);
      g_free (xfont->charsets[i]);
    }

  g_free (xfont->charsets);

  g_strfreev (xfont->fonts);
  g_free (font);
}

/* Utility functions */

static XCharStruct *
pango_x_get_per_char (PangoXCFont *cfont, guint16 glyph_index)
{
  guint8 byte1;
  guint8 byte2;
  
  XFontStruct *fs = cfont->font_struct;
  int index;

  byte1 = glyph_index / 256;
  byte2 = glyph_index % 256;
  
  if ((fs->min_byte1 == 0) && (fs->min_byte1 == 0))
    {
      if (byte2 < fs->min_char_or_byte2 || byte2 > fs->max_char_or_byte2)
	return NULL;
      
      index = byte2 - fs->min_char_or_byte2;
    }
  else
    {
      if (byte1 < fs->min_byte1 || byte1 > fs->max_byte1 ||
	  byte2 < fs->min_char_or_byte2 || byte2 > fs->max_char_or_byte2)
	return NULL;
      
      index = ((byte1 - fs->min_byte1) *
	       (fs->max_char_or_byte2 - fs->min_char_or_byte2 + 1)) +
	byte2 - fs->min_char_or_byte2;
    }
  
  if (fs->per_char)
    return &fs->per_char[index];
  else
    return &fs->min_bounds;
}

static PangoXCFont *
pango_x_find_cfont (PangoXFont *font, PangoXCharset charset, guint16 index, XCharStruct **per_char_return)
{
  GSList *cfonts = font->charsets[charset - 1]->cfonts;
  
  while (cfonts)
    {
      XCharStruct *per_char;
      PangoXCFont *cfont = cfonts->data;
      
      if (!cfont->font_struct)
	{
	  cfont->font_struct = XLoadQueryFont (font->display, cfont->xlfd);
	  if (!cfont->font_struct)
	    g_error ("Error loading font '%s'\n", cfont->xlfd);
	}

      per_char = pango_x_get_per_char (cfont, index);

      if (per_char && per_char->width != 0)
	{
	  if (per_char_return)
	    *per_char_return = per_char;
	  
	  return cfont;
	}
	  
      cfonts = cfonts->next;
    }

  if (per_char_return)
    *per_char_return = NULL;
  
  return NULL;
}
      
