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

#pragma once

#include "pango-hbface.h"
#include "pango-fontmap.h"
#include "pango-language-set-private.h"
#include <hb.h>

typedef struct _CommonFace CommonFace;
struct _CommonFace {
  PangoFontFace parent_instance;

  PangoFontDescription *description;
  char *name;
  PangoFontFamily *family;
  char *psname;
  char *faceid;
};

struct _PangoHbFace
{
  PangoFontFace parent_instance;

  PangoFontDescription *description;
  char *name;
  PangoFontFamily *family;
  char *psname;
  char *faceid;

  /* up to here shared with PangoUserFace */

  unsigned int index;
  int instance_id;
  char *file;
  hb_face_t *face;
  hb_variation_t *variations;
  unsigned int n_variations;
  PangoMatrix *matrix;
  double x_scale, y_scale;
  PangoLanguageSet *languages;
  gboolean embolden;
  gboolean synthetic;
};

void                    pango_hb_face_set_family        (PangoHbFace          *self,
                                                         PangoFontFamily      *family);

PangoLanguageSet *      pango_hb_face_get_language_set  (PangoHbFace          *self);

PANGO_AVAILABLE_IN_1_52
void                    pango_hb_face_set_language_set  (PangoHbFace          *self,
                                                         PangoLanguageSet     *languages);

PANGO_AVAILABLE_IN_1_52
void                    pango_hb_face_set_matrix        (PangoHbFace          *self,
                                                         const PangoMatrix    *matrix);

gboolean                pango_hb_face_has_char          (PangoHbFace          *self,
                                                         gunichar              wc);

const char *            pango_hb_face_get_faceid        (PangoHbFace          *self);
