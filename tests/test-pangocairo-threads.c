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

  cairo_surface_t *surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, 100, 100);
  cairo_t *cr = cairo_create (surface);
  PangoLayout *layout = pango_cairo_create_layout (cr);

  pango_layout_set_text (layout, text, -1);
  pango_layout_set_width (layout, 100 * PANGO_SCALE);

  for (i = 0; i < num_iters; i++)
    {
      cairo_set_source_rgb (cr, 1, 1, 1);
      cairo_paint (cr);
      cairo_set_source_rgb (cr, 0, 0, 0);

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

  g_type_init ();
  g_thread_init (NULL);

  for (i = 0; i < num_threads; i++)
    g_ptr_array_add (threads,
		     g_thread_new (g_strdup_printf ("%d", i + 1),
				   thread_func,
				   GINT_TO_POINTER (i+1)));

  for (i = 0; i < num_threads; i++)
    g_thread_join (g_ptr_array_index (threads, i));

  return 0;
}
