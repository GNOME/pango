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
typedef struct _PangoXSubfontInfo PangoXSubfontInfo;

struct _PangoXSubfontInfo {
  gchar *xlfd;
  XFontStruct *font_struct;
};

struct _PangoXFont {
  PangoFont font;
  Display *display;

  char **fonts;
  int n_fonts;

  /* hash table mapping from charset-name to array of PangoXSubfont ids,
   * of length n_fonts
   */
  GHashTable *subfonts_by_charset;
  
  PangoXSubfontInfo **subfonts;

  gint n_subfonts;
  gint max_subfonts;
};

static PangoXSubfontInfo * pango_x_find_subfont    (PangoFont          *font,
						    PangoXSubfont       subfont_index);
static XCharStruct *       pango_x_get_per_char    (PangoFont          *font,
						    PangoXSubfontInfo  *subfont,
						    guint16             char_index);
static void                pango_x_font_destroy    (PangoFont          *font);
static gboolean            pango_x_find_glyph      (PangoFont          *font,
						    PangoGlyph          glyph,
						    PangoXSubfontInfo **subfont_return,
						    XCharStruct       **charstruct_return);
static XFontStruct *       pango_x_get_font_struct (PangoFont          *font,
						    PangoXSubfontInfo  *info);

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

  for (result->n_fonts = 0; result->fonts[result->n_fonts]; result->n_fonts++)
    /* Nothing */

  result->subfonts_by_charset = g_hash_table_new (g_str_hash, g_str_equal);
  result->subfonts = g_new (PangoXSubfontInfo *, 1);

  result->n_subfonts = 0;
  result->max_subfonts = 1;

  return (PangoFont *)result;
}
 
/**
 * pango_x_render:
 * @display: the X display
 * @d:       the drawable on which to draw string
 * @gc:      the graphics context
 * @font:    the font in which to draw the string
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
  Font old_fid = None;
  XFontStruct *fs;
  int i;

  g_return_if_fail (display != NULL);
  g_return_if_fail (glyphs != NULL);
  
  for (i=0; i<glyphs->num_glyphs; i++)
    {
      guint16 index = PANGO_X_GLYPH_INDEX (glyphs->glyphs[i].glyph);
      guint16 subfont_index = PANGO_X_GLYPH_SUBFONT (glyphs->glyphs[i].glyph);
      PangoXSubfontInfo *subfont;
      
      XChar2b c;

      subfont = pango_x_find_subfont (font, subfont_index);
      if (subfont)
	{
	  c.byte1 = index / 256;
	  c.byte2 = index % 256;

	  fs = pango_x_get_font_struct (font, subfont);
	  if (!fs)
	    continue;
	  
	  if (fs->fid != old_fid)
	    {
	      XSetFont (display, gc, fs->fid);
	      old_fid = fs->fid;
	    }
	  
	  XDrawString16 (display, d, gc,
			 x + glyphs->glyphs[i].geometry.x_offset / 72,
			 y + glyphs->glyphs[i].geometry.y_offset / 72,
			 &c, 1);
	}

      x += glyphs->glyphs[i].geometry.width / 72;
    }
}

/**
 * pango_x_glyph_extents:
 * @font:     a #PangoFont
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
		       PangoGlyph       glyph,
		       gint            *lbearing, 
		       gint            *rbearing,
		       gint            *width, 
		       gint            *ascent, 
		       gint            *descent,
		       gint            *logical_ascent,
		       gint            *logical_descent)
{
  XCharStruct *cs;
  PangoXSubfontInfo *subfont;

  if (pango_x_find_glyph (font, glyph, &subfont, &cs))
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
	*logical_ascent = subfont->font_struct->ascent;
      if (logical_descent)
	*logical_descent = subfont->font_struct->descent;
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
 * @font:     a #PangoFont
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
  PangoXSubfontInfo *subfont;
  XCharStruct *cs;

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
      PangoGlyphGeometry *geometry = &glyphs->glyphs[i].geometry;
      
      if (pango_x_find_glyph (font, glyphs->glyphs[i].glyph, &subfont, &cs))
	{
	  if (i == 0)
	    {
	      t_lbearing = cs->lbearing - geometry->x_offset / 72;
	      t_rbearing = cs->rbearing + geometry->x_offset / 72;
	      t_ascent = cs->ascent + geometry->y_offset / 72;
	      t_descent = cs->descent - geometry->y_offset / 72;
	      
	      t_logical_ascent = subfont->font_struct->ascent + geometry->y_offset / 72;
	      t_logical_descent = subfont->font_struct->descent - geometry->y_offset / 72;
	    }
	  else
	    {
	      t_lbearing = MAX (t_lbearing,
				cs->lbearing - geometry->x_offset / 72 - t_width);
	      t_rbearing = MAX (t_rbearing,
				t_width + cs->rbearing + geometry->x_offset / 72);
	      t_ascent = MAX (t_ascent, cs->ascent + geometry->y_offset / 72);
	      t_descent = MAX (t_descent, cs->descent - geometry->y_offset / 72);
	      t_logical_ascent = MAX (t_logical_ascent, subfont->font_struct->ascent + geometry->y_offset / 72);
	      t_logical_descent = MAX (t_logical_descent, subfont->font_struct->descent - geometry->y_offset / 72);
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
 * pango_x_list_subfonts:
 * @font: a PangoFont
 * @charsets: the charsets to list subfonts for.
 * @n_charsets: the number of charsets in @charsets
 * @subfont_ids: location to store a pointer to an array of subfont IDs for each found subfont
 *               the result must be freed using g_free()
 * @subfont_charsets: location to store a pointer to an array of subfont IDs for each found subfont
 *               the result must be freed using g_free()
 * 
 * Returns number of charsets found
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
  int i, j;
  int n_subfonts = 0;

  g_return_val_if_fail (font != NULL, 0);
  g_return_val_if_fail (n_charsets == 0 || charsets != NULL, 0);

  subfont_lists = g_new (PangoXSubfont *, n_charsets);

  for (j=0; j<n_charsets; j++)
    {
      subfont_lists[j] = g_hash_table_lookup (xfont->subfonts_by_charset, charsets[j]);
      if (!subfont_lists[j])
	{
	  subfont_lists[j] = g_new (PangoXSubfont, xfont->n_fonts);
	  
	  for (i = 0; i < xfont->n_fonts; i++)
	    {
	      PangoXSubfontInfo *info = NULL;
	      char *xlfd = name_for_charset (xfont->fonts[i], charsets[j]);
	      
	      if (xlfd)
		{
		  int count;
		  char **names = XListFonts (xfont->display, xlfd, 1, &count);
		  if (count > 0)
		    {
		      info = g_new (PangoXSubfontInfo, 1);
		      
		      info->xlfd = g_strdup (names[0]);
		      info->font_struct = NULL;
		    }

		  g_free (xlfd);
		}
	      
	      if (info)
		{
		  xfont->n_subfonts++;
		  
		  if (xfont->n_subfonts > xfont->max_subfonts)
		    {
		      xfont->max_subfonts *= 2;
		      xfont->subfonts = g_renew (PangoXSubfontInfo *, xfont->subfonts, xfont->max_subfonts);
		    }
		  
		  xfont->subfonts[xfont->n_subfonts - 1] = info;
		  subfont_lists[j][i] = xfont->n_subfonts;
		}
	      else
		subfont_lists[j][i] = 0;
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

  for (i=0; i<xfont->n_fonts; i++)
    for (j=0; j<n_charsets; j++)
      if (subfont_lists[j][i])
	{
	  (*subfont_ids)[n_subfonts] = subfont_lists[j][i];
	  (*subfont_charsets)[n_subfonts] = j;
	  n_subfonts++;
	}

  g_free (subfont_lists);

  return n_subfonts;
}

gboolean
pango_x_has_glyph (PangoFont       *font,
		   PangoGlyph  glyph)
{
  g_return_val_if_fail (font != NULL, FALSE);

  return pango_x_find_glyph (font, glyph, NULL, NULL);
}

static void
subfonts_foreach (gpointer key, gpointer value, gpointer data)
{
  g_free (key);
  g_free (value);
}

static void
pango_x_font_destroy (PangoFont *font)
{
  PangoXFont *xfont = (PangoXFont *)font;
  int i;

  g_hash_table_destroy (xfont->subfonts_by_charset);

  for (i=0; i<xfont->n_subfonts; i++)
    {
      PangoXSubfontInfo *info = xfont->subfonts[i];

      g_free (info->xlfd);
      if (info->font_struct)
	XFreeFont (xfont->display, info->font_struct);

      g_free (info);
    }

  g_free (xfont->subfonts);

  g_hash_table_foreach (xfont->subfonts_by_charset, subfonts_foreach, NULL);

  g_strfreev (xfont->fonts);
  g_free (font);
}

/* Utility functions */

