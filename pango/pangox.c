/* Pango
 * gscriptx.c:
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
typedef struct _XLFDInfo XLFDInfo;

struct _PangoXFont {
  PangoFont font;
  Display *display;
  gchar **fonts;
  gint n_fonts;
  GHashTable *name_hash;
  GHashTable *xlfd_hash;
};

struct _XLFDInfo {
  XFontStruct *font_struct;
  PangoCFont *cfont;
};

static void pango_x_font_destroy (PangoFont *font);
static void pango_x_cfont_destroy (PangoCFont *cfont);

PangoFontClass pango_x_font_class = {
  pango_x_font_destroy
};

PangoCFontClass pango_x_cfont_class = {
  pango_x_cfont_destroy
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
  result->n_fonts = 0;
  while (result->fonts[result->n_fonts])
    result->n_fonts++;
  
  result->name_hash = g_hash_table_new (g_str_hash, g_str_equal);
  result->xlfd_hash = g_hash_table_new (g_str_hash, g_str_equal);

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
pango_x_render  (Display       *display,
		    Drawable       d,
		    GC             gc,
		    PangoGlyphString  *glyphs,
		    gint           x, 
		    gint           y)
{
  /* Slow initial implementation. For speed, it should really
   * collect the characters into runs, and draw multiple
   * characters with each XDrawString16 call.
   */
  XChar2b c;
  PangoXCFont *cfont;
  Font old_fid = None;
  XFontStruct *fs;
  int i;

  for (i=0; i<glyphs->num_glyphs; i++)
    {
      c.byte1 = glyphs->glyphs[i].glyph / 256;
      c.byte2 = glyphs->glyphs[i].glyph % 256;
      cfont = (PangoXCFont *)glyphs->glyphs[i].font;
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
pango_x_glyph_extents (PangoGlyph        *glyph,
			  gint          *lbearing, 
			  gint          *rbearing,
			  gint          *width, 
			  gint          *ascent, 
			  gint          *descent,
			  gint          *logical_ascent,
			  gint          *logical_descent)
{
  int index;
  
  PangoXCFont *cfont;
  XFontStruct *fs;
  XCharStruct *cs;
  XChar2b c;
  
  c.byte1 = glyph->glyph / 256;
  c.byte2 = glyph->glyph % 256;
  cfont = (PangoXCFont *)glyph->font;
  fs = cfont->font_struct;
  
  if ((fs->min_byte1 == 0) && (fs->max_byte1 == 0))
    index = c.byte2 - fs->min_char_or_byte2;
  else
    {
      index = ((c.byte1 - fs->min_byte1) *
	       (fs->max_char_or_byte2 - fs->min_char_or_byte2 + 1)) +
	c.byte2 - fs->min_char_or_byte2;
    }
  
  if (fs->per_char)
    cs = &fs->per_char[index];
  else
    cs = &fs->min_bounds;

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
    *logical_ascent = fs->ascent;
  if (logical_descent)
    *logical_descent = fs->descent;
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
pango_x_extents (PangoGlyphString *glyphs,
		    gint          *lbearing, 
		    gint          *rbearing,
		    gint          *width, 
		    gint          *ascent, 
		    gint          *descent,
		    gint          *logical_ascent,
		    gint          *logical_descent)
{
  int index;
  
  PangoXCFont *cfont;
  XFontStruct *fs;
  XCharStruct *cs;
  PangoGlyphGeometry *geometry;
  XChar2b c;
  
  int i;

  int t_lbearing = 0;
  int t_rbearing = 0;
  int t_ascent = 0;
  int t_descent = 0;
  int t_logical_ascent = 0;
  int t_logical_descent = 0;
  int t_width = 0;

  for (i=0; i<glyphs->num_glyphs; i++)
    {
      c.byte1 = glyphs->glyphs[i].glyph / 256;
      c.byte2 = glyphs->glyphs[i].glyph % 256;
      cfont = (PangoXCFont *)glyphs->glyphs[i].font;
      fs = cfont->font_struct;
      
      if ((fs->min_byte1 == 0) && (fs->min_byte1 == 0))
	index = c.byte2 - fs->min_char_or_byte2;
      else
	{
	  index = ((c.byte1 - fs->min_byte1) *
		   (fs->max_char_or_byte2 - fs->min_char_or_byte2 + 1)) +
	    c.byte2 - fs->min_char_or_byte2;
	}

      if (fs->per_char)
	cs = &fs->per_char[index];
      else
	cs = &fs->min_bounds;

      geometry = &glyphs->geometry[i];

      if (i == 0)
	{
	  t_lbearing = cs->lbearing - geometry->x_offset / 72;
	  t_rbearing = cs->rbearing + geometry->x_offset / 72;
	  t_ascent = cs->ascent + geometry->y_offset / 72;
	  t_descent = cs->descent - geometry->y_offset / 72;
	  t_logical_ascent = fs->ascent + geometry->y_offset / 72;
	  t_logical_descent = fs->descent - geometry->y_offset / 72;
	}
      else
	{
	  t_lbearing = MAX (t_lbearing,
			    cs->lbearing - geometry->x_offset / 72 - t_width);
	  t_rbearing = MAX (t_rbearing,
			    t_width + cs->rbearing + geometry->x_offset / 72);
	  t_ascent = MAX (t_ascent, cs->ascent + geometry->y_offset / 72);
	  t_descent = MAX (t_descent, cs->descent - geometry->y_offset / 72);
	  t_logical_ascent = MAX (t_logical_ascent, fs->ascent + geometry->y_offset / 72);
	  t_logical_descent = MAX (t_logical_descent, fs->descent - geometry->y_offset / 72);
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
 * pango_x_load_xlfd:
 * @font:    a #PangoFont
 * @xlfd:    the XLFD of a component font to load
 *
 * Create a component font from a XLFD. It is assumed that
 * the xlfd matches a font matching one of the names
 * of @font, but this is not currently required.
 * 
 * Returns the new #PangoXCFont
 */
PangoCFont *
pango_x_load_xlfd (PangoFont *font,
		      gchar       *xlfd)
{
  XFontStruct *fs;
  PangoXFont *xfont = (PangoXFont *)font;
  XLFDInfo *info;

  g_return_val_if_fail (font != NULL, NULL);

  info = g_hash_table_lookup (xfont->xlfd_hash, xlfd);
  if (!info)
    {
      info = g_new (XLFDInfo, 1);
      info->cfont = NULL;
      info->font_struct = NULL;
      
      g_hash_table_insert (xfont->xlfd_hash, g_strdup(xlfd), info);
    }

  if (!info->cfont)
    {
      fs = XLoadQueryFont (xfont->display, xlfd);
      if (fs)
	{
	  PangoXCFont *cfont = g_new (PangoXCFont, 1);
	  cfont->display = xfont->display;
	  cfont->font_struct = fs;
	  cfont->font.klass = &pango_x_cfont_class;

	  info->cfont = (PangoCFont *)cfont;
	  
	  pango_cfont_init (info->cfont);
	  pango_cfont_ref (info->cfont);

	  if (info->font_struct)
	    XFreeFontInfo (NULL, info->font_struct, 1);

	  info->font_struct = fs;
	}
    }

  return info->cfont;
}

static gchar **
find_cfonts (PangoFont *font, gchar *charset)
{
  PangoXFont *xfont = (PangoXFont *)font;
  gchar **cfonts;
  int i;
  
  cfonts = g_hash_table_lookup (xfont->name_hash, charset);
  if (!cfonts)
    {
      cfonts = g_new (gchar *, xfont->n_fonts + 1);
      for (i=0; i<xfont->n_fonts; i++)
	{
	  char *xlfd = name_for_charset (xfont->fonts[i], charset);
	  gchar **names;
	  gint count;

	  cfonts[i] = NULL;
	  if (xlfd)
	    {
	      names = XListFonts (xfont->display, xlfd, 1, &count);
	      if (count > 0)
		cfonts[i] = g_strdup (names[0]);

	      XFreeFontNames (names);
	      g_free (xlfd);
	    }
	}

      g_hash_table_insert (xfont->name_hash, g_strdup(charset), cfonts);
    }

  return cfonts;
}

/**
 * pango_x_find_cfont:
 * @font:     A font from pango_x_load_font()
 * @charset:  characterset descript (last two components of XLFD)
 *
 * Find a component font of a #PangoFont.
 *
 * Returns the #PangoCFont for @charset, or NULL, if no appropriate
 *   font could be found.
 */
PangoCFont *
pango_x_find_cfont (PangoFont *font,
		       gchar       *charset)
{
  PangoXFont *xfont = (PangoXFont *)font;
  gchar **names;
  int i;

  names = find_cfonts (font, charset);
  for (i=0; i<xfont->n_fonts; i++)
    if (names[i])
      return pango_x_load_xlfd (font, names[i]);

  return NULL;
}

void
font_struct_get_ranges (Display     *display,
			XFontStruct *fs,
			gint       **ranges,
			gint        *n_ranges)
{
  gint i, j;
  static Atom bounds_atom = None;
  gint *range_buf = NULL;
  size_t range_buf_size = 8;

  if (bounds_atom == None)
    bounds_atom = XInternAtom (display, "_XFREE86_GLYPH_RANGES", False);

  j = 0;
  for (i=0; i<fs->n_properties; i++)
    {
      if (fs->properties[i].name == bounds_atom)
	{
	  char *val = XGetAtomName (display, fs->properties[i].card32);
	  char *p;
	  guint start, end;

	  p = val;
	  while (*p)
	    {
	      int count;
	      
	      while (*p && isspace (*p))
		p++;

	      count = sscanf (p, "%u_%u", &start, &end);
	      
	      if (count > 0)
		{
		  if (count == 1)
		    end = start;
		    
		  if (!range_buf || (2*j+1) >= range_buf_size)
		    {
		      size_t new_size = range_buf_size * 2;
		      if (new_size < range_buf_size) /* Paranoia */
			{
			  XFree (val);
			  *ranges = NULL;
			  *n_ranges = 0;
			  
			  return;
			}
		      range_buf_size = new_size;
		      range_buf = g_realloc (range_buf, sizeof(gint) * range_buf_size);
		    }

		  range_buf[2*j] = start;
		  range_buf[2*j + 1] = end;
		  j++;
		}
	      else
		{
		  goto error;
		}

	      while (*p && !isspace (*p))
		p++;
	    }

	error:
	  XFree (val);
	}
      
    }

  if (j > 0)
    {
      *n_ranges = j;
      *ranges = g_malloc (sizeof(gint) * 2*j);
      memcpy (*ranges, range_buf, sizeof(gint) * 2*j);
    }
  else
    {
      *n_ranges = 1;
      *ranges = g_malloc (sizeof(gint) * 2);

      (*ranges)[0] = fs->min_byte1 * 256 + fs->min_char_or_byte2;
      (*ranges)[1] = fs->max_byte1 * 256 + fs->max_char_or_byte2;
    }

  g_free (range_buf);
}

/**
 * pango_x_xlfd_get_ranges:
 * @font:     a #PangoFont.
 * @xlfd:     a XLFD of a component font.
 * @ranges:   location to store returned ranges.
 * @n_ranges: location to store the number of ranges.
 *
 * Find the range of valid characters for a particular
 * XLFD representing a component of the given font.
 *
 * Returns %TRUE if the XLFD matches a font, FALSE otherwise.
 * in the latter case, @ranges and @n_ranges are unchanged.
 */
gboolean
pango_x_xlfd_get_ranges (PangoFont *font,
			    gchar       *xlfd,
			    gint       **ranges,
			    gint        *n_ranges)
{
  PangoXFont *xfont = (PangoXFont *)font;
  gchar **names;
  gint count;
  XLFDInfo *info;

  info = g_hash_table_lookup (xfont->xlfd_hash, xlfd);
  if (!info)
    {
      info = g_new (XLFDInfo, 1);
      info->cfont = NULL;
      info->font_struct = NULL;
      g_hash_table_insert (xfont->xlfd_hash, g_strdup(xlfd), info);
    }

  if (!info->font_struct)
    {
      names = XListFontsWithInfo (xfont->display, xlfd, 1, &count, &info->font_struct);

      if (count == 0)
	info->font_struct = NULL;

      XFreeFontNames (names);
    }

  if (info->font_struct)
    {
      font_struct_get_ranges (xfont->display, info->font_struct, ranges, n_ranges);
      return TRUE;
    }
  else
    return FALSE;
}

/**
 * pango_x_xlfd_list_cfonts:
 * @font:       a #PangoFont.
 * @charsets:   the list of character sets to match against.
 * @n_charsets: the number of elements in @charsets.
 * @xlfds:      location to store a pointer to an array of returned XLFDs.
 * @n_xlfds:    location to store the number of XLFDs.
 *
 * List all possible XLFDs that can match a particular set
 * of character sets for the given #PangoFont. The
 * returned values are sorted at highest priority by
 * the order of the names in fontlist used to create
 * the #PangoFont, and then sorted by the ordering
 * of the character sets in @charsets.
 *
 */
void 
pango_x_list_cfonts (PangoFont *font,
			gchar      **charsets,
			gint         n_charsets,
			gchar     ***xlfds,
			gint        *n_xlfds)
{
  PangoXFont *xfont = (PangoXFont *)font;

  int i, j;

  GSList *result = NULL;
  GSList *tmp_list;

  gchar ***names = g_new (gchar **, n_charsets);

  *n_xlfds = 0;
  for (j=0; j<n_charsets; j++)
    names[j] = find_cfonts (font, charsets[j]);

  for (i=0; i < xfont->n_fonts; i++)
    for (j=0; j < n_charsets; j++)
      if (names[j][i] != 0)
	{
	  (*n_xlfds)++;
	  result = g_slist_prepend (result, g_strdup (names[j][i]));
	}

  result = g_slist_reverse (result);
  *xlfds = g_new (gchar *, *n_xlfds);

  tmp_list = result;
  for (i=0; i< *n_xlfds; i++)
    {
      (*xlfds)[i] = tmp_list->data;
      tmp_list = tmp_list->next;
    }
  
  g_slist_free (result);
  g_free (names);
}

void
name_hash_foreach (gpointer key, gpointer value, gpointer data)
{
  gchar *charset = key;
  gchar **names = value;
  PangoXFont *xfont = data;
  int i;

  for (i=0; i<xfont->n_fonts; i++)
    g_free (names[i]);
  g_free (names);
  g_free (charset);
}

void
xlfd_hash_foreach (gpointer key, gpointer value, gpointer data)
{
  gchar *xlfd = key;
  XLFDInfo *info = value;

  if (info->cfont)
    pango_cfont_unref (info->cfont);
  else if (info->font_struct)
    XFreeFontInfo (NULL, info->font_struct, 1);

  g_free (info);
  
  g_free (xlfd);
}

static void
pango_x_font_destroy (PangoFont *font)
{
  PangoXFont *xfont = (PangoXFont *)font;

  g_hash_table_foreach (xfont->name_hash, name_hash_foreach, xfont);
  g_hash_table_destroy (xfont->name_hash);

  g_hash_table_foreach (xfont->xlfd_hash, xlfd_hash_foreach, xfont);
  g_hash_table_destroy (xfont->xlfd_hash);

  g_strfreev (xfont->fonts);
  g_free (font);
}

static void
pango_x_cfont_destroy (PangoCFont *font)
{
  PangoXCFont *xcfont = (PangoXCFont *)font;

  XFreeFont (xcfont->display, xcfont->font_struct);
  
  g_free (font);
}
