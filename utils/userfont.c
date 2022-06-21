#include <stdlib.h>
#include <stdio.h>

#include <pango/pangocairo.h>
#include "userfont.h"

#define END_GLYPH 0
#define STROKE 126
#define CLOSE 127

/* Simple glyph definition: 1 - 15 means lineto (or moveto for first
 * point) for one of the points on this grid:
 *
 *      1  2  3
 *      4  5  6
 *      7  8  9
 * ----10 11 12----(baseline)
 *     13 14 15
 */
typedef struct
{
  gunichar ucs4;
  int width;
  char data[16];
} test_scaled_font_glyph_t;

/* Simple glyph definition: 1 - 15 means lineto (or moveto for first
 * point) for one of the points on this grid:
 *
 *      1  2  3
 *      4  5  6
 *      7  8  9
 * ----10 11 12----(baseline)
 *     13 14 15
 */
static const test_scaled_font_glyph_t glyphs [] = {
  { 'a',  3, { 4, 6, 12, 10, 7, 9, STROKE, END_GLYPH } },
  { 'c',  3, { 6, 4, 10, 12, STROKE, END_GLYPH } },
  { 'e',  3, { 12, 10, 4, 6, 9, 7, STROKE, END_GLYPH } },
  { 'f',  3, { 3, 2, 11, STROKE, 4, 6, STROKE, END_GLYPH } },
  { 'g',  3, { 12, 10, 4, 6, 15, 13, STROKE, END_GLYPH } },
  { 'h',  3, { 1, 10, STROKE, 7, 5, 6, 12, STROKE, END_GLYPH } },
  { 'i',  1, { 1, 1, STROKE, 4, 10, STROKE, END_GLYPH } },
  { 'l',  1, { 1, 10, STROKE, END_GLYPH } },
  { 'n',  3, { 10, 4, STROKE, 7, 5, 6, 12, STROKE, END_GLYPH } },
  { 'o',  3, { 4, 10, 12, 6, CLOSE, END_GLYPH } },
  { 'p',  3, { 4, 10, 12, 6, CLOSE, 4, 13, STROKE, END_GLYPH } },
  { 'r',  3, { 4, 10, STROKE, 7, 5, 6, STROKE, END_GLYPH } },
  { 's',  3, { 6, 4, 7, 9, 12, 10, STROKE, END_GLYPH } },
  { 't',  3, { 2, 11, 12, STROKE, 4, 6, STROKE, END_GLYPH } },
  { 'u',  3, { 4, 10, 12, 6, STROKE, END_GLYPH } },
  { 'y',  3, { 4, 10, 12, 6, STROKE, 12, 15, 13, STROKE, END_GLYPH } },
  { 'z',  3, { 4, 6, 10, 12, STROKE, END_GLYPH } },
  { ' ',  1, { END_GLYPH } },
  { '-',  2, { 7, 8, STROKE, END_GLYPH } },
  { '.',  1, { 10, 10, STROKE, END_GLYPH } },
  { 0xe000, 3, { 3, 2, 11, STROKE, 4, 6, STROKE, 3, 3, STROKE, 6, 12, STROKE, END_GLYPH } }, /* fi */
  { -1,  0, { END_GLYPH } },
};

static gboolean
glyph_cb (PangoUserFace  *face,
          hb_codepoint_t  unicode,
          hb_codepoint_t *glyph,
          gpointer        user_data)
{
  test_scaled_font_glyph_t *glyphs = user_data;

  for (int i = 0; glyphs[i].ucs4 != (gunichar) -1; i++)
    {
      if (glyphs[i].ucs4 == unicode)
        {
          *glyph = i;
          return TRUE;
        }
    }

  return FALSE;
}

static gboolean
glyph_info_cb (PangoUserFace      *face,
               int                 size,
               hb_codepoint_t      glyph,
               hb_glyph_extents_t *extents,
               hb_position_t      *h_advance,
               hb_position_t      *v_advance,
               gboolean           *is_color,
               gpointer            user_data)
{
  test_scaled_font_glyph_t *glyphs = user_data;

  extents->x_bearing = 0;
  extents->y_bearing = - 0.75 * size;
  extents->width = glyphs[glyph].width / 4.0 * size;
  extents->height = size;

  *h_advance = *v_advance = glyphs[glyph].width / 4.0 * size;

  *is_color = FALSE;

  return TRUE;
}

static gboolean
font_info_cb (PangoUserFace     *face,
              int                size,
              hb_font_extents_t *extents,
              gpointer           user_data)
{
  extents->ascender = - 0.75 * size;
  extents->descender = 0.25 * size;
  extents->line_gap = 0;

  return TRUE;
}

static gboolean
render_cb (PangoUserFace  *face,
           int             size,
           hb_codepoint_t  glyph,
           gpointer        user_data,
           const char     *backend_id,
           gpointer        backend_data)
{
  test_scaled_font_glyph_t *glyphs = user_data;
  cairo_t *cr = backend_data;
  const char *data;
  div_t d;
  double x, y;

  if (strcmp (backend_id, "cairo") != 0)
    return FALSE;

  cairo_set_line_width (cr, 0.1);
  cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);

  data = glyphs[glyph].data;
  for (int i = 0; data[i] != END_GLYPH; i++)
    {
      switch (data[i])
        {
        case STROKE:
          cairo_new_sub_path (cr);
          break;

        case CLOSE:
          cairo_close_path (cr);
          break;

        default:
          d = div (data[i] - 1, 3);
          x = d.rem / 4.0 + 0.125;
          y = d.quot / 5.0 + 0.4 - 1.0;
          cairo_line_to (cr, x, y);
        }
    }

  cairo_stroke (cr);

  return TRUE;
}

void
add_userfont (PangoFontMap *fontmap)
{
  PangoFontDescription *desc;
  PangoUserFace *face;

  desc = pango_font_description_new ();
  pango_font_description_set_family (desc, "Userfont");
  face = pango_user_face_new (font_info_cb,
                              glyph_cb,
                              glyph_info_cb,
                              NULL,
                              render_cb,
                              (gpointer) glyphs, NULL,
                              "Black", desc);
  pango_font_map_add_face (fontmap, PANGO_FONT_FACE (face));
  pango_font_description_free (desc);
}
