/* Example code to show how to use user fonts with Pango
 *
 * Written by Matthias Clasen, 2022
 *
 * Permission to use, copy, modify, distribute, and sell this example
 * for any purpose is hereby granted without fee.
 * It is provided "as is" without express or implied warranty.
 *
 * Font data taken from the cairo user-font test, written by
 *   Kristian Høgsberg <krh@redhat.com>
 *   Behdad Esfahbod <behdad@behdad.org>
 */

#include <stdlib.h>
#include <stdio.h>

#include <pango/pangocairo.h>
#include <pango/pangofc-hbfontmap.h>

static PangoFontMap *fontmap;

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

const char text[] = "finally... pango user-font";

static PangoLayout *
get_layout (void)
{
  PangoContext *context;
  PangoLayout *layout;
  PangoFontDescription *desc;

  /* Create a PangoLayout, set the font and text */
  context = pango_font_map_create_context (fontmap);
  layout = pango_layout_new (context);
  g_object_unref (context);

  pango_layout_set_text (layout, text, -1);

  desc = pango_font_description_from_string ("Userfont 20");
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);

  pango_layout_write_to_file (layout, "out.layout");

  return layout;
}

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
shape_cb (PangoUserFace       *face,
          int                  size,
          const char          *text,
          int                  length,
          const PangoAnalysis *analysis,
          PangoGlyphString    *glyphs,
          PangoShapeFlags      flags,
          gpointer             user_data)
{
  int n_chars;
  const char *p;
  int cluster = 0;
  int i, j;
  int last_cluster;
  gboolean is_color;
  hb_glyph_extents_t ext;
  hb_position_t dummy;

  n_chars = g_utf8_strlen (text, length);

  pango_glyph_string_set_size (glyphs, n_chars);

  last_cluster = -1;

  p = text;
  j = 0;
  for (i = 0; i < n_chars; i++)
    {
      gunichar wc;
      PangoGlyph glyph = 0;
      PangoRectangle logical_rect;

      wc = g_utf8_get_char (p);

      if (g_unichar_type (wc) != G_UNICODE_NON_SPACING_MARK)
        cluster = p - text;

      /* Handle the fi ligature */
      if (p[0] == 'f' && p[1] == 'i')
        {
          p = g_utf8_next_char (p);
          i++;
          glyph_cb (face, 0xe000, &glyph, user_data);
        }
      else if (pango_is_zero_width (wc))
        glyph = PANGO_GLYPH_EMPTY;
      else if (!glyph_cb (face, wc, &glyph, user_data))
        glyph = PANGO_GET_UNKNOWN_GLYPH (wc);

      glyph_info_cb (face, size, glyph, &ext, &dummy, &dummy, &is_color, user_data);
      pango_font_get_glyph_extents (analysis->font, glyph, NULL, &logical_rect);

      glyphs->glyphs[j].glyph = glyph;

      glyphs->glyphs[j].attr.is_cluster_start = cluster != last_cluster;
      glyphs->glyphs[j].attr.is_color = is_color;

      glyphs->glyphs[j].geometry.x_offset = 0;
      glyphs->glyphs[j].geometry.y_offset = 0;
      glyphs->glyphs[j].geometry.width = logical_rect.width;

      glyphs->log_clusters[j] = cluster;

      j++;

      last_cluster = cluster;

      p = g_utf8_next_char (p);
    }

  glyphs->num_glyphs = j;

#if 0
  /* FIXME export this */
  if (analysis->level & 1)
    pango_glyph_string_reverse_range (glyphs, 0, glyphs->num_glyphs);
#endif

  return TRUE;
}

static gboolean
font_info_cb (PangoUserFace     *face,
              int                size,
              hb_font_extents_t *extents,
              gpointer           user_data)
{
  extents->ascender = 0.75 * size;
  extents->descender = - 0.25 * size;
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

static void
setup_fontmap (PangoHbFontMap *fontmap)
{
  PangoFontDescription *desc;
  PangoUserFace *face;

  desc = pango_font_description_new ();
  pango_font_description_set_family (desc, "Userfont");
  face = pango_user_face_new (font_info_cb,
                              glyph_cb,
                              glyph_info_cb,
                              shape_cb,
                              render_cb,
                              (gpointer) glyphs, NULL,
                              "Black", desc);
  pango_hb_font_map_add_face (fontmap, PANGO_FONT_FACE (face));
  pango_font_description_free (desc);
}

int
main (int argc, char **argv)
{
  cairo_t *cr;
  char *filename;
  cairo_status_t status;
  cairo_surface_t *surface;
  PangoLayout *layout;
  PangoRectangle ext;

  if (argc != 2)
    {
      g_printerr ("Usage: userfont OUTPUT_FILENAME\n");
      return 1;
    }

  filename = argv[1];

  fontmap = PANGO_FONT_MAP (pango_fc_hb_font_map_new ());
  setup_fontmap (PANGO_HB_FONT_MAP (fontmap));

  layout = get_layout ();

  pango_lines_get_extents (pango_layout_get_lines (layout), NULL, &ext);
  pango_extents_to_pixels (&ext, NULL);

  /* Now create the final surface and draw to it. */
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, ext.width + 20, ext.height + 20);
  cr = cairo_create (surface);

  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_paint (cr);
  cairo_set_source_rgb (cr, 0.0, 0.0, 0.5);

  cairo_move_to (cr, 10, 10);
  pango_cairo_show_layout (cr, layout);

  cairo_destroy (cr);

  g_object_unref (layout);

  /* Write out the surface as PNG */
  status = cairo_surface_write_to_png (surface, filename);
  cairo_surface_destroy (surface);

  if (status != CAIRO_STATUS_SUCCESS)
    {
      g_printerr ("Could not save png to '%s': %s\n", filename, cairo_status_to_string (status));
      return 1;
    }

  return 0;
}
