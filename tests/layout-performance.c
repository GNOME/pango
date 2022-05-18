#include <pango/pango.h>
#include <pango/pangocairo.h>

#define WARM_UP_N_RUNS 50
#define ESTIMATE_ROUND_TIME_N_RUNS 5
#define DEFAULT_TEST_TIME 15 /* seconds */
 /* The time we want each round to take, in seconds, this should
  * be large enough compared to the timer resolution, but small
  * enough that the risk of any random slowness will miss the
  * running window */
#define TARGET_ROUND_TIME 0.008

static gboolean verbose = FALSE;
static int test_length = DEFAULT_TEST_TIME;
static char *filename;

static GOptionEntry cmd_entries[] = {
  {"data", 0, 0, G_OPTION_ARG_FILENAME, &filename, "Test data", "FILE"},
  {"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
   "Print extra information", NULL},
  {"seconds", 's', 0, G_OPTION_ARG_INT, &test_length,
   "Time to run each test in seconds", NULL},
  { NULL, 0, },
};
typedef struct _PerformanceTest PerformanceTest;
struct _PerformanceTest {
  const char *name;
  gpointer extra_data;

  gpointer (*setup) (PerformanceTest *test);
  void (*init) (PerformanceTest *test,
                gpointer data,
                double factor);
  void (*run) (PerformanceTest *test,
               gpointer data);
  void (*finish) (PerformanceTest *test,
                  gpointer data);
  void (*teardown) (PerformanceTest *test,
                    gpointer data);
  void (*print_result) (PerformanceTest *test,
                        gpointer data,
                        double time);
};

static void
run_test (PerformanceTest *test)
{
  gpointer data = NULL;
  guint64 i, num_rounds;
  double elapsed, min_elapsed, max_elapsed, avg_elapsed, factor;
  GTimer *timer;

  g_print ("Running test %s\n", test->name);

  /* Set up test */
  timer = g_timer_new ();
  data = test->setup (test);

  if (verbose)
    g_print ("Warming up\n");

  g_timer_start (timer);

  /* Warm up the test by doing a few runs */
  for (i = 0; i < WARM_UP_N_RUNS; i++)
    {
      test->init (test, data, 1.0);
      test->run (test, data);
      test->finish (test, data);
    }

  g_timer_stop (timer);
  elapsed = g_timer_elapsed (timer, NULL);

  if (verbose)
    {
      g_print ("Warm up time: %.2f secs\n", elapsed);
      g_print ("Estimating round time\n");
    }

  /* Estimate time for one run by doing a few test rounds */
  min_elapsed = 0;
  for (i = 0; i < ESTIMATE_ROUND_TIME_N_RUNS; i++)
    {
      test->init (test, data, 1.0);
      g_timer_start (timer);
      test->run (test, data);
      g_timer_stop (timer);
      test->finish (test, data);

      elapsed = g_timer_elapsed (timer, NULL);
      if (i == 0)
        min_elapsed = elapsed;
      else
        min_elapsed = MIN (min_elapsed, elapsed);
    }

  factor = TARGET_ROUND_TIME / min_elapsed;

  if (verbose)
    g_print ("Uncorrected round time: %.4f msecs, correction factor %.2f\n", 1000*min_elapsed, factor);

  /* Calculate number of rounds needed */
  num_rounds = (test_length / TARGET_ROUND_TIME) + 1;

  if (verbose)
    g_print ("Running %"G_GINT64_MODIFIER"d rounds\n", num_rounds);


  /* Run the test */
  avg_elapsed = 0.0;
  min_elapsed = 0.0;
  max_elapsed = 0.0;
  for (i = 0; i < num_rounds; i++)
    {
      test->init (test, data, factor);
      g_timer_start (timer);
      test->run (test, data);
      g_timer_stop (timer);
      test->finish (test, data);
      elapsed = g_timer_elapsed (timer, NULL);

      if (i == 0)
        max_elapsed = min_elapsed = avg_elapsed = elapsed;
      else
        {
          min_elapsed = MIN (min_elapsed, elapsed);
          max_elapsed = MAX (max_elapsed, elapsed);
          avg_elapsed += elapsed;
        }
    }

  if (num_rounds > 1)
    avg_elapsed = avg_elapsed / num_rounds;

  if (verbose)
    {
      g_print ("Minimum corrected round time: %.2f msecs\n", min_elapsed * 1000);
      g_print ("Maximum corrected round time: %.2f msecs\n", max_elapsed * 1000);
      g_print ("Average corrected round time: %.2f msecs\n", avg_elapsed * 1000);
    }

  /* Print the results */
  test->print_result (test, data, min_elapsed);

  /* Tear down */
  test->teardown (test, data);
  g_timer_destroy (timer);
}

