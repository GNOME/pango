/* Pango
 *
 * Copyright (C) 2021 Matthias Clasen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <math.h>

#include <gio/gio.h>

#include "pango-fontset-cached-private.h"
#include "pango-font-private.h"
#include "pango-font-face-private.h"
#include "pango-generic-family-private.h"

#ifdef HAVE_CAIRO
#include "pangocairo-font.h"
#endif

G_DEFINE_TYPE (PangoFontsetCached, pango_fontset_cached, PANGO_TYPE_FONTSET);

static void
pango_fontset_cached_init (PangoFontsetCached *fontset)
{
  fontset->items = g_ptr_array_new_with_free_func (g_object_unref);
  fontset->cache = g_hash_table_new_full (NULL, NULL, NULL, g_object_unref);
  fontset->language = NULL;
#ifdef HAVE_CAIRO
  fontset->font_options = NULL;
#endif
}

static void
pango_fontset_cached_finalize (GObject *object)
{
  PangoFontsetCached *self = (PangoFontsetCached *)object;

  g_ptr_array_free (self->items, TRUE);
  g_hash_table_unref (self->cache);
  pango_font_description_free (self->description);
#ifdef HAVE_CAIRO
  if (self->font_options)
    cairo_font_options_destroy (self->font_options);
#endif

  G_OBJECT_CLASS (pango_fontset_cached_parent_class)->finalize (object);
}

static PangoFont *
pango_fontset_cached_get_font (PangoFontset *fontset,
                               guint         wc)
{
  PangoFontsetCached *self = (PangoFontsetCached *)fontset;
  PangoFont *retval;
  int i;

  retval = g_hash_table_lookup (self->cache, GUINT_TO_POINTER (wc));
  if (retval)
    return g_object_ref (retval);

  for (i = 0; i < self->items->len; i++)
    {
      gpointer item = g_ptr_array_index (self->items, i);

      if (PANGO_IS_FONT (item))
        {
          PangoFont *font = PANGO_FONT (item);
          if (pango_font_face_has_char (font->face, wc))
            {
              retval = g_object_ref (font);
              break;
            }
        }
      else if (PANGO_IS_GENERIC_FAMILY (item))
        {
          PangoGenericFamily *family = PANGO_GENERIC_FAMILY (item);
          PangoFontFace *face;

          /* Here is where we implement delayed picking for generic families.
           * If a face does not cover the character and its family is generic,
           * choose a different face from the same family and create a font to use.
           */
          face = pango_generic_family_find_face (family,
                                                 self->description,
                                                 self->language,
                                                 wc);
          if (face)
            {
              GHashTableIter iter;
              PangoFont *font;

              g_hash_table_iter_init (&iter, self->cache);
              while (g_hash_table_iter_next (&iter, NULL, (gpointer *)&font))
                {
                  if (pango_font_get_face (font) == face)
                    {
                      retval = g_object_ref (font);
                      break;
                    }
                }

              if (!retval)
                {
                  retval = pango_font_face_create_font (face,
                                                        self->description,
                                                        self->dpi,
                                                        self->matrix);
#ifdef HAVE_CAIRO
                  pango_cairo_font_set_font_options (retval, self->font_options);
#endif
                }
              break;
            }
        }
    }

  if (retval)
    g_hash_table_insert (self->cache, GUINT_TO_POINTER (wc), g_object_ref (retval));

  return retval;
}

PangoFont *
pango_fontset_cached_get_first_font (PangoFontsetCached *self)
{
  gpointer item;

  item = g_ptr_array_index (self->items, 0);

  if (PANGO_IS_FONT (item))
    return g_object_ref (PANGO_FONT (item));
  else if (PANGO_IS_GENERIC_FAMILY (item))
    {
      PangoFontFace *face;
      PangoFont *font;

      face = pango_generic_family_find_face (PANGO_GENERIC_FAMILY (item),
                                             self->description,
                                             self->language,
                                             0);
      font = pango_font_face_create_font (face,
                                          self->description,
                                          self->dpi,
                                          self->matrix);
#ifdef HAVE_CAIRO
      pango_cairo_font_set_font_options (font, self->font_options);
#endif
      return font;
    }

  return NULL;
}

static PangoFontMetrics *
pango_fontset_cached_get_metrics (PangoFontset *fontset)
{
  PangoFontsetCached *self = (PangoFontsetCached *)fontset;

  if (self->items->len == 1)
    {
      PangoFont *font = pango_fontset_cached_get_first_font (self);
      PangoFontMetrics *ret;

      ret = pango_font_get_metrics (font, self->language);
      g_object_unref (font);

      return ret;
    }

  return PANGO_FONTSET_CLASS (pango_fontset_cached_parent_class)->get_metrics (fontset);
}

static PangoLanguage *
pango_fontset_cached_get_language (PangoFontset *fontset)
{
  PangoFontsetCached *self = (PangoFontsetCached *)fontset;

  return self->language;
}

static void
pango_fontset_cached_foreach (PangoFontset            *fontset,
                              PangoFontsetForeachFunc  func,
                              gpointer                 data)
{
  PangoFontsetCached *self = (PangoFontsetCached *)fontset;
  unsigned int i;

  for (i = 0; i < self->items->len; i++)
    {
      gpointer item = g_ptr_array_index (self->items, i);
      PangoFont *font = NULL;

      if (PANGO_IS_FONT (item))
        {
          font = g_object_ref (PANGO_FONT (item));
        }
      else if (PANGO_IS_GENERIC_FAMILY (item))
        {
          PangoFontFace *face = pango_generic_family_find_face (PANGO_GENERIC_FAMILY (item), self->description, self->language, 0);
          font = pango_font_face_create_font (face, self->description, self->dpi, self->matrix);
#ifdef HAVE_CAIRO
          pango_cairo_font_set_font_options (font, self->font_options);
#endif
        }

      if ((*func) (fontset, font, data))
        {
          g_object_unref (font);
          return;
        }
      g_object_unref (font);
    }
}

static void
pango_fontset_cached_class_init (PangoFontsetCachedClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  PangoFontsetClass *fontset_class = PANGO_FONTSET_CLASS (class);

  object_class->finalize = pango_fontset_cached_finalize;

  fontset_class->get_font = pango_fontset_cached_get_font;
  fontset_class->get_metrics = pango_fontset_cached_get_metrics;
  fontset_class->get_language = pango_fontset_cached_get_language;
  fontset_class->foreach = pango_fontset_cached_foreach;
}

PangoFontsetCached *
pango_fontset_cached_new (const PangoFontDescription *description,
                          PangoLanguage              *language,
                          float                       dpi,
                          const PangoMatrix          *matrix)
{
  PangoFontsetCached *self;

  self = g_object_new (pango_fontset_cached_get_type (), NULL);
  self->language = language;
  self->description = pango_font_description_copy (description);
  self->dpi = dpi;
  self->matrix = matrix;

  return self;
}

void
pango_fontset_cached_add_face (PangoFontsetCached *self,
                               PangoFontFace      *face)
{
  PangoFont *font;

  font = pango_font_face_create_font (face,
                                      self->description,
                                      self->dpi,
                                      self->matrix);
#ifdef HAVE_CAIRO
  pango_cairo_font_set_font_options (font, self->font_options);
#endif
  g_ptr_array_add (self->items, font);
}

void
pango_fontset_cached_add_family (PangoFontsetCached *self,
                                 PangoGenericFamily *family)
{
  g_ptr_array_add (self->items, g_object_ref (family));
}

int
pango_fontset_cached_size (PangoFontsetCached *self)
{
  return self->items->len;
}

/* vim:set foldmethod=marker expandtab: */
