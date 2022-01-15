#pragma once

#include <glib-object.h>
#include <pango/pango-types.h>
#include <pango/pango-attributes.h>
#include <pango/pango-lines.h>
#include <pango/pango-tabs.h>

G_BEGIN_DECLS

#define PANGO_TYPE_SIMPLE_LAYOUT pango_simple_layout_get_type ()

PANGO_AVAILABLE_IN_ALL
G_DECLARE_FINAL_TYPE (PangoSimpleLayout, pango_simple_layout, PANGO, SIMPLE_LAYOUT, GObject);

PANGO_AVAILABLE_IN_ALL
PangoSimpleLayout *     pango_simple_layout_new            (PangoContext       *context);

PANGO_AVAILABLE_IN_ALL
PangoSimpleLayout *     pango_simple_layout_copy           (PangoSimpleLayout  *layout);

PANGO_AVAILABLE_IN_ALL
guint                   pango_simple_layout_get_serial     (PangoSimpleLayout  *layout);

PANGO_AVAILABLE_IN_ALL
PangoContext *          pango_simple_layout_get_context    (PangoSimpleLayout  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_simple_layout_set_text       (PangoSimpleLayout  *layout,
                                                            const char         *text,
                                                            int                 length);
PANGO_AVAILABLE_IN_ALL
const char *            pango_simple_layout_get_text       (PangoSimpleLayout  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_simple_layout_set_markup     (PangoSimpleLayout  *layout,
                                                            const char         *markup,
                                                            int                 length);

PANGO_AVAILABLE_IN_ALL
int                     pango_simple_layout_get_character_count
                                                           (PangoSimpleLayout  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_simple_layout_set_attributes (PangoSimpleLayout  *layout,
                                                            PangoAttrList      *attrs);

PANGO_AVAILABLE_IN_ALL
PangoAttrList *         pango_simple_layout_get_attributes (PangoSimpleLayout  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_simple_layout_set_font_description
                                                           (PangoSimpleLayout          *layout,
                                                            const PangoFontDescription *desc);

PANGO_AVAILABLE_IN_ALL
const PangoFontDescription *
                        pango_simple_layout_get_font_description
                                                           (PangoSimpleLayout  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_simple_layout_set_line_spacing
                                                           (PangoSimpleLayout  *layout,
                                                            float               line_spacing);

PANGO_AVAILABLE_IN_ALL
float                   pango_simple_layout_get_line_spacing
                                                           (PangoSimpleLayout  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_simple_layout_set_width      (PangoSimpleLayout  *layout,
                                                            int                 width);

PANGO_AVAILABLE_IN_ALL
int                     pango_simple_layout_get_width      (PangoSimpleLayout  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_simple_layout_set_height     (PangoSimpleLayout  *layout,
                                                            int                 height);

PANGO_AVAILABLE_IN_ALL
int                     pango_simple_layout_get_height     (PangoSimpleLayout  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_simple_layout_set_tabs       (PangoSimpleLayout  *layout,
                                                            PangoTabArray      *tabs);

PANGO_AVAILABLE_IN_ALL
PangoTabArray *         pango_simple_layout_get_tabs       (PangoSimpleLayout  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_simple_layout_set_single_paragraph
                                                           (PangoSimpleLayout  *layout,
                                                            gboolean            single_paragraph);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_simple_layout_get_single_paragraph
                                                           (PangoSimpleLayout  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_simple_layout_set_wrap       (PangoSimpleLayout  *layout,
                                                            PangoWrapMode       wrap);

PANGO_AVAILABLE_IN_ALL
PangoWrapMode           pango_simple_layout_get_wrap       (PangoSimpleLayout  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_simple_layout_set_indent     (PangoSimpleLayout  *layout,
                                                            int                 indent);

PANGO_AVAILABLE_IN_ALL
int                     pango_simple_layout_get_indent     (PangoSimpleLayout  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_simple_layout_set_alignment  (PangoSimpleLayout  *layout,
                                                            PangoAlignmentMode  alignment);

PANGO_AVAILABLE_IN_ALL
PangoAlignmentMode      pango_simple_layout_get_alignment  (PangoSimpleLayout  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_simple_layout_set_ellipsize  (PangoSimpleLayout  *layout,
                                                            PangoEllipsizeMode  ellipsize);

PANGO_AVAILABLE_IN_ALL
PangoEllipsizeMode      pango_simple_layout_get_ellipsize  (PangoSimpleLayout  *layout);

PANGO_AVAILABLE_IN_ALL
void                    pango_simple_layout_set_auto_dir   (PangoSimpleLayout  *layout,
                                                            gboolean            auto_dir);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_simple_layout_get_auto_dir   (PangoSimpleLayout  *layout);

PANGO_AVAILABLE_IN_ALL
PangoLines *            pango_simple_layout_get_lines      (PangoSimpleLayout  *layout);

PANGO_AVAILABLE_IN_ALL
PangoLineIter *         pango_simple_layout_get_iter       (PangoSimpleLayout  *layout);

PANGO_AVAILABLE_IN_ALL
const PangoLogAttr *    pango_simple_layout_get_log_attrs  (PangoSimpleLayout  *layout,
                                                            int                *n_attrs);

typedef enum {
  PANGO_SIMPLE_LAYOUT_SERIALIZE_DEFAULT = 0,
  PANGO_SIMPLE_LAYOUT_SERIALIZE_CONTEXT = 1 << 0,
  PANGO_SIMPLE_LAYOUT_SERIALIZE_OUTPUT = 1 << 1,
} PangoSimpleLayoutSerializeFlags;

PANGO_AVAILABLE_IN_ALL
GBytes *                pango_simple_layout_serialize      (PangoSimpleLayout               *layout,
                                                            PangoSimpleLayoutSerializeFlags  flags);

PANGO_AVAILABLE_IN_ALL
gboolean                pango_simple_layout_write_to_file  (PangoSimpleLayout               *layout,
                                                            const char                      *filename);

typedef enum {
  PANGO_SIMPLE_LAYOUT_DESERIALIZE_DEFAULT = 0,
  PANGO_SIMPLE_LAYOUT_DESERIALIZE_CONTEXT = 1 << 0,
} PangoSimpleLayoutDeserializeFlags;

PANGO_AVAILABLE_IN_ALL
PangoSimpleLayout *     pango_simple_layout_deserialize    (PangoContext                       *context,
                                                            GBytes                             *bytes,
                                                            PangoSimpleLayoutDeserializeFlags   flags,
                                                            GError                            **error);


G_END_DECLS