const char default_text[] = 
  "'Twas brillig, and the slithy toves\n"
  "Did gyre and gimble in the wabe;\n"
  "All mimsy were the borogoves,\n"
  "And the mome raths outgrabe.\n"
  "\n"
  "'Beware the Jabberwock, my son!\n"
  "The jaws that bite, the claws that catch!\n"
  "Beware the Jubjub bird, and shun\n"
  "The frumious Bandersnatch!'";

static void
get_test_text (char          **text,
               PangoAttrList **attrs)
{
  if (filename)
    {
      char *buffer;
      gsize len;
      GError *error = NULL;

      if (!g_file_get_contents (filename, &buffer, &len, &error))
        {
          g_error ("%s", error->message);
          exit (1);
        }

      if (!pango_parse_markup (buffer, len, 0, attrs, text, NULL, &error))
        {
          g_error ("%s", error->message);
          exit (1);
        }

      return;
    }

  *text = g_strdup (default_text);
  *attrs = NULL;
}

#define NUM_OBJECT_TO_CONSTRUCT 10000

typedef struct _CreateTest CreateTest;
struct _CreateTest {
  PangoContext *context;
  PangoLayout **layouts;
  int n_layouts;
  char *text;
  PangoAttrList *attrs;
};

static gpointer
test_create_setup (PerformanceTest *test)
{
  CreateTest *data;
  PangoFontMap *fontmap;

  data = g_new0 (CreateTest, 1);

  fontmap = pango_cairo_font_map_get_default ();
  data->context = pango_font_map_create_context (fontmap);

  get_test_text (&data->text, &data->attrs);

  return data;
}

static void
test_create_init (PerformanceTest *test,
                  gpointer _data,
                  double count_factor)
{
  CreateTest *data = _data;
  int n;

  n = NUM_OBJECT_TO_CONSTRUCT * count_factor;
  if (data->n_layouts != n)
    {
      data->n_layouts = n;
      data->layouts = g_new (PangoLayout *, n);
    }
}

static void
test_create_run (PerformanceTest *test,
                 gpointer _data)
{
  CreateTest *data = _data;
  PangoContext *context = data->context;
  PangoLayout **layouts = data->layouts;
  int n_layouts = data->n_layouts;
  int i;

  for (i = 0; i < n_layouts; i++)
    {
      layouts[i] = pango_layout_new (context);
      pango_layout_set_text (layouts[i], data->text, -1);
      pango_layout_set_attributes (layouts[i], data->attrs);
    }
}

static void
test_create_finish (PerformanceTest *test,
                    gpointer _data)
{
  CreateTest *data = _data;
  int i;

  for (i = 0; i < data->n_layouts; i++)
    g_object_unref (data->layouts[i]);
}

static void
test_create_teardown (PerformanceTest *test,
                      gpointer _data)
{
  CreateTest *data = _data;
  g_object_unref (data->context);
  g_free (data->layouts);
  g_free (data->text);
  pango_attr_list_unref (data->attrs);
  g_free (data);
}

static void
test_create_print_result (PerformanceTest *test,
                          gpointer _data,
                          double time)
{
  CreateTest *data = _data;

  g_print ("Millions of constructed layouts per second: %.3f\n",
           data->n_layouts / (time * 1000000));
}

/* Test shaping */

