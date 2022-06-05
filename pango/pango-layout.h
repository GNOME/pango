/*
 * Copyright (C) 2000 Red Hat Software
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

#include <glib-object.h>
#include <pango/pango-types.h>
#include <pango/pango-attributes.h>
#include <pango/pango-lines.h>
#include <pango/pango-tabs.h>

G_BEGIN_DECLS

#define PANGO_TYPE_LAYOUT pango_layout_get_type ()

PANGO_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (PangoLayout, pango_layout, PANGO, LAYOUT, GObject);

PANGO_AVAILABLE_IN_ALL
PangoLayout *           pango_layout_new            (PangoContext                 *context);

PANGO_AVAILABLE_IN_ALL
PangoLayout *           pango_layout_copy           (PangoLayout                  *layout);

PANGO_AVAILABLE_IN_ALL
guint                   pango_layout_get_serial     (PangoLayout                  *layout);

PANGO_AVAILABLE_IN_ALL
PangoContext *          pango_layout_get_context    (PangoLayout                  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_context_changed (PangoLayout                 *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_set_text       (PangoLayout                  *layout,
                                                     const char                   *text,
                                                     int                           length);
PANGO_AVAILABLE_IN_ALL
const char *            pango_layout_get_text       (PangoLayout                  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_set_markup     (PangoLayout                  *layout,
                                                     const char                   *markup,
                                                     int                           length);

PANGO_AVAILABLE_IN_ALL
int                     pango_layout_get_character_count
                                                    (PangoLayout                  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_set_attributes (PangoLayout                  *layout,
                                                     PangoAttrList                *attrs);

PANGO_AVAILABLE_IN_ALL
PangoAttrList *         pango_layout_get_attributes (PangoLayout                  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_set_font_description
                                                    (PangoLayout                  *layout,
                                                     const PangoFontDescription   *desc);

PANGO_AVAILABLE_IN_ALL
const PangoFontDescription *
                        pango_layout_get_font_description
                                                    (PangoLayout                  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_set_line_height
                                                    (PangoLayout                  *layout,
                                                     float                         line_height);

PANGO_AVAILABLE_IN_ALL
float                   pango_layout_get_line_height
                                                    (PangoLayout                  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_set_spacing    (PangoLayout                  *layout,
                                                     int                           spacing);

PANGO_AVAILABLE_IN_ALL
int                     pango_layout_get_spacing    (PangoLayout                  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_set_width      (PangoLayout                  *layout,
                                                     int                           width);

PANGO_AVAILABLE_IN_ALL
int                     pango_layout_get_width      (PangoLayout                  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_set_height     (PangoLayout                  *layout,
                                                     int                           height);

PANGO_AVAILABLE_IN_ALL
int                     pango_layout_get_height     (PangoLayout                  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_set_tabs       (PangoLayout                  *layout,
                                                     PangoTabArray                *tabs);

PANGO_AVAILABLE_IN_ALL
PangoTabArray *         pango_layout_get_tabs       (PangoLayout                  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_set_single_paragraph
                                                    (PangoLayout                  *layout,
                                                     gboolean                      single_paragraph);
PANGO_AVAILABLE_IN_ALL
gboolean                pango_layout_get_single_paragraph
                                                    (PangoLayout                  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_set_wrap       (PangoLayout                  *layout,
                                                     PangoWrapMode                 wrap);

PANGO_AVAILABLE_IN_ALL
PangoWrapMode           pango_layout_get_wrap       (PangoLayout                  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_set_indent     (PangoLayout                  *layout,
                                                     int                           indent);

PANGO_AVAILABLE_IN_ALL
int                     pango_layout_get_indent     (PangoLayout                  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_set_alignment  (PangoLayout                  *layout,
                                                     PangoAlignment                alignment);

PANGO_AVAILABLE_IN_ALL
PangoAlignment          pango_layout_get_alignment  (PangoLayout                  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_set_ellipsize  (PangoLayout                  *layout,
                                                     PangoEllipsizeMode            ellipsize);

PANGO_AVAILABLE_IN_ALL
PangoEllipsizeMode      pango_layout_get_ellipsize  (PangoLayout                  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_layout_set_auto_dir   (PangoLayout                  *layout,
                                                     gboolean                      auto_dir);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_layout_get_auto_dir   (PangoLayout                  *layout);

PANGO_AVAILABLE_IN_ALL
PangoLines *            pango_layout_get_lines      (PangoLayout                  *layout);

PANGO_AVAILABLE_IN_ALL
PangoLineIter *         pango_layout_get_iter       (PangoLayout                  *layout);

PANGO_AVAILABLE_IN_ALL
const PangoLogAttr *    pango_layout_get_log_attrs  (PangoLayout                  *layout,
                                                     int                          *n_attrs);

typedef enum {
  PANGO_LAYOUT_SERIALIZE_DEFAULT = 0,
  PANGO_LAYOUT_SERIALIZE_CONTEXT = 1 << 0,
  PANGO_LAYOUT_SERIALIZE_OUTPUT = 1 << 1,
} PangoLayoutSerializeFlags;

PANGO_AVAILABLE_IN_ALL
GBytes *                pango_layout_serialize      (PangoLayout                  *layout,
                                                     PangoLayoutSerializeFlags     flags);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_layout_write_to_file  (PangoLayout                  *layout,
                                                     const char                   *filename);

#define PANGO_LAYOUT_DESERIALIZE_ERROR (pango_layout_deserialize_error_quark ())

/**
 * PangoLayoutDeserializeError:
 * @PANGO_LAYOUT_DESERIALIZE_INVALID: Unspecified error
 * @PANGO_LAYOUT_DESERIALIZE_INVALID_VALUE: A JSon value could not be
 *   interpreted
 * @PANGO_LAYOUT_DESERIALIZE_MISSING_VALUE: A required JSon member was
 *   not found
 *
 * Errors that can be returned by [func@Pango.Layout.deserialize].
 */
typedef enum {
  PANGO_LAYOUT_DESERIALIZE_INVALID,
  PANGO_LAYOUT_DESERIALIZE_INVALID_VALUE,
  PANGO_LAYOUT_DESERIALIZE_MISSING_VALUE,
} PangoLayoutDeserializeError;

typedef enum {
  PANGO_LAYOUT_DESERIALIZE_DEFAULT = 0,
  PANGO_LAYOUT_DESERIALIZE_CONTEXT = 1 << 0,
} PangoLayoutDeserializeFlags;

PANGO_AVAILABLE_IN_ALL
GQuark                  pango_layout_deserialize_error_quark (void);

PANGO_AVAILABLE_IN_ALL
PangoLayout *           pango_layout_deserialize    (PangoContext                 *context,
                                                     GBytes                       *bytes,
                                                     PangoLayoutDeserializeFlags   flags,
                                                     GError                      **error);

G_END_DECLS
