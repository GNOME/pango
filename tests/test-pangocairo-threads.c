#include <stdlib.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>

const char *text = "The quick brown fox jumped over the lazy dog!";
int num_iters = 100;

GMutex mutex;

static PangoLayout *
create_layout (cairo_t *cr)
{
  PangoLayout *layout = pango_cairo_create_layout (cr);
  pango_layout_set_text (layout, text, -1);
  pango_layout_set_width (layout, 100 * PANGO_SCALE);
  return layout;
}

static void
draw (cairo_t *cr, PangoLayout *layout)
{
  cairo_set_source_rgba (cr, 1, 1, 1, 1);
  cairo_paint (cr);
  cairo_set_source_rgba (cr, 1, 1, 1, 0);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

  /* force a relayout */
  pango_layout_context_changed (layout);

  pango_cairo_show_layout (cr, layout);
}

static gpointer
thread_func (gpointer data)
{
  int num = GPOINTER_TO_INT (data);
  int i;
  char *filename;
  PangoLayout *layout;

  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_A8, 100, 100);
  cairo_t *cr = cairo_create (surface);

  layout = create_layout (cr);

  g_mutex_lock (&mutex);
  g_mutex_unlock (&mutex);

  for (i = 0; i < num_iters; i++)
    draw (cr, layout);

  filename = g_strdup_printf ("%d.png", num);
  cairo_surface_write_to_png (surface, filename);
  g_free (filename);

  return 0;
}

int
main (int argc, char **argv)
{
  int num_threads = 2;
  int i;
  GPtrArray *threads = g_ptr_array_new ();

  if (argc > 1)
    num_threads = atoi (argv[1]);
  if (argc > 2)
    num_iters = atoi (argv[2]);

  g_mutex_lock (&mutex);

  for (i = 0; i < num_threads; i++)
    g_ptr_array_add (threads,
		     g_thread_new (g_strdup_printf ("%d", i + 1),
				   thread_func,
				   GINT_TO_POINTER (i+1)));

  /* Let them loose! */
  g_mutex_unlock (&mutex);

  for (i = 0; i < num_threads; i++)
    g_thread_join (g_ptr_array_index (threads, i));

  return 0;
}