typedef struct _LayoutTest LayoutTest;
struct _LayoutTest {
  PangoContext *context;
  PangoLayout *layout;
  int n_layouts;
};

static gpointer
test_layout_setup (PerformanceTest *test)
{
  LayoutTest *data;
  PangoFontMap *fontmap;
  char *text;
  PangoAttrList *attrs;

  data = g_new0 (LayoutTest, 1);

  fontmap = pango_cairo_font_map_get_default ();
  data->context = pango_font_map_create_context (fontmap);

  data->layout = pango_layout_new (data->context);

  get_test_text (&text, &attrs);

  pango_layout_set_text (data->layout, text, -1);
  pango_layout_set_attributes (data->layout, attrs);

  g_free (text);
  pango_attr_list_unref (attrs);

  return data;
}

static void
test_layout_init (PerformanceTest *test,
                  gpointer _data,
                  double count_factor)
{
  LayoutTest *data = _data;
  int n;

  n = NUM_OBJECT_TO_CONSTRUCT * count_factor;
  if (data->n_layouts != n)
    data->n_layouts = n;
}

static void
test_ellipsis_init (PerformanceTest *test,
                    gpointer _data,
                    double count_factor)
{
  LayoutTest *data = _data;

  test_layout_init (test, _data, count_factor);

  pango_layout_set_ellipsize (data->layout, PANGO_ELLIPSIZE_MIDDLE);
}

static void
test_layout_run (PerformanceTest *test,
                 gpointer _data)
{
  LayoutTest *data = _data;
  PangoLayout *layout = data->layout;
  int n_layouts = data->n_layouts;
  int i;
  int width;

  for (i = 0; i < n_layouts; i++)
    {
      width = g_random_int_range (50, 200);
      pango_layout_set_width (layout, width * PANGO_SCALE);
      if (pango_layout_get_line_count (layout) <= 0)
        g_printerr ("layout produced no lines\n");
    }
}

static void
test_layout_finish (PerformanceTest *test,
                    gpointer _data)
{
}

static void
test_layout_teardown (PerformanceTest *test,
                      gpointer _data)
{
  LayoutTest *data = _data;
  g_object_unref (data->layout);
  g_object_unref (data->context);
  g_free (data);
}

static void
test_layout_print_result (PerformanceTest *test,
                          gpointer _data,
                          double time)
{
  LayoutTest *data = _data;

  g_print ("Re-layouts per second: %.3f\n",
           data->n_layouts / (time));
}

static PerformanceTest tests[] = {
  {
    "create-layout",
    0,
    test_create_setup,
    test_create_init,
    test_create_run,
    test_create_finish,
    test_create_teardown,
    test_create_print_result
  },
  {
    "shape-layout",
    0,
    test_layout_setup,
    test_layout_init,
    test_layout_run,
    test_layout_finish,
    test_layout_teardown,
    test_layout_print_result
  },
  {
    "shape-with-ellipsis",
    0,
    test_layout_setup,
    test_ellipsis_init,
    test_layout_run,
    test_layout_finish,
    test_layout_teardown,
    test_layout_print_result
  },
};

static PerformanceTest *
find_test (const char *name)
{
  gsize i;
  for (i = 0; i < G_N_ELEMENTS (tests); i++)
    {
      if (strcmp (tests[i].name, name) == 0)
        return &tests[i];
    }
  return NULL;
}

int
main (int argc, char *argv[])
{
  PerformanceTest *test;
  GOptionContext *context;
  GError *error = NULL;
  int i;

  context = g_option_context_new ("Pango performance tests");
  g_option_context_add_main_entries (context, cmd_entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_printerr ("%s: %s\n", argv[0], error->message);
      return 1;
    }

  if (argc > 1)
    {
      for (i = 1; i < argc; i++)
        {
          test = find_test (argv[i]);
          if (test)
            run_test (test);
          else
            g_printerr ("No such test: %s\n", argv[i]);
        }
    }
  else
    {
      gsize k;
      for (k = 0; k < G_N_ELEMENTS (tests); k++)
        run_test (&tests[k]);
    }

  return 0;
}
