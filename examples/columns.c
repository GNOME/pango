#include <pango/pango.h>
#include <pango/pangocairo.h>

int
main (int argc, char *argv[])
{
  const char *filename;
  PangoContext *context;
  PangoLineBreaker *breaker;
  int margin;
  int x, y, width, height;
  int x0, y0;
  int col;
  PangoLines *lines;
  cairo_surface_t *surface;
  cairo_t *cr;
  char *text;
  gsize length;
  PangoAttrList *attrs;
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

  context = pango_font_map_create_context (pango_font_map_get_default ());

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 600, 600);
  cr = cairo_create (surface);
  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);
  cairo_set_source_rgb (cr, 0, 0, 0);

  breaker = pango_line_breaker_new (context);

  g_print ("Using %s\n", G_OBJECT_TYPE_NAME (breaker));

  attrs = pango_attr_list_new ();

  pango_line_breaker_add_text (breaker, text, -1, attrs);

  pango_attr_list_unref (attrs);

  lines = pango_lines_new ();

  /* We do 2 columns, followed by 3 columns, to demonstrate changing
   * line width.
   */
  margin = 20;
  x = x0 = margin * PANGO_SCALE;
  y = y0 = margin * PANGO_SCALE;
  width = (300 - 2 * margin) * PANGO_SCALE;
  height = (300 - margin) * PANGO_SCALE;
  col = 0;

  while (pango_line_breaker_has_line (breaker))
    {
      PangoLine *line;
      PangoRectangle ext;

retry:
      line = pango_line_breaker_next_line (breaker,
                                           x, width,
                                           PANGO_WRAP_CHAR,
                                           PANGO_ELLIPSIZE_NONE);

      if (!pango_line_is_paragraph_end (line))
        line = pango_line_justify (line, width);

      pango_line_get_extents (line, NULL, &ext);

      if (y + ext.height > height)
        {
          gboolean width_changed = FALSE;

          if (col == 0)
            x0 += 300 * PANGO_SCALE;
          else if (col == 1)
            {
              x0 = margin * PANGO_SCALE;
              y0 = (300 + margin) * PANGO_SCALE;
              width = (200 - 2 * margin) * PANGO_SCALE;
              height = (600 - 2 * margin) * PANGO_SCALE;

              width_changed = TRUE;
            }
          else
            x0 += 200 * PANGO_SCALE;

          x = x0;
          y = y0;

          col++;

          if (width_changed &&
              pango_line_breaker_undo_line (breaker, line))
            {
              g_object_unref (line);
              goto retry;
            }
        }

      pango_lines_add_line (lines, line, x, y - ext.y);

      y += ext.height;
    }

  pango_cairo_show_lines (cr, lines);

  cairo_surface_write_to_png (surface, filename);
  g_print ("Output written to %s\n", filename);

  g_object_unref (lines);
  g_object_unref (breaker);

  cairo_surface_destroy (surface);
  cairo_destroy (cr);

  g_object_unref (context);

  g_free (text);

  return 0;
}
