/*
 * Copyright 2022 Red Hat, Inc.
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

#include <pango/pango-font-face.h>
#include <pango/pango-font-description.h>


struct _PangoFontFace
{
  GObject parent_instance;

  PangoFontFamily *family;
  PangoFontDescription *description;
  char *name;
  char *faceid;
};

typedef struct _PangoFontFaceClass PangoFontFaceClass;

struct _PangoFontFaceClass
{
  GObjectClass parent_class;

  gboolean               (* is_synthesized)    (PangoFontFace *face);
  gboolean               (* is_monospace)      (PangoFontFace *face);
  gboolean               (* is_variable)       (PangoFontFace *face);
  gboolean               (* supports_language) (PangoFontFace *face,
                                                PangoLanguage *language);
  PangoLanguage **       (* get_languages)     (PangoFontFace *face);
  gboolean               (* has_char)          (PangoFontFace *face,
                                                gunichar       wc);
  const char *           (* get_faceid)        (PangoFontFace *face);
  PangoFont *            (* create_font)       (PangoFontFace              *face,
                                                const PangoFontDescription *desc,
                                                float                       dpi,
                                                const PangoMatrix          *matrix);
};

#define PANGO_FONT_FACE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO_TYPE_FONT_FACE, PangoFontFaceClass))
#define PANGO_FONT_FACE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO_TYPE_FONT_FACE, PangoFontFaceClass))

const char *    pango_font_face_get_faceid      (PangoFontFace              *face);
PangoFont *     pango_font_face_create_font     (PangoFontFace              *face,
                                                 const PangoFontDescription *desc,
                                                 float                       dpi,
                                                 const PangoMatrix          *matrix);

static inline void
pango_font_face_set_name (PangoFontFace *face,
                          const char    *name)
{
  face->name = g_strdup (name);
}

static inline void
pango_font_face_set_description (PangoFontFace              *face,
                                 const PangoFontDescription *description)
{
  face->description = pango_font_description_copy (description);
}

static inline void
pango_font_face_set_family (PangoFontFace   *face,
                            PangoFontFamily *family)
{
  face->family = family;
}
