#include <config.h>
#include <math.h>
#include <stdlib.h>
#include <pango/pangocairo.h>

void fancy_cairo_stroke (cairo_t *cr);
void fancy_cairo_stroke_preserve (cairo_t *cr);


static double
two_points_distance (cairo_path_data_t *a, cairo_path_data_t *b)
{
  double dx, dy;

  dx = b->point.x - a->point.x;
  dy = b->point.y - a->point.y;

  return sqrt (dx * dx + dy * dy);
}

typedef double parametrization_t;

static parametrization_t *
parametrize_path (cairo_path_t *path)
{
  int i;
  cairo_path_data_t *data, current_point;
  parametrization_t *parametrization;

  parametrization = malloc (path->num_data * sizeof (parametrization[0]));

  for (i=0; i < path->num_data; i += path->data[i].header.length) {
    data = &path->data[i];
    parametrization[i] = 0.0;
    switch (data->header.type) {
    case CAIRO_PATH_MOVE_TO:
	current_point = data[1];
	break;
    case CAIRO_PATH_LINE_TO:
	parametrization[i] = two_points_distance (&current_point, &data[1]);
	current_point = data[1];
	break;
    case CAIRO_PATH_CURVE_TO:
	/*
	parametrization[i] = curve_length (current_point.point.x, current_point.point.x,
					   data[1].point.x, data[1].point.y,
					   data[2].point.x, data[2].point.y,
					   data[3].point.x, data[3].point.y);
					   */
	parametrization[i] = two_points_distance (&current_point, &data[1]);
	parametrization[i] += two_points_distance (&data[1], &data[2]);
	parametrization[i] += two_points_distance (&data[2], &data[3]);

	current_point = data[3];
	break;
    case CAIRO_PATH_CLOSE_PATH:
	break;
    }
  }

  return parametrization;
}

static void
_fancy_cairo_stroke (cairo_t *cr, cairo_bool_t preserve)
{
  int i;
  double line_width;
  cairo_path_t *path;
  cairo_path_data_t *data;
  const double dash[] = {10, 10};

  cairo_save (cr);
  cairo_set_source_rgb (cr, 1.0, 0.0, 0.0);

  line_width = cairo_get_line_width (cr);
  path = cairo_copy_path (cr);
  cairo_new_path (cr);

  cairo_save (cr);
  cairo_set_line_width (cr, line_width / 3);
  cairo_set_dash (cr, dash, G_N_ELEMENTS (dash), 0);
  for (i=0; i < path->num_data; i += path->data[i].header.length) {
    data = &path->data[i];
    switch (data->header.type) {
    case CAIRO_PATH_MOVE_TO:
    case CAIRO_PATH_LINE_TO:
	cairo_move_to (cr, data[1].point.x, data[1].point.y);
	break;
    case CAIRO_PATH_CURVE_TO:
	cairo_line_to (cr, data[1].point.x, data[1].point.y);
	cairo_move_to (cr, data[2].point.x, data[2].point.y);
	cairo_line_to (cr, data[3].point.x, data[3].point.y);
	break;
    case CAIRO_PATH_CLOSE_PATH:
	break;
    }
  }
  cairo_stroke (cr);
  cairo_restore (cr);

  cairo_save (cr);
  cairo_set_line_width (cr, line_width * 4);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  for (i=0; i < path->num_data; i += path->data[i].header.length) {
    data = &path->data[i];
    switch (data->header.type) {
    case CAIRO_PATH_MOVE_TO:
	cairo_move_to (cr, data[1].point.x, data[1].point.y);
	break;
    case CAIRO_PATH_LINE_TO:
	cairo_rel_line_to (cr, 0, 0);
	cairo_move_to (cr, data[1].point.x, data[1].point.y);
	break;
    case CAIRO_PATH_CURVE_TO:
	cairo_rel_line_to (cr, 0, 0);
	cairo_move_to (cr, data[1].point.x, data[1].point.y);
	cairo_rel_line_to (cr, 0, 0);
	cairo_move_to (cr, data[2].point.x, data[2].point.y);
	cairo_rel_line_to (cr, 0, 0);
	cairo_move_to (cr, data[3].point.x, data[3].point.y);
	break;
    case CAIRO_PATH_CLOSE_PATH:
	cairo_rel_line_to (cr, 0, 0);
	break;
    }
  }
  cairo_rel_line_to (cr, 0, 0);
  cairo_stroke (cr);
  cairo_restore (cr);

  for (i=0; i < path->num_data; i += path->data[i].header.length) {
    data = &path->data[i];
    switch (data->header.type) {
    case CAIRO_PATH_MOVE_TO:
	cairo_move_to (cr, data[1].point.x, data[1].point.y);
	break;
    case CAIRO_PATH_LINE_TO:
	cairo_line_to (cr, data[1].point.x, data[1].point.y);
	break;
    case CAIRO_PATH_CURVE_TO:
	cairo_curve_to (cr, data[1].point.x, data[1].point.y,
			    data[2].point.x, data[2].point.y,
			    data[3].point.x, data[3].point.y);
	break;
    case CAIRO_PATH_CLOSE_PATH:
	cairo_close_path (cr);
	break;
    }
  }
  cairo_stroke (cr);

  if (preserve)
    cairo_append_path (cr, path);

  cairo_path_destroy (path);

  cairo_restore (cr);
}

