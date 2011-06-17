/* Example code to show how to use pangocairo to render arbitrary shapes
 * inside a text layout, positioned by Pango.  This has become possibly
 * using the following API added in Pango 1.18:
 * 
 * 	pango_cairo_context_set_shape_renderer ()
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

#define BULLET "•"

const char text[] =
"The GNOME project provides two things:\n"
"\n"
"  • The GNOME desktop environment\n"
"  • The GNOME development platform\n"
"  • Planet GNOME";

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

  cairo_get_current_point (cr, &x, &y);
  cairo_translate (cr, x, y);

  for (p = shape->path; sscanf (p, "%1s %n", op, &len), p += len, *p;)
    switch (*op)
    {
      case 'M':
        {
	  sscanf (p, "%lf,%lf %n", &x, &y, &len); p += len;
	  cairo_move_to (cr, x, y);
	  break;
	}
      case 'L':
        {
	  sscanf (p, "%lf,%lf %n", &x, &y, &len); p += len;
	  cairo_line_to (cr, x, y);
	  break;
	}
      case 'C':
        {
	  double x1, y1, x2, y2, x3, y3;
	  sscanf (p, "%lf,%lf %lf,%lf %lf,%lf %n", &x1, &y1, &x2, &y2, &x3, &y3, &len); p += len;
	  cairo_curve_to (cr, x1, y1, x2, y2, x3, y3);
	  break;
	}
      case 'z':
        {
	  cairo_close_path (cr);
	  break;
	}
      default: 
        {
	  g_warning ("Invalid MiniSvg operation '%c'", *op);
	  break;
	}
    }

  if (!do_path)
    cairo_fill (cr);
}

static void
mini_svg_shape_renderer (cairo_t        *cr,
			 PangoAttrShape *attr,
			 gboolean        do_path,
			 gpointer        data G_GNUC_UNUSED)
{
  MiniSvg *shape = (MiniSvg *) attr->data;
  double scale_x, scale_y;

  scale_x = (double) attr->ink_rect.width  / (PANGO_SCALE * shape->width );
  scale_y = (double) attr->ink_rect.height / (PANGO_SCALE * shape->height);

  cairo_rel_move_to (cr,
		     (double) attr->ink_rect.x / PANGO_SCALE,
		     (double) attr->ink_rect.y / PANGO_SCALE);
  cairo_scale (cr, scale_x, scale_y);

  mini_svg_render (shape, cr, do_path);
}


static PangoLayout *
get_layout (cairo_t *cr)
{
  PangoLayout *layout;
  PangoAttrList *attrs;
  PangoRectangle ink_rect     = {1 * PANGO_SCALE, -11 * PANGO_SCALE,  8 * PANGO_SCALE, 10 * PANGO_SCALE};
  PangoRectangle logical_rect = {0 * PANGO_SCALE, -12 * PANGO_SCALE, 10 * PANGO_SCALE, 12 * PANGO_SCALE};
  const char *p;

  /* Create a PangoLayout, set the font and text */
  layout = pango_cairo_create_layout (cr);

  pango_cairo_context_set_shape_renderer (pango_layout_get_context (layout),
					  mini_svg_shape_renderer, NULL, NULL);

  pango_layout_set_text (layout, text, -1);

  attrs = pango_attr_list_new ();

  /* Set gnome shape attributes for all bullets */
  for (p = text; (p = strstr (p, BULLET)); p += strlen (BULLET))
    {
      PangoAttribute *attr;
      
      attr = pango_attr_shape_new_with_data (&ink_rect,
					     &logical_rect,
					     &GnomeFootLogo,
					     NULL, NULL);

      attr->start_index = p - text;
      attr->end_index = attr->start_index + strlen (BULLET);

      pango_attr_list_insert (attrs, attr);
    }

  pango_layout_set_attributes (layout, attrs);
  pango_attr_list_unref (attrs);

  return layout;
}

static void
draw_text (cairo_t *cr, int *width, int *height)
{

  PangoLayout *layout = get_layout (cr);

  /* Adds a fixed 10-pixel margin on the sides. */

  if (width || height)
    {
      pango_layout_get_pixel_size (layout, width, height);
      if (width)
        *width += 20;
      if (height)
        *height += 20;
    }

  cairo_move_to (cr, 10, 10);
  pango_cairo_show_layout (cr, layout);

  g_object_unref (layout);
}

int main (int argc, char **argv)
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

  /* First create and use a 0x0 surface, to measure how large
   * the final surface needs to be */
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					0, 0);
  cr = cairo_create (surface);
  draw_text (cr, &width, &height);
  cairo_destroy (cr);
  cairo_surface_destroy (surface);

  /* Now create the final surface and draw to it. */
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					width, height);
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
      g_printerr ("Could not save png to '%s'\n", filename);
      return 1;
    }

  return 0;
}
