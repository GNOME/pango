#include <math.h>
#include <pango/pangocairo.h>
#include <cairo-png.h>

static void
draw_text (cairo_t *cr)
{
#define RADIUS 150
#define N_WORDS 10
#define FONT "Sans Bold 27"

  PangoLayout *layout;
  PangoFontDescription *desc;
  int i;

  /* Center coordinates on the middle of the region we are drawing
   */
  cairo_translate (cr, RADIUS, RADIUS);

  /* Create a PangoLayout, set the font and text */
  layout = pango_cairo_create_layout (cr);
  
  pango_layout_set_text (layout, "Text", -1);
  desc = pango_font_description_from_string (FONT);
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);

  /* Draw the layout N_WORDS times in a circle */
  for (i = 0; i < N_WORDS; i++)
    {
      int width, height;
      double angle = (360. * i) / N_WORDS;
      double red;

      cairo_save (cr);

      /* Gradient from red at angle == 60 to blue at angle == 300 */
      red   = (1 + cos ((angle - 60) * G_PI / 180.)) / 2;
      cairo_set_rgb_color (cr, red, 0, 1.0 - red);

      cairo_rotate (cr, angle * G_PI / 180.);
    
      /* Inform Pango to re-layout the text with the new transformation */
      pango_cairo_update_layout (cr, layout);
    
      pango_layout_get_size (layout, &width, &height);
      cairo_move_to (cr, - ((double)width / PANGO_SCALE) / 2, - RADIUS);
      pango_cairo_show_layout (cr, layout);

      cairo_restore (cr);
    }

  /* free the layout object */
  g_object_unref (layout);
}

int main (int argc, char **argv)
{
  cairo_t *cr;
  FILE *f;

  if (argc != 2)
    {
      g_printerr ("Usage: cairosimple OUTPUT_FILENAME\n");
      return 1;
    }

  f = fopen (argv[1], "wb");
  if (!f)
    {
      g_printerr ("Usage: cannot open '%s'\n", argv[1]);
      return 1;
    }

  cr = cairo_create();
  cairo_set_target_image_no_data (cr,
				  CAIRO_FORMAT_ARGB32,
				  2 * RADIUS, 2 * RADIUS);

  cairo_set_rgb_color (cr, 1.0, 1.0, 1.0);
  cairo_rectangle (cr, 0, 0, 2 * RADIUS, 2 * RADIUS);
  cairo_fill (cr);
  draw_text (cr);
  
  cairo_surface_write_png (cairo_get_target_surface (cr), f);
  
  cairo_destroy (cr);
  fclose (f);

  return 0;
}
