/* Example code to show how to use pangocairo to render arbitrary shapes
 * inside a text layout, positioned by Pango.  This has become possibly
 * using the following API added in Pango 1.18:
 * 
 *      pango_cairo_context_set_shape_renderer ()
 *
 * This examples uses a small parser to convert shapes in the format of
 * SVG paths to cairo instructions.  You can typically extract these from
 * the SVG file's <path> elements directly.
 *
 * The code then searches for the Unicode bullet character in the layout
 * text and automatically adds PangoAttribtues to the layout to replace
 * each of the with a rendering of the GNOME Foot logo.
 *
 *
 * Written by Behdad Esfahbod, 2007
 *
 * Permission to use, copy, modify, distribute, and sell this example
 * for any purpose is hereby granted without fee.
 * It is provided "as is" without express or implied warranty.
 */


#include <stdio.h>
#include <string.h>

#include <pango/pangocairo.h>
#include <pango/pangofc-hbfontmap.h>

static PangoFontMap *fontmap;

#define BULLET "•"
#define HEART "♥"

const char text[] =
"The GNOME project provides three things:\n"
"\n"
"  • The GNOME desktop environment\n"
"  • The GNOME development platform\n"
"  • Planet GNOME\n"
"  ♥ Lots of love";

typedef struct {
  double width, height;
  const char *path;
} MiniSvg;

static MiniSvg GnomeFootLogo = {
  96.2152, 118.26,
  "M 86.068,1 C 61.466,0 56.851,35.041 70.691,35.041 C 84.529,35.041 110.671,0 86.068,0 z "
  "M 45.217,30.699 C 52.586,31.149 60.671,2.577 46.821,4.374 C 32.976,6.171 37.845,30.249 45.217,30.699 z "
  "M 11.445,48.453 C 16.686,46.146 12.12,23.581 3.208,29.735 C -5.7,35.89 6.204,50.759 11.445,48.453 z "
  "M 26.212,36.642 C 32.451,35.37 32.793,9.778 21.667,14.369 C 10.539,18.961 19.978,37.916 26.212,36.642 L 26.212,36.642 z "
  "M 58.791,93.913 C 59.898,102.367 52.589,106.542 45.431,101.092 C 22.644,83.743 83.16,75.088 79.171,51.386 C 75.86,31.712 15.495,37.769 8.621,68.553 C 3.968,89.374 27.774,118.26 52.614,118.26 C 64.834,118.26 78.929,107.226 81.566,93.248 C 83.58,82.589 57.867,86.86 58.791,93.913 L 58.791,93.913 z "
  "\0"
};

static void
mini_svg_render (MiniSvg  *shape,
                 cairo_t  *cr,
                 gboolean  do_path)
{
  double x, y;
  const char *p;
  char op[2];
  int len;
  double x1, y1, x2, y2, x3, y3;

  cairo_get_current_point (cr, &x, &y);
  cairo_translate (cr, x, y);

  for (p = shape->path; sscanf (p, "%1s %n", op, &len), p += len, *p;)
    switch (*op)
      {
      case 'M':
        sscanf (p, "%lf,%lf %n", &x, &y, &len); p += len;
        cairo_move_to (cr, x, y);
        break;
      case 'L':
        sscanf (p, "%lf,%lf %n", &x, &y, &len); p += len;
        cairo_line_to (cr, x, y);
        break;
      case 'C':
        sscanf (p, "%lf,%lf %lf,%lf %lf,%lf %n", &x1, &y1, &x2, &y2, &x3, &y3, &len); p += len;
        cairo_curve_to (cr, x1, y1, x2, y2, x3, y3);
        break;
      case 'z':
        cairo_close_path (cr);
        break;
      default:
        g_warning ("Invalid MiniSvg operation '%c'", *op);
        break;
      }

  if (!do_path)
    cairo_fill (cr);
}

