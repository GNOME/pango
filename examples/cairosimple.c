/* Simple example to use pangocairo to render rotated text */

#include <math.h>
#include <pango/pangocairo.h>

static void
draw_text (cairo_t *cr)
{
#define RADIUS 200
#define N_WORDS 8
#define FONT_WITH_MANUAL_SIZE "Times new roman,Sans"
#define FONT_SIZE 36
#define DEVICE_DPI 72

/* The following number applies a cairo CTM.  Tests for
 * https://bugzilla.gnome.org/show_bug.cgi?id=700592
 */
#define TWEAKABLE_SCALE ((double) 0.1)

  PangoLayout *layout;
  PangoLines *lines;
  PangoFontDescription *desc;
  int i;

  /* Center coordinates on the middle of the region we are drawing
   */
  cairo_translate (cr, RADIUS / TWEAKABLE_SCALE, RADIUS / TWEAKABLE_SCALE);

  /* Create a PangoLayout, set the font and text */
  layout = pango_cairo_create_layout (cr);

  pango_layout_set_text (layout, "Test\nسَلام", -1);

  desc = pango_font_description_from_string (FONT_WITH_MANUAL_SIZE);
  pango_font_description_set_absolute_size (desc, FONT_SIZE * DEVICE_DPI * PANGO_SCALE / (72.0 * TWEAKABLE_SCALE));
  //pango_font_description_set_size(desc, 27 * PANGO_SCALE / TWEAKABLE_SCALE);

  printf("PANGO_SCALE = %d\n", PANGO_SCALE);
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);

  /* Draw the layout N_WORDS times in a circle */
  for (i = 0; i < N_WORDS; i++)
    {
      PangoRectangle ext;
      double angle = (360. * i) / N_WORDS;
      double red;

      cairo_save (cr);

      /* Gradient from red at angle == 60 to blue at angle == 240 */
      red   = (1 + cos ((angle - 60) * G_PI / 180.)) / 2;
      cairo_set_source_rgb (cr, red, 0, 1.0 - red);

      cairo_rotate (cr, angle * G_PI / 180.);

      /* Inform Pango to re-layout the text with the new transformation */
      pango_cairo_update_layout (cr, layout);

      lines = pango_layout_get_lines (layout);

      pango_lines_get_extents (lines, NULL, &ext);
      cairo_move_to (cr,( - (((double)ext.width) / PANGO_SCALE) / 2.0) , (- RADIUS)  / TWEAKABLE_SCALE);
      pango_cairo_show_lines (cr, lines);

      cairo_restore (cr);
    }

  /* free the layout object */
  g_object_unref (layout);
}

int main (int argc, char **argv)
{
  cairo_t *cr;
  char *filename;
  cairo_status_t status;
  cairo_surface_t *surface;

  if (argc != 2)
    {
      g_printerr ("Usage: cairosimple OUTPUT_FILENAME\n");
      return 1;
    }

     filename = argv[1];

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                        2 * RADIUS, 2 * RADIUS);
  cr = cairo_create (surface);

  cairo_scale(cr, 1 * TWEAKABLE_SCALE, 1 * TWEAKABLE_SCALE);

  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_paint (cr);
  draw_text (cr);
  cairo_destroy (cr);

  status = cairo_surface_write_to_png (surface, filename);
  cairo_surface_destroy (surface);

  if (status != CAIRO_STATUS_SUCCESS)
    {
      g_printerr ("Could not save png to '%s'\n", filename);
      return 1;
    }

  return 0;
}
