/* Pango
 * pango-impl-utils.h: Macros for get_type() functions
 * Inspired by Jody Goldberg's gsf-impl-utils.h
 *
 * Copyright (C) 2003 Red Hat Software
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

#ifndef __PANGO_IMPL_UTILS_H__
#define __PANGO_IMPL_UTILS_H__

#include <glib.h>
#include <glib-object.h>
#include <pango/pango.h>

G_BEGIN_DECLS


/* String interning for static strings */
#define I_(string) g_intern_static_string (string)


/* Some functions for handling PANGO_ATTR_SHAPE */
void _pango_shape_shape (const char       *text,
			 unsigned int      n_chars,
			 PangoRectangle   *shape_ink,
			 PangoRectangle   *shape_logical,
			 PangoGlyphString *glyphs);

void _pango_shape_get_extents (gint              n_chars,
			       PangoRectangle   *shape_ink,
			       PangoRectangle   *shape_logical,
			       PangoRectangle   *ink_rect,
			       PangoRectangle   *logical_rect);


/* We define these functions static here because we don't want to add public API
 * for them (if anything, it belongs to glib, but glib found it trivial enough
 * not to add API for).  At some point metrics calculations will be
 * centralized and this mess can be minimized.  Or so I hope.
 */

static inline G_GNUC_UNUSED int
pango_unichar_width (gunichar c)
{
  return G_UNLIKELY (g_unichar_iszerowidth (c)) ? 0 :
	   G_UNLIKELY (g_unichar_iswide (c)) ? 2 : 1;
}

static G_GNUC_UNUSED glong
pango_utf8_strwidth (const gchar *p)
{
  glong len = 0;
  g_return_val_if_fail (p != NULL, 0);

  while (*p)
    {
      len += pango_unichar_width (g_utf8_get_char (p));
      p = g_utf8_next_char (p);
    }

  return len;
}

/* Glib's g_utf8_strlen() is broken and stops at embedded NUL's.
 * Wrap it here. */
static G_GNUC_UNUSED glong
pango_utf8_strlen (const gchar *p, gssize max)
{
  glong len = 0;
  const gchar *start = p;
  g_return_val_if_fail (p != NULL || max == 0, 0);

  if (max <= 0)
    return g_utf8_strlen (p, max);

  p = g_utf8_next_char (p);
  while (p - start < max)
    {
      ++len;
      p = g_utf8_next_char (p);
    }

  /* only do the last len increment if we got a complete
   * char (don't count partial chars)
   */
  if (p - start <= max)
    ++len;

  return len;
}


/* To be made public at some point */

static G_GNUC_UNUSED void
pango_glyph_string_reverse_range (PangoGlyphString *glyphs,
				  int start, int end)
{
  int i, j;

  for (i = start, j = end - 1; i < j; i++, j--)
    {
      PangoGlyphInfo glyph_info;
      gint log_cluster;

      glyph_info = glyphs->glyphs[i];
      glyphs->glyphs[i] = glyphs->glyphs[j];
      glyphs->glyphs[j] = glyph_info;

      log_cluster = glyphs->log_clusters[i];
      glyphs->log_clusters[i] = glyphs->log_clusters[j];
      glyphs->log_clusters[j] = log_cluster;
    }
}

/* The cairo hexbox drawing code assumes
 * that these nicks are 1-6 ASCII chars
 */
static struct {
  gunichar ch;
  const char *nick;
} ignorables[] = {
  { 0x00ad, "SHY"   }, /* SOFT HYPHEN */
  { 0x034f, "CGJ"   }, /* COMBINING GRAPHEME JOINER */
  { 0x200b, "ZWS"   }, /* ZERO WIDTH SPACE */
  { 0x200c, "ZWNJ"  }, /* ZERO WIDTH NON-JOINER */
  { 0x200d, "ZWJ"   }, /* ZERO WIDTH JOINER */
  { 0x200e, "LRM"   }, /* LEFT-TO-RIGHT MARK */
  { 0x200f, "RLM"   }, /* RIGHT-TO-LEFT MARK */
  { 0x2028, "LS"    }, /* LINE SEPARATOR */
  { 0x2029, "PS"    }, /* PARAGRAPH SEPARATOR */
  { 0x202a, "LRE"   }, /* LEFT-TO-RIGHT EMBEDDING */
  { 0x202b, "RLE"   }, /* RIGHT-TO-LEFT EMBEDDING */
  { 0x202c, "PDF"   }, /* POP DIRECTIONAL FORMATTING */
  { 0x202d, "LRO"   }, /* LEFT-TO-RIGHT OVERRIDE */
  { 0x202e, "RLO"   }, /* RIGHT-TO-LEFT OVERRIDE */
  { 0x2060, "WJ"    }, /* WORD JOINER */
  { 0x2061, "FA"    }, /* FUNCTION APPLICATION */
  { 0x2062, "IT"    }, /* INVISIBLE TIMES */
  { 0x2063, "IS"    }, /* INVISIBLE SEPARATOR */
  { 0xfeff, "ZWNBS" }, /* ZERO WIDTH NO-BREAK SPACE */
};

static inline G_GNUC_UNUSED const char *
pango_get_ignorable (gunichar ch)
{
  int i;

  for (i = 0; i < G_N_ELEMENTS (ignorables); i++)
    {
      if (ch == ignorables[i].ch)
        return ignorables[i].nick;
    }
  return NULL;
}

static inline G_GNUC_UNUSED const char *
pango_get_ignorable_size (gunichar  ch,
                          int      *rows,
                          int      *cols)
{
  const char *nick;
  int len;

  nick = pango_get_ignorable (ch);
  if (nick)
    {
      len = strlen (nick);
      if (len < 4)
        {
          *rows = 1;
          *cols = len;
        }
      else if (len > 4)
        {
          *rows = 2;
          *cols = 3;
         }
       else
        {
          *rows = 2;
          *cols = 2;
        }
    }

  return nick;
}

G_END_DECLS

#endif /* __PANGO_IMPL_UTILS_H__ */
