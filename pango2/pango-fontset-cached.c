/* Pango2
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

G_DEFINE_FINAL_TYPE (Pango2FontsetCached, pango2_fontset_cached, PANGO2_TYPE_FONTSET);

static void
pango2_fontset_cached_init (Pango2FontsetCached *fontset)
{
  fontset->items = g_ptr_array_new_with_free_func (g_object_unref);
  fontset->cache = g_hash_table_new_full (NULL, NULL, NULL, g_object_unref);
  fontset->language = NULL;
#ifdef HAVE_CAIRO
  fontset->font_options = NULL;
#endif
}

static void
pango2_fontset_cached_finalize (GObject *object)
{
  Pango2FontsetCached *self = (Pango2FontsetCached *)object;

  g_ptr_array_free (self->items, TRUE);
  g_hash_table_unref (self->cache);
  pango2_font_description_free (self->description);
#ifdef HAVE_CAIRO
  if (self->font_options)
    cairo_font_options_destroy (self->font_options);
#endif

  G_OBJECT_CLASS (pango2_fontset_cached_parent_class)->finalize (object);
}

static Pango2Font *
find_font_for_face (Pango2FontsetCached *self,
                    Pango2FontFace      *face)
{
  GHashTableIter iter;
  Pango2Font *font;

  g_hash_table_iter_init (&iter, self->cache);
  while (g_hash_table_iter_next (&iter, NULL, (gpointer *)&font))
    {
      if (pango2_font_get_face (font) == face)
        {
          return font;
          break;
        }
    }

  return NULL;
}

static Pango2Font *
pango2_fontset_cached_get_font (Pango2Fontset *fontset,
                                guint          wc)
{
  Pango2FontsetCached *self = (Pango2FontsetCached *)fontset;
  Pango2Font *retval;
  int i;

  retval = g_hash_table_lookup (self->cache, GUINT_TO_POINTER (wc));
  if (retval)
    return g_object_ref (retval);

  for (i = 0; i < self->items->len; i++)
    {
      gpointer item = g_ptr_array_index (self->items, i);

      if (PANGO2_IS_FONT (item))
        {
          Pango2Font *font = PANGO2_FONT (item);
          if (pango2_font_face_has_char (font->face, wc))
            {
              retval = g_object_ref (font);
              break;
            }
        }
      else if (PANGO2_IS_GENERIC_FAMILY (item))
        {
          Pango2GenericFamily *family = PANGO2_GENERIC_FAMILY (item);
          Pango2FontDescription *copy;
          Pango2FontFace *face;

          /* Here is where we implement delayed picking for generic families.
           * If a face does not cover the character and its family is generic,
           * choose a different face from the same family and create a font to use.
           */
          copy = pango2_font_description_copy_static (self->description);
          pango2_font_description_unset_fields (copy, PANGO2_FONT_MASK_VARIATIONS |
                                                      PANGO2_FONT_MASK_GRAVITY |
                                                      PANGO2_FONT_MASK_VARIANT);
          face = pango2_generic_family_find_face (family, copy, self->language, wc);
          pango2_font_description_free (copy);

          if (face)
            {
              Pango2Font *font;

              font = find_font_for_face (self, face);
              if (font)
                {
                  retval = g_object_ref (font);
                }
              else
                {
                  retval = pango2_font_face_create_font (face,
                                                         self->description,
                                                         self->dpi,
                                                         self->ctm);
#ifdef HAVE_CAIRO
                  pango2_cairo_font_set_font_options (retval, self->font_options);
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

Pango2Font *
pango2_fontset_cached_get_first_font (Pango2FontsetCached *self)
{
  gpointer item;

  item = g_ptr_array_index (self->items, 0);

  if (PANGO2_IS_FONT (item))
    return g_object_ref (PANGO2_FONT (item));
  else if (PANGO2_IS_GENERIC_FAMILY (item))
    {
      Pango2FontFace *face;
      Pango2Font *font;

      face = pango2_generic_family_find_face (PANGO2_GENERIC_FAMILY (item),
                                              self->description,
                                              self->language,
                                              0);
      font = find_font_for_face (self, face);
      if (font)
        g_object_ref (font);
      else
        {
          font = pango2_font_face_create_font (face,
                                               self->description,
                                               self->dpi,
                                               self->ctm);
#ifdef HAVE_CAIRO
          pango2_cairo_font_set_font_options (font, self->font_options);
#endif
        }

      return font;
    }

  return NULL;
}

static Pango2FontMetrics *
pango2_fontset_cached_get_metrics (Pango2Fontset *fontset)
{
  Pango2FontsetCached *self = (Pango2FontsetCached *)fontset;

  if (self->items->len == 1)
    {
      Pango2Font *font;
      Pango2FontMetrics *ret;

      font = pango2_fontset_cached_get_first_font (self);
      ret = pango2_font_get_metrics (font, self->language);
      g_object_unref (font);

      return ret;
    }

  return PANGO2_FONTSET_CLASS (pango2_fontset_cached_parent_class)->get_metrics (fontset);
}

static Pango2Language *
pango2_fontset_cached_get_language (Pango2Fontset *fontset)
{
  Pango2FontsetCached *self = (Pango2FontsetCached *)fontset;

  return self->language;
}

static void
pango2_fontset_cached_foreach (Pango2Fontset            *fontset,
                               Pango2FontsetForeachFunc  func,
                               gpointer                  data)
{
  Pango2FontsetCached *self = (Pango2FontsetCached *)fontset;
  unsigned int i;

  for (i = 0; i < self->items->len; i++)
    {
      gpointer item = g_ptr_array_index (self->items, i);
      Pango2Font *font = NULL;

      if (PANGO2_IS_FONT (item))
        {
          font = g_object_ref (PANGO2_FONT (item));
        }
      else if (PANGO2_IS_GENERIC_FAMILY (item))
        {
          Pango2FontFace *face;

          face = pango2_generic_family_find_face (PANGO2_GENERIC_FAMILY (item),
                                                  self->description,
                                                  self->language,
                                                  0);

          font = find_font_for_face (self, face);
          if (font)
            g_object_ref (font);
          else
            {
              font = pango2_font_face_create_font (face,
                                                   self->description,
                                                   self->dpi,
                                                   self->ctm);
#ifdef HAVE_CAIRO
              pango2_cairo_font_set_font_options (font, self->font_options);
#endif
            }
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
pango2_fontset_cached_class_init (Pango2FontsetCachedClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  Pango2FontsetClass *fontset_class = PANGO2_FONTSET_CLASS (class);

  object_class->finalize = pango2_fontset_cached_finalize;

  fontset_class->get_font = pango2_fontset_cached_get_font;
  fontset_class->get_metrics = pango2_fontset_cached_get_metrics;
  fontset_class->get_language = pango2_fontset_cached_get_language;
  fontset_class->foreach = pango2_fontset_cached_foreach;
}

Pango2FontsetCached *
pango2_fontset_cached_new (const Pango2FontDescription *description,
                           Pango2Language              *language,
                           float                        dpi,
                           const Pango2Matrix          *ctm)
{
  Pango2FontsetCached *self;

  self = g_object_new (pango2_fontset_cached_get_type (), NULL);
  self->language = language;
  self->description = pango2_font_description_copy (description);
  self->dpi = dpi;
  self->ctm = ctm;

  return self;
}

void
pango2_fontset_cached_add_face (Pango2FontsetCached *self,
                                Pango2FontFace      *face)
{
  Pango2Font *font;

  font = pango2_font_face_create_font (face,
                                       self->description,
                                       self->dpi,
                                       self->ctm);
#ifdef HAVE_CAIRO
  pango2_cairo_font_set_font_options (font, self->font_options);
#endif
  g_ptr_array_add (self->items, font);
}

void
pango2_fontset_cached_add_family (Pango2FontsetCached *self,
                                  Pango2GenericFamily *family)
{
  g_ptr_array_add (self->items, g_object_ref (family));
}

int
pango2_fontset_cached_size (Pango2FontsetCached *self)
{
  return self->items->len;
}

void
pango2_fontset_cached_append (Pango2FontsetCached *self,
                              Pango2FontsetCached *other)
{
  for (int i = 0; i < other->items->len; i++)
    {
      gpointer item = g_ptr_array_index (other->items, i);

      if (PANGO2_IS_FONT (item))
        pango2_fontset_cached_add_face (self, pango2_font_get_face (PANGO2_FONT (item)));
      else if (PANGO2_IS_GENERIC_FAMILY (item))
        pango2_fontset_cached_add_family (self, PANGO2_GENERIC_FAMILY (item));
    }
}

/* vim:set foldmethod=marker expandtab: */
