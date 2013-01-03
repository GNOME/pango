#include <stdlib.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>

const char *text = "The quick brown fox jumped over the lazy dog!";
int num_iters = 100;

static gpointer
thread_func (gpointer data)
{
  int num = GPOINTER_TO_INT (data);
  int i;
  char *filename;

  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, 500, 300);
  cairo_t *cr = cairo_create (surface);
  PangoLayout *layout = pango_cairo_create_layout (cr);

  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);
  cairo_set_source_rgb (cr, 0, 0, 0);

  pango_layout_set_text (layout, text, -1);
  pango_layout_set_width (layout, 500 * PANGO_SCALE);

  for (i = 0; i < num_iters; i++)
    {
      /* force a relayout */
      PangoWrapMode wrap = pango_layout_get_wrap (layout);
      wrap = wrap == PANGO_WRAP_WORD ? PANGO_WRAP_CHAR : PANGO_WRAP_WORD;
      pango_layout_set_wrap (layout, wrap);

      pango_cairo_show_layout (cr, layout);
    }

  filename = g_strdup_printf ("%d.png", num);
  cairo_surface_write_to_png (surface, filename);
  g_free (filename);

  return 0;
}

static void
join_thread (gpointer thread, gpointer self)
{
  if (thread != self)
    g_thread_join (thread);
}


int
main (int argc, char **argv)
{
  int num_threads = 2;
  int i;

  if (argc > 1)
    num_threads = atoi (argv[1]);
  if (argc > 2)
    num_iters = atoi (argv[2]);

  g_type_init ();
  g_thread_init (NULL);

  /* force pango module initializations */
/*  thread_func (GINT_TO_POINTER (0)); */

  for (i = 0; i < num_threads; i++)
    g_thread_create (thread_func, GINT_TO_POINTER (i+1), TRUE, NULL);

  g_thread_foreach (join_thread, g_thread_self ());

  return 0;
}