void
fancy_cairo_stroke (cairo_t *cr)
{
  _fancy_cairo_stroke (cr, FALSE);
}

void
fancy_cairo_stroke_preserve (cairo_t *cr)
{
  _fancy_cairo_stroke (cr, TRUE);
}

typedef void (*transform_point_func_t) (void *closure, double *x, double *y);

static void
transform_path (cairo_path_t *path, transform_point_func_t f, void *closure)
{
  int i;
  cairo_path_data_t *data;

  for (i=0; i < path->num_data; i += path->data[i].header.length) {
    data = &path->data[i];
    switch (data->header.type) {
    case CAIRO_PATH_CURVE_TO:
      f (closure, &data[3].point.x, &data[3].point.y);
      f (closure, &data[2].point.x, &data[2].point.y);
    case CAIRO_PATH_MOVE_TO:
    case CAIRO_PATH_LINE_TO:
      f (closure, &data[1].point.x, &data[1].point.y);
      break;
    case CAIRO_PATH_CLOSE_PATH:
      break;
    }
  }
}

typedef struct {
  cairo_path_t *path;
  parametrization_t *parametrization;
} parametrized_path_t;

static void
point_on_path (parametrized_path_t *param,
	       double *x, double *y)
{
  int i;
  double ratio, oldy = *y, d = *x, dx, dy;
  cairo_path_data_t *data, current_point;
  cairo_path_t *path = param->path;
  parametrization_t *parametrization = param->parametrization;

  for (i=0; i + path->data[i].header.length < path->num_data &&
	    (d > parametrization[i] ||
	     path->data[i].header.type == CAIRO_PATH_MOVE_TO);
       i += path->data[i].header.length) {
    d -= parametrization[i];
    data = &path->data[i];
    switch (data->header.type) {
    case CAIRO_PATH_MOVE_TO:
	current_point = data[1];
	break;
    case CAIRO_PATH_LINE_TO:
	current_point = data[1];
	break;
    case CAIRO_PATH_CURVE_TO:
	current_point = data[3];
	break;
    case CAIRO_PATH_CLOSE_PATH:
	break;
    }
  }
  data = &path->data[i];

  switch (data->header.type) {

  case CAIRO_PATH_MOVE_TO:
      break;
  case CAIRO_PATH_LINE_TO:
      ratio = d / parametrization[i];
      *x = current_point.point.x * (1 - ratio) + data[1].point.x * ratio;
      *y = current_point.point.y * (1 - ratio) + data[1].point.y * ratio;

      dx = -(current_point.point.x - data[1].point.x);
      dy = -(current_point.point.y - data[1].point.y);

      d = oldy;
      ratio = d / parametrization[i];
      /*ratio = d / sqrt (dx * dx + dy * dy);*/

      *x += -dy * ratio;
      *y +=  dx * ratio;

      break;
  case CAIRO_PATH_CURVE_TO:
      ratio = d / parametrization[i];
      *x = current_point.point.x * (1 - ratio) * (1 - ratio) * (1 - ratio)
	 + 3 *   data[1].point.x * (1 - ratio) * (1 - ratio) * ratio
	 + 3 *   data[2].point.x * (1 - ratio) *      ratio  * ratio
	 +       data[3].point.x *      ratio  *      ratio  * ratio;
      *y = current_point.point.y * (1 - ratio) * (1 - ratio) * (1 - ratio)
	 + 3 *   data[1].point.y * (1 - ratio) * (1 - ratio) * ratio
	 + 3 *   data[2].point.y * (1 - ratio) *      ratio  * ratio
	 +       data[3].point.y *      ratio  *      ratio  * ratio;

      dx =-3 * current_point.point.x * (1 - ratio) * (1 - ratio)
	 + 3 *       data[1].point.x * (1 - 4 * ratio + 3 * ratio * ratio)
	 + 3 *       data[2].point.x * (    2 * ratio - 3 * ratio * ratio)
	 + 3 *       data[3].point.x *      ratio  *      ratio;
      dy =-3 * current_point.point.y * (1 - ratio) * (1 - ratio)
	 + 3 *       data[1].point.y * (1 - 4 * ratio + 3 * ratio * ratio)
	 + 3 *       data[2].point.y * (    2 * ratio - 3 * ratio * ratio)
	 + 3 *       data[3].point.y *      ratio  *      ratio;

      d = oldy;
      ratio = d / sqrt (dx * dx + dy * dy);

      *x += -dy * ratio;
      *y +=  dx * ratio;

      break;
  case CAIRO_PATH_CLOSE_PATH:
      break;
  }
}

