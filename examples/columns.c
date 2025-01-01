#include <pango2/pangocairo.h>

int
main (int argc, char *argv[])
{
  const char *filename;
  Pango2Context *context;
  Pango2LineBreaker *breaker;
  int margin;
  int x, y, width, height;
  int x0, y0;
  int col;
  Pango2Lines *lines;
  cairo_surface_t *surface;
  cairo_t *cr;
  cairo_status_t status;
  char *text;
  gsize length;
  Pango2AttrList *attrs;
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

  context = pango2_context_new ();

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 600, 600);
  cr = cairo_create (surface);
  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);
  cairo_set_source_rgb (cr, 0, 0, 0);

  breaker = pango2_line_breaker_new (context);

  attrs = pango2_attr_list_new ();

  pango2_line_breaker_add_text (breaker, text, -1, attrs);

  pango2_attr_list_unref (attrs);

  lines = pango2_lines_new ();

  /* We do 2 columns, followed by 3 columns, to demonstrate changing
   * line width.
   */
  margin = 20;
  x = x0 = margin * PANGO2_SCALE;
  y = y0 = margin * PANGO2_SCALE;
  width = (300 - 2 * margin) * PANGO2_SCALE;
  height = (300 - margin) * PANGO2_SCALE;
  col = 0;

  while (pango2_line_breaker_has_line (breaker))
    {
      Pango2Line *line;
      Pango2Rectangle ext;

retry:
      line = pango2_line_breaker_next_line (breaker,
                                           x, width,
                                           PANGO2_WRAP_CHAR,
                                           PANGO2_ELLIPSIZE_NONE);

      if (!pango2_line_is_paragraph_end (line))
        line = pango2_line_justify (line, width);

      pango2_line_get_extents (line, NULL, &ext);

      if (y + ext.height > height)
        {
          gboolean width_changed = FALSE;

          if (col == 0)
            x0 += 300 * PANGO2_SCALE;
          else if (col == 1)
            {
              x0 = margin * PANGO2_SCALE;
              y0 = (300 + margin) * PANGO2_SCALE;
              width = (200 - 2 * margin) * PANGO2_SCALE;
              height = (600 - 2 * margin) * PANGO2_SCALE;

              width_changed = TRUE;
            }
          else
            x0 += 200 * PANGO2_SCALE;

          x = x0;
          y = y0;

          col++;

          if (width_changed &&
              pango2_line_breaker_undo_line (breaker, line))
            {
              pango2_line_free (line);
              goto retry;
            }
        }

      pango2_lines_add_line (lines, line, x, y - ext.y);

      y += ext.height;
    }

  pango2_cairo_show_lines (cr, lines);

#ifdef CAIRO_HAS_PNG_FUNCTIONS
  status = cairo_surface_write_to_png (surface, filename);
#else
  status = CAIRO_STATUS_PNG_ERROR; /* Not technically correct, but... */
#endif

  if (status != CAIRO_STATUS_SUCCESS)
    g_printerr ("Could not save png to '%s'\n", filename);
  else
    g_print ("Output written to %s\n", filename);

  g_object_unref (lines);
  g_object_unref (breaker);

  cairo_surface_destroy (surface);
  cairo_destroy (cr);

  g_object_unref (context);

  g_free (text);

  return 0;
}
