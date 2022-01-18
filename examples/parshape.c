#include <pango/pango.h>
#include <pango/pangocairo.h>

int
main (int argc, char *argv[])
{
  const char *filename;
  PangoContext *context;
  PangoLineBreaker *breaker;
  int x, y, width;
  PangoLines *lines;
  int inc, m, w, w2;
  cairo_surface_t *surface;
  cairo_t *cr;
  char *text;
  gsize length;
  GError *error = NULL;

  if (argc != 3)
    {
      g_printerr ("Usage: %s INPUT_FILENAME OUTPUT_FILENAME\n", argv[0]);
      return 1;
    }

  if (!g_file_get_contents (argv[1], &text, &length, &error))
    {
      g_printerr ("%s\n", error->message);
      return 1;
    }

  filename = argv[2];

  context = pango_font_map_create_context (pango_cairo_font_map_get_default ());

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 400, 600);
  cr = cairo_create (surface);
  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);
  cairo_set_source_rgb (cr, 0, 0, 0);

  breaker = pango_line_breaker_new (context);

  g_print ("Using %s\n", G_OBJECT_TYPE_NAME (breaker));

  pango_line_breaker_add_text (breaker, text, -1, NULL);

  lines = pango_lines_new ();

  m = 200;
  w = 10;
  w2 = -200;
  inc = 40;

  y = 40 * PANGO_SCALE;
  x = (m - w / 2) * PANGO_SCALE;
  width = w * PANGO_SCALE;

  while (pango_line_breaker_has_line (breaker))
    {
      PangoLine *line;
      PangoRectangle ext;
      gboolean ltr;

      line = pango_line_breaker_next_line (breaker,
                                           x, width,
                                           PANGO_WRAP_CHAR,
                                           PANGO_ELLIPSIZE_NONE);

      pango_line_get_extents (line, NULL, &ext);
      line = pango_line_justify (line, width);
      pango_lines_add_line (lines, line, x, y - ext.y);

      ltr = pango_line_breaker_get_direction (breaker) == PANGO_DIRECTION_LTR;

      if (w2 > 0 && ltr && x <= m * PANGO_SCALE)
        x = (m + w2 / 2) * PANGO_SCALE;
      else if (w2 > 0 && !ltr && x > m * PANGO_SCALE)
        x = (m - w2 / 2) * PANGO_SCALE;
      else
        {
          y += ext.height;

          w += inc;
          w2 += inc;
          if (w + inc >= 340 || w + inc < 0)
            inc = - inc;

          if (w2 > 0)
            width = ((w - w2) / 2) * PANGO_SCALE;
          else
            width = w * PANGO_SCALE;

          if (ltr)
            x = (m - w / 2) * PANGO_SCALE;
          else
            x = (m + w / 2) * PANGO_SCALE;
        }
    }

  for (int i = 0; i < pango_lines_get_line_count (lines); i++)
    {
      PangoLine *line = pango_lines_get_line (lines, i, &x, &y);

      cairo_save (cr);
      cairo_move_to (cr, x / (double)PANGO_SCALE, y / (double)PANGO_SCALE);
      pango_cairo_show_line (cr, line);
      cairo_restore (cr);
    }

  cairo_surface_write_to_png (surface, filename);
  g_print ("Output written to %s\n", filename);

  g_object_unref (lines);
  g_object_unref (breaker);

  cairo_surface_destroy (surface);
  cairo_destroy (cr);

  g_object_unref (context);

  return 0;
}
