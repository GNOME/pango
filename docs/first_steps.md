---
Title: First steps with Pango
---

# First steps with Pango

Pango has a rich API, and it can be hard to figure out what is
important, and where to begin. To help with this, here is an
example that aims to show the simplest possible Pango program
that renders some text into a png image. You can use the example
as a basis to explore more parts of the API that are not covered
here.

Useful next steps would be:

- Adding attributes to the text
- Using different languages in the same paragraph
- Making parts of the text use a different font
- Make Pango wrap the text into multiple lines

## A simple example

```
#include <math.h>
#include <pango2/pangocairo.h>

#define SIZE 150

static void
draw_text (cairo_t *cr)
{
  Pango2Layout *layout;
  Pango2FontDescription *desc;

  /* Create a Pango2Layout */
  layout = pango2_cairo_create_layout (cr);

  /* Set the text */
  pango2_layout_set_text (layout, "Text", -1);

  /* Set a font for the layout */
  desc = pango2_font_description_from_string ("Sans Bold 20");
  pango2_layout_set_font_description (layout, desc);
  pango2_font_description_free (desc);

  /* Show the layout */
  pango2_cairo_show_layout (cr, layout);

  /* Free the layout object */
  g_object_unref (layout);
}

int
main (int argc, char **argv)
{
  cairo_t *cr;
  char *filename;
  cairo_status_t status;
  cairo_surface_t *surface;

  if (argc != 2)
    {
      g_printerr ("Usage: first-steps OUTPUT_FILENAME\n");
      return 1;
    }

  filename = argv[1];

  /* Prepare a cairo surface to draw on */
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                        2 * SIZE, 2 * SIZE);
  cr = cairo_create (surface);

  /* White background */
  cairo_set_source_rgb (cr, 1., 1., 1.);
  cairo_paint (cr);

  /* Center coordinates on the middle of the region we are drawing */
  cairo_translate (cr, SIZE, SIZE);

  /* Draw the text, in black */
  cairo_set_source_rgb (cr, 0., 0., 0.);

  draw_text (cr);

  cairo_destroy (cr);

#ifdef CAIRO_HAS_PNG_FUNCTIONS
  status = cairo_surface_write_to_png (surface, filename);
#else
  status = CAIRO_STATUS_PNG_ERROR; /* Not technically correct, but... */
#endif

  cairo_surface_destroy (surface);

  if (status != CAIRO_STATUS_SUCCESS)
    {
      g_printerr ("Could not save png to '%s'\n", filename);
      return 1;
    }

  return 0;
}
```

Once you build and run the example code above, you should see the
following result:

![Output of example](first-steps.png#frame)