static PangoLayout *
get_layout (cairo_t *cr)
{
  PangoContext *context;
  PangoLayout *layout;
  PangoAttrList *attrs;
  const char *p;

  /* Create a PangoLayout, set the font and text */
  context = pango_font_map_create_context (fontmap);
  layout = pango_layout_new (context);
  g_object_unref (context);

  pango_layout_set_text (layout, text, -1);

  attrs = pango_attr_list_new ();

  PangoFontDescription *font_desc = pango_font_description_from_string ("Bullets 12");

  for (p = text; (p = strstr (p, BULLET)); p += strlen (BULLET))
    {
      PangoAttribute *attr;

      attr = pango_attr_font_desc_new (font_desc);
      attr->start_index = p - text;
      attr->end_index = attr->start_index + strlen (BULLET);
      pango_attr_list_insert (attrs, attr);
    }

  for (p = text; (p = strstr (p, HEART)); p += strlen (HEART))
    {
      PangoAttribute *attr;

      attr = pango_attr_font_desc_new (font_desc);
      attr->start_index = p - text;
      attr->end_index = attr->start_index + strlen (HEART);
      pango_attr_list_insert (attrs, attr);

      attr = pango_attr_fallback_new (0);
      attr->start_index = p - text;
      attr->end_index = attr->start_index + strlen (HEART);
      pango_attr_list_insert (attrs, attr);
    }

  pango_font_description_free (font_desc);

  pango_layout_set_attributes (layout, attrs);
  pango_attr_list_unref (attrs);


  return layout;
}

static void
draw_text (cairo_t *cr, int *width, int *height)
{
  PangoLayout *layout = get_layout (cr);
  PangoLines *lines = pango_layout_get_lines (layout);

  pango_layout_write_to_file (layout, "out.layout");

  /* Adds a fixed 10-pixel margin on the sides. */

  if (width || height)
    {
      PangoRectangle ext;
      pango_lines_get_extents (lines, NULL, &ext);
      pango_extents_to_pixels (&ext, NULL);
      if (width)
        *width = ext.width + 20;
      if (height)
        *height = ext.height + 20;
    }

  cairo_move_to (cr, 10, 10);
  pango_cairo_show_lines (cr, lines);

  g_object_unref (layout);
}

static gboolean
glyph_cb (PangoUserFace  *face,
          hb_codepoint_t  unicode,
          hb_codepoint_t *glyph,
          gpointer        data)
{
  if (unicode == 0x2022 || /* bullet */
      unicode == 0x2665)   /* heart */
    {
      *glyph = unicode;
      return TRUE;
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
  if (glyph == 0x2022 || glyph == 0x2665)
    {
      extents->x_bearing = 0;
      extents->y_bearing = - size;
      extents->width = size;
      extents->height = size;

      *h_advance = size;
      *v_advance = size;

      *is_color = glyph == 0x2665;

      return TRUE;
    }

  return FALSE;
}

static gboolean
font_info_cb (PangoUserFace     *face,
              int                size,
              hb_font_extents_t *extents,
              gpointer           user_data)
{
  extents->ascender = size;
  extents->descender = 0;
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
  cairo_t *cr = backend_data;

  if (strcmp (backend_id, "cairo") != 0)
    return FALSE;

  if (glyph == 0x2022)
    {
      MiniSvg *shape = &GnomeFootLogo;

      cairo_move_to (cr, 0, -1);
      cairo_scale (cr, 1. / shape->width, 1. / shape->height);

      mini_svg_render (shape, cr, FALSE);
    }
  else if (glyph == 0x2665)
    {
      cairo_set_source_rgb (cr, 1., 0., 0.);

      cairo_move_to (cr, .5, .0);
      cairo_line_to (cr, .9, -.4);
      cairo_curve_to (cr, 1.1, -.8, .5, -.9, .5, -.5);
      cairo_curve_to (cr, .5, -.9, -.1, -.8, .1, -.4);
      cairo_close_path (cr);
      cairo_fill (cr);
    }

  return TRUE;
}

static void
setup_fontmap (PangoHbFontMap *fontmap)
{
  PangoFontDescription *desc;
  PangoUserFace *face;

  desc = pango_font_description_new ();
  pango_font_description_set_family (desc, "Bullets");

  face = pango_user_face_new (font_info_cb,
                              glyph_cb,
                              glyph_info_cb,
                              NULL,
                              render_cb,
                              NULL, NULL, "Black", desc);
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
  int width, height;

  if (argc != 2)
    {
      g_printerr ("Usage: cairoshape OUTPUT_FILENAME\n");
      return 1;
    }

  filename = argv[1];

  fontmap = PANGO_FONT_MAP (pango_fc_hb_font_map_new ());
  setup_fontmap (PANGO_HB_FONT_MAP (fontmap));

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 0, 0);
  cr = cairo_create (surface);
  draw_text (cr, &width, &height);
  cairo_destroy (cr);
  cairo_surface_destroy (surface);

  /* Now create the final surface and draw to it. */
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  cr = cairo_create (surface);

  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_paint (cr);
  cairo_set_source_rgb (cr, 0.0, 0.0, 0.5);
  draw_text (cr, NULL, NULL);
  cairo_destroy (cr);

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
