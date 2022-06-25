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


struct _Pango2FontFace
{
  GObject parent_instance;

  Pango2FontFamily *family;
  Pango2FontDescription *description;
  char *name;
  char *faceid;
};

typedef struct _Pango2FontFaceClass Pango2FontFaceClass;

struct _Pango2FontFaceClass
{
  GObjectClass parent_class;

  gboolean               (* is_synthesized)    (Pango2FontFace *face);
  gboolean               (* is_monospace)      (Pango2FontFace *face);
  gboolean               (* is_variable)       (Pango2FontFace *face);
  gboolean               (* supports_language) (Pango2FontFace *face,
                                                Pango2Language *language);
  Pango2Language **      (* get_languages)     (Pango2FontFace *face);
  gboolean               (* has_char)          (Pango2FontFace *face,
                                                gunichar        wc);
  const char *           (* get_faceid)        (Pango2FontFace *face);
  Pango2Font *           (* create_font)       (Pango2FontFace              *face,
                                                const Pango2FontDescription *desc,
                                                float                        dpi,
                                                const Pango2Matrix          *ctm);
};

#define PANGO2_FONT_FACE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), PANGO2_TYPE_FONT_FACE, Pango2FontFaceClass))
#define PANGO2_FONT_FACE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), PANGO2_TYPE_FONT_FACE, Pango2FontFaceClass))

const char *    pango2_font_face_get_faceid      (Pango2FontFace              *face);
Pango2Font *    pango2_font_face_create_font     (Pango2FontFace              *face,
                                                  const Pango2FontDescription *desc,
                                                  float                        dpi,
                                                  const Pango2Matrix          *ctm);

static inline void
pango2_font_face_set_name (Pango2FontFace *face,
                           const char     *name)
{
  face->name = g_strdup (name);
}

static inline void
pango2_font_face_set_description (Pango2FontFace              *face,
                                  const Pango2FontDescription *description)
{
  face->description = pango2_font_description_copy (description);
}

static inline void
pango2_font_face_set_family (Pango2FontFace   *face,
                             Pango2FontFamily *family)
{
  face->family = family;
}
