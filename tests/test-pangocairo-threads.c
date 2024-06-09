#include <stdlib.h>
#include <string.h>
#include <pango/pango.h>
#include <pango/pangocairo.h>

#define WIDTH 100
#define HEIGHT 100
const char *text = "Hamburgerfonts\nวิวิวิวิวิวิ\nبهداد";

int num_iters = 50;
int num_threads = 5;

GMutex mutex;

static cairo_surface_t *
create_surface (void)
{
  return cairo_image_surface_create (CAIRO_FORMAT_A8, WIDTH, HEIGHT);
}

static PangoLayout *
create_layout (cairo_t *cr)
{
  PangoLayout *layout = pango_cairo_create_layout (cr);
  pango_layout_set_text (layout, text, -1);
  pango_layout_set_width (layout, WIDTH * PANGO_SCALE);
  return layout;
}

static void
draw (cairo_t *cr, PangoLayout *layout, unsigned int i)
{
  cairo_set_source_rgba (cr, 1, 1, 1, 1);
  cairo_paint (cr);
  cairo_set_source_rgba (cr, 1, 1, 1, 0);
  cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

  cairo_identity_matrix (cr);
  cairo_scale (cr, (100 + i) / 100.,  (100 + i) / 100.);
  pango_cairo_update_layout (cr, layout);

  pango_cairo_show_layout (cr, layout);
}

static gpointer
thread_func (gpointer data)
{
  cairo_surface_t *surface = data;
  PangoLayout *layout;
  int i;

  cairo_t *cr = cairo_create (surface);

  layout = create_layout (cr);

  g_mutex_lock (&mutex);
  g_mutex_unlock (&mutex);

  for (i = 0; i < num_iters; i++)
    draw (cr, layout, i);

  g_object_unref (layout);

  cairo_destroy (cr);

  return 0;
}

static void
pangocairo_threads (void)
{
  GPtrArray *threads = g_ptr_array_new ();
  GPtrArray *surfaces = g_ptr_array_new ();
  int i;

  g_mutex_lock (&mutex);

  for (i = 0; i < num_threads; i++)
    {
      char buf[10];
      cairo_surface_t *surface = create_surface ();
      g_ptr_array_add (surfaces, surface);
      g_snprintf (buf, sizeof (buf), "%d", i);
      g_ptr_array_add (threads,
		       g_thread_new (buf,
				     thread_func,
				     surface));
    }

  /* Let them loose! */
  g_mutex_unlock (&mutex);

  for (i = 0; i < num_threads; i++)
    g_thread_join (g_ptr_array_index (threads, i));

  g_ptr_array_unref (threads);

  /* Now, draw a reference image and check results. */
  {
    cairo_surface_t *ref_surface = create_surface ();
    cairo_t *cr = cairo_create (ref_surface);
    PangoLayout *layout = create_layout (cr);
    unsigned char *ref_data = cairo_image_surface_get_data (ref_surface);
    unsigned int len = WIDTH * HEIGHT;

    draw (cr, layout, num_iters - 1);

    g_object_unref (layout);
    cairo_destroy (cr);

    /* cairo_surface_write_to_png (ref_surface, "test-pangocairo-threads-reference.png"); */

    g_assert_cmpint (WIDTH, ==, cairo_format_stride_for_width (CAIRO_FORMAT_A8, WIDTH));

    for (i = 0; i < num_threads; i++)
      {
	cairo_surface_t *surface = g_ptr_array_index (surfaces, i);
	unsigned char *data = cairo_image_surface_get_data (surface);
	if (memcmp (ref_data, data, len))
	  {
	    g_test_message ("image for thread %d different from reference image", i);
	    cairo_surface_write_to_png (ref_surface, "test-pangocairo-threads-reference.png");
	    cairo_surface_write_to_png (surface, "test-pangocairo-threads-failed.png");
	    g_test_fail ();
            break;
	  }
	cairo_surface_destroy (surface);
      }

    cairo_surface_destroy (ref_surface);
  }

  g_ptr_array_unref (surfaces);

  pango_cairo_font_map_set_default (NULL);

}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  if (argc > 1)
    num_threads = atoi (argv[1]);
  if (argc > 2)
    num_iters = atoi (argv[2]);

  g_test_add_func ("/pangocairo/threads", pangocairo_threads);

  return g_test_run ();
}