static XFontStruct *
pango_x_get_font_struct (PangoFont *font, PangoXSubfontInfo *info)
{
  PangoXFont *xfont = (PangoXFont *)font;
  
  if (!info->font_struct)
    {
      info->font_struct = XLoadQueryFont (xfont->display, info->xlfd);
      if (!info->font_struct)
	g_warning ("Cannot load font for XLFD '%s\n", info->xlfd);
    }
  
  return info->font_struct;
}

static PangoXSubfontInfo *
pango_x_find_subfont (PangoFont  *font,
		      PangoXSubfont subfont_index)
{
  PangoXFont *xfont = (PangoXFont *)font;
  
  if (subfont_index < 1 || subfont_index > xfont->n_subfonts)
    {
      g_warning ("Invalid subfont %d", subfont_index);
      return NULL;
    }
  
  return xfont->subfonts[subfont_index-1];
}

static XCharStruct *
pango_x_get_per_char (PangoFont         *font,
		      PangoXSubfontInfo *subfont,
		      guint16            char_index)
{
  XFontStruct *fs;

  int index;
  guint8 byte1;
  guint8 byte2;

  byte1 = char_index / 256;
  byte2 = char_index % 256;

  fs = pango_x_get_font_struct (font, subfont);
  if (!fs)
    return NULL;
	  
  if ((fs->min_byte1 == 0) && (fs->max_byte1 == 0))
    {
      if (byte2 < fs->min_char_or_byte2 || byte2 > fs->max_char_or_byte2)
	return NULL;
      
      index = byte2 - fs->min_char_or_byte2;
    }
  else
    {
      if (byte1 < fs->min_byte1 || byte1 > fs->max_byte1 ||
	  byte2 < fs->min_char_or_byte2 || byte2 > fs->max_char_or_byte2)
	return FALSE;
      
      index = ((byte1 - fs->min_byte1) *
	       (fs->max_char_or_byte2 - fs->min_char_or_byte2 + 1)) +
	byte2 - fs->min_char_or_byte2;
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

  if (cs && (cs->lbearing != cs->rbearing))
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
