/*
 * Copyright (C) 2003 Red Hat Software
 * Copyright (C) 2005-2007 Imendio AB
 * Copyright (C) 2010  Kristian Rietveld  <kris@gtk.org>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "pangocoretext.h"
#include "pango-font-private.h"
#include "pango-fontmap-private.h"


#define PANGO_CORE_TEXT_FONT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_CORE_TEXT_FONT, PangoCoreTextFontClass))
#define PANGO_CORE_TEXT_FONT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_CORE_TEXT_FONT, PangoCoreTextFontClass))

typedef struct _PangoCoreTextFontPrivate  PangoCoreTextFontPrivate;

struct _PangoCoreTextFont
{
  PangoFont parent_instance;
  PangoCoreTextFontPrivate *priv;
};

struct _PangoCoreTextFontClass
{
  PangoFontClass parent_class;

  /*< private >*/

  /* Padding for future expansion */
  void (*_pango_reserved1) (void);
  void (*_pango_reserved2) (void);
  void (*_pango_reserved3) (void);
  void (*_pango_reserved4) (void);
};


#define PANGO_TYPE_CORE_TEXT_FONT_MAP             (pango_core_text_font_map_get_type ())
#define PANGO_CORE_TEXT_FONT_MAP(object)          (G_TYPE_CHECK_INSTANCE_CAST ((object), PANGO_TYPE_CORE_TEXT_FONT_MAP, PangoCoreTextFontMap))
#define PANGO_CORE_TEXT_IS_FONT_MAP(object)       (G_TYPE_CHECK_INSTANCE_TYPE ((object), PANGO_TYPE_CORE_TEXT_FONT_MAP))
#define PANGO_CORE_TEXT_FONT_MAP_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_CORE_TEXT_FONT_MAP, PangoCoreTextFontMapClass))
#define PANGO_CORE_TEXT_FONT_MAP_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_CORE_TEXT_FONT_MAP, PangoCoreTextFontMapClass))


typedef struct _PangoCoreTextFamily       PangoCoreTextFamily;
typedef struct _PangoCoreTextFace         PangoCoreTextFace;

typedef struct _PangoCoreTextFontMap      PangoCoreTextFontMap;
typedef struct _PangoCoreTextFontMapClass PangoCoreTextFontMapClass;

typedef struct _PangoCoreTextFontsetKey   PangoCoreTextFontsetKey;
typedef struct _PangoCoreTextFontKey      PangoCoreTextFontKey;

struct _PangoCoreTextFontMap
{
  PangoFontMap parent_instance;

  guint serial;
  GHashTable *fontset_hash;
  GHashTable *font_hash;

  GHashTable *families;
};

struct _PangoCoreTextFontMapClass
{
  PangoFontMapClass parent_class;

  gconstpointer (*context_key_get)   (PangoCoreTextFontMap   *ctfontmap,
                                      PangoContext           *context);
  gpointer     (*context_key_copy)   (PangoCoreTextFontMap   *ctfontmap,
                                      gconstpointer           key);
  void         (*context_key_free)   (PangoCoreTextFontMap   *ctfontmap,
                                      gpointer                key);
  guint32      (*context_key_hash)   (PangoCoreTextFontMap   *ctfontmap,
                                      gconstpointer           key);
  gboolean     (*context_key_equal)  (PangoCoreTextFontMap   *ctfontmap,
                                      gconstpointer           key_a,
                                      gconstpointer           key_b);

  PangoCoreTextFont * (* create_font)   (PangoCoreTextFontMap       *fontmap,
                                         PangoCoreTextFontKey       *key);

  double              (* get_resolution) (PangoCoreTextFontMap      *fontmap,
                                          PangoContext              *context);
};


_PANGO_EXTERN
GType                 pango_core_text_font_map_get_type          (void) G_GNUC_CONST;

void                  _pango_core_text_font_set_font_map         (PangoCoreTextFont    *afont,
                                                                  PangoCoreTextFontMap *fontmap);
PangoCoreTextFace *   _pango_core_text_font_get_face             (PangoCoreTextFont    *font);
gpointer              _pango_core_text_font_get_context_key      (PangoCoreTextFont    *afont);
void                  _pango_core_text_font_set_context_key      (PangoCoreTextFont    *afont,
                                                                  gpointer           context_key);
void                  _pango_core_text_font_set_font_key         (PangoCoreTextFont    *font,
                                                                  PangoCoreTextFontKey *key);
void                  _pango_core_text_font_set_ctfont           (PangoCoreTextFont    *font,
                                                                  CTFontRef         font_ref);

PangoFontDescription *_pango_core_text_font_description_from_ct_font_descriptor (CTFontDescriptorRef desc);

_PANGO_EXTERN
int                   pango_core_text_font_key_get_size             (const PangoCoreTextFontKey *key);
_PANGO_EXTERN
int                   pango_core_text_font_key_get_size    (const PangoCoreTextFontKey *key);
_PANGO_EXTERN
double                pango_core_text_font_key_get_resolution       (const PangoCoreTextFontKey *key);
_PANGO_EXTERN
gboolean              pango_core_text_font_key_get_synthetic_italic (const PangoCoreTextFontKey *key);
_PANGO_EXTERN
gpointer              pango_core_text_font_key_get_context_key      (const PangoCoreTextFontKey *key);
_PANGO_EXTERN
const PangoMatrix    *pango_core_text_font_key_get_matrix           (const PangoCoreTextFontKey *key);
_PANGO_EXTERN
PangoGravity          pango_core_text_font_key_get_gravity          (const PangoCoreTextFontKey *key);
_PANGO_EXTERN
CTFontDescriptorRef   pango_core_text_font_key_get_ctfontdescriptor (const PangoCoreTextFontKey *key);

PangoCoreTextFace *   pango_core_text_font_map_find_face (PangoCoreTextFontMap       *map,
                                                          const PangoCoreTextFontKey *key);