static void
map_path_onto (cairo_t *cr, cairo_path_t *path)
{
  cairo_path_t *current_path;
  parametrized_path_t param;

  param.path = path;
  param.parametrization = parametrize_path (path);

  current_path = cairo_copy_path (cr);
  cairo_new_path (cr);

  transform_path (current_path,
		  (transform_point_func_t) point_on_path, &param);

  cairo_append_path (cr, current_path);
}


typedef void (*draw_path_func_t) (cairo_t *cr);

static void
draw_text (cairo_t *cr,
	   double x,
	   double y,
	   const char *font,
	   const char *text)
{
  PangoLayout *layout;
  PangoLayoutLine *line;
  PangoFontDescription *desc;
  cairo_font_options_t *font_options;

  font_options = cairo_font_options_create ();

  cairo_font_options_set_hint_style (font_options, CAIRO_HINT_STYLE_NONE);
  cairo_font_options_set_hint_metrics (font_options, CAIRO_HINT_METRICS_OFF);

  cairo_set_font_options (cr, font_options);
  cairo_font_options_destroy (font_options);

  layout = pango_cairo_create_layout (cr);

  desc = pango_font_description_from_string (font);
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);

  pango_layout_set_text (layout, text, -1);

  /* Use pango_layout_get_line() instead of pango_layout_get_line_readonly()
   * for older versions of pango
   */
  line = pango_layout_get_line_readonly (layout, 0);

  cairo_move_to (cr, x, y);
  pango_cairo_layout_line_path (cr, line);

  g_object_unref (layout);
}

static void
draw_twisted (cairo_t *cr,
	      double x,
	      double y,
	      const char *font,
	      const char *text)
{
  cairo_path_t *path;

  cairo_save (cr);

  /* Decrease tolerance a bit, since it's going to be magnified */
  cairo_set_tolerance (cr, 0.05);

  path = cairo_copy_path_flat (cr);
  /*path = cairo_copy_path (cr);*/

  cairo_new_path (cr);

  draw_text (cr, x, y, font, text);
  map_path_onto (cr, path);

  cairo_fill_preserve (cr);

  cairo_save (cr);
  cairo_set_source_rgb (cr, 0.1, 0.1, 0.1);
  cairo_stroke (cr);
  cairo_restore (cr);

  cairo_restore (cr);
}

static void
draw_dream (cairo_t *cr)
{
  cairo_move_to (cr, 50, 650);

  cairo_rel_line_to (cr, 250, 50);
  cairo_rel_curve_to (cr, 250, 50, 600, -50, 600, -250);
  cairo_rel_curve_to (cr, 0, -400, -300, -100, -800, -300);

  cairo_set_line_width (cr, 1.5);
  cairo_set_source_rgba (cr, 0.3, 0.3, 1.0, 0.3);

  fancy_cairo_stroke_preserve (cr);

  draw_twisted (cr,
		0, 0,
		"Serif 72",
		"It was a dream... Oh Just a dream...");
}

static void
draw_wow (cairo_t *cr)
{
  cairo_move_to (cr, 400, 780);

  cairo_rel_curve_to (cr, 50, -50, 150, -50, 200, 0);

  cairo_scale (cr, 1.0, 2.0);
  cairo_set_line_width (cr, 2.0);
  cairo_set_source_rgba (cr, 0.3, 1.0, 0.3, 1.0);

  fancy_cairo_stroke_preserve (cr);

  draw_twisted (cr,
		-20, -150,
		"Serif 60",
		"WOW!");
}

int main (int argc, char **argv)
{
  cairo_t *cr;
  char *filename;
  cairo_status_t status;
  cairo_surface_t *surface;

  if (argc != 2)
    {
      g_printerr ("Usage: cairotwisted OUTPUT_FILENAME\n");
      return 1;
    }

  filename = argv[1];

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
					1000, 800);
  cr = cairo_create (surface);

  /* cairo_scale (cr, 0.5, 0.5); */

  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_paint (cr);

  draw_dream (cr);
  draw_wow (cr);

  cairo_destroy (cr);

  status = cairo_surface_write_to_png (surface, filename);
  cairo_surface_destroy (surface);

  if (status != CAIRO_STATUS_SUCCESS)
    {
      g_printerr ("Could not save png to '%s'\n", filename);
      return 1;
    }

  return 0;
}
